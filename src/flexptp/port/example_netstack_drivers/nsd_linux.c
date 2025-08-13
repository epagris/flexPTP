#include "../../network_stack_driver.h"

#include "../../ptp_defs.h"
#include "../../task_ptp.h"

#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm/socket.h>
#include <fcntl.h>
#include <linux/ethtool.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>

#include <features.h>
#include <asm-generic/unistd.h>
#include <bits/time.h>

#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/timex.h>

#define LINUX_NSD_TS_DEBUG (0)         // timestamp debugging
#define LINUX_NSD_TX_ENQUEUE_DEBUG (0) // transmit enqueue debugging

// initialize connection blocks to invalid states
static int event_fd = -1;
static int general_fd = -1;

// store current settings
static PtpTransportType TP = -1;
static PtpDelayMechanism DM = -1;

// interface data
static uint16_t if_idx;                // interface index
static char if_name[IFNAMSIZ];         // name of the interface
static uint8_t if_hwaddr[IFHWADDRLEN]; // hardware address of the interface
static struct sockaddr_in if_ipaddr;   // IP-address of the interface

// hardware clock data
#define PHY_FILE_NAME_SIZE (16)
static uint16_t phc_index;                     // index of the PHC
static char phc_file_name[PHY_FILE_NAME_SIZE]; // PHC device file name
static int phc_fd;                             // PHC file descriptor
static clockid_t phc_clkid;                    // PHC clock id

// transception management
static int notif_q[2];               // notification queue
static int matching_q[2];            // message pointer queue
static pthread_t transceiver_thread; // thread managing transmission and reception
static void *nsd_thread(void *arg);  // thread function

#define PRINT_HWADDR(a) MSG("%02X:%02X:%02X:%02X:%02X:%02X", a[0], a[1], a[2], a[3], a[4], a[5]);

#define NOTIF_QUIT_TRANSCEIVER_THREAD 'Q'

static void post_notification(char c) {
    write(notif_q[1], &c, 1);
}

/* FD <-> CLOCKID conversions (man 2 clock_getres, 'Dynamic clocks' section) */
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd) ((clockid_t)((((unsigned int)~fd) << 3) | CLOCKFD))
#define CLOCKID_TO_FD(clk) ((unsigned int)~((clk) >> 3))

bool linux_nsd_preinit(const char *ifn) {
    bool init_ok = false;

    // copy interface name
    strncpy(if_name, ifn, IFNAMSIZ - 1);
    if_name[IFNAMSIZ - 1] = '\0';

    // create dummy socket
    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        MSG("Could not create a dummy socket to retrieve interface data!\n");
        return false;
    }

    int err;

    // retrieve the network interface index
    // man 7 netdevice
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
    err = ioctl(fd, SIOCGIFINDEX, &ifr);
    if (err < 0) {
        MSG("Could not get interface index!\n");
        goto cleanup;
    }
    if_idx = ifr.ifr_ifindex;

    // retrieve the hardware address of the interface
    // man 7 netdevice
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
    err = ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (err < 0) {
        MSG("Failed to retrieve the hardware address!\n");
        goto cleanup;
    }
    memcpy(if_hwaddr, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);

    // retrieve the IP address of the interface
    // man 7 netdevice
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
    err = ioctl(fd, SIOCGIFADDR, &ifr);
    if (err < 0) {
        MSG("Failed to retrieve the interface IP address!\n");
        goto cleanup;
    }
    memcpy(&if_ipaddr, &ifr.ifr_addr, sizeof(struct sockaddr_in));

    // print collected information
    MSG("Network Interface information\n");
    MSG("-- Interface: " PTP_COLOR_YELLOW "%s\n" PTP_COLOR_RESET, if_name);
    MSG("   Hardware address: " PTP_COLOR_CYAN);
    PRINT_HWADDR(if_hwaddr);
    MSG("\n");
    MSG(PTP_COLOR_RESET);
    MSG("   IP-address: " PTP_COLOR_CYAN "%s\n" PTP_COLOR_RESET, inet_ntoa(if_ipaddr.sin_addr));

    // check the hardware timestamp support
    // https://docs.kernel.org/networking/ethtool-netlink.html#tsinfo-get
    struct ethtool_ts_info tsi = {.cmd = ETHTOOL_GET_TS_INFO};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
    ifr.ifr_data = (caddr_t)&tsi;
    err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err < 0) {
        MSG("Failed to query the interface timestamp capabilities!\n");
        goto cleanup;
    }

    // print timestamp information
    MSG("   Hardware timestamping: ");
    if ((tsi.so_timestamping & SOF_TIMESTAMPING_TX_HARDWARE) && (tsi.so_timestamping & SOF_TIMESTAMPING_RX_HARDWARE)) {
        phc_index = tsi.phc_index;
        FLEXPTP_SNPRINTF(phc_file_name, PHY_FILE_NAME_SIZE - 1, "/dev/ptp%u", phc_index);
        phc_file_name[PHY_FILE_NAME_SIZE - 1] = '\0';

        MSG(PTP_COLOR_GREEN "OK\n" PTP_COLOR_RESET);
        MSG("   PHC index: " PTP_COLOR_CYAN "%u\n" PTP_COLOR_RESET, phc_index);
        MSG("    -- corresponding file: " PTP_COLOR_CYAN "%s\n" PTP_COLOR_RESET, phc_file_name);
    } else {
        MSG(PTP_COLOR_RED "MISSING\n" PTP_COLOR_RESET);
        goto cleanup;
    }
    MSG("---------------\n\n");

    // open PHC
    phc_fd = open(phc_file_name, O_RDWR);
    if (phc_fd < 0) {
        MSG("Failed to open PHC file!\n");
        goto cleanup;
    }
    phc_clkid = FD_TO_CLOCKID(phc_fd);

    struct timespec ts;
    if (clock_gettime(phc_clkid, &ts) < 0) {
        MSG("Failed to access the PHC time!\n");
        goto cleanup;
    }

    // create the notification queue
    if (pipe(notif_q) < 0) {
        MSG("Failed to create notification queue!\n");
        goto cleanup;
    }

    // create message pointer queue
    if (pipe(matching_q) < 0) {
        MSG("Failed to create buffer matching pointer queue!\n");
        goto cleanup;
    }

    // clear thread handle
    transceiver_thread = 0;

    // initialization done
    init_ok = true;
    return init_ok;

cleanup:
    if (fd > 0) {
        close(fd);
    }
    if (phc_fd > 0) {
        close(phc_fd);
    }
    return init_ok;
}

void linux_nsd_cleanup(void) {
    if (phc_fd > 0) {
        close(phc_fd);
        phc_fd = 0;
    }
}

static void socket_join_igmp(int fd) {
    // fill in the multicast assignment request
    struct ip_mreq mreq;
    if (DM == PTP_DM_E2E) {
        mreq.imr_multiaddr.s_addr = PTP_IGMP_PRIMARY;
    } else if (DM == PTP_DM_P2P) {
        mreq.imr_multiaddr.s_addr = PTP_IGMP_PEER_DELAY;
    }
    mreq.imr_interface = if_ipaddr.sin_addr;

    // join the IGMP group
    // man 7 ip
    int err = setsockopt(event_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0) {
        // MSG("Could not join the required network group!\n");
    }
}

void ptp_nsd_igmp_join_leave(bool join) {
    // only join IGMP if Transport Type is IP
    if ((TP == PTP_TP_IPv4) && join) {
        if (event_fd > 0) {
            socket_join_igmp(event_fd);
        }
        if (general_fd > 0) {
            socket_join_igmp(general_fd);
        }
    }

    // don't have to explicitly leave the IGMP group
}

static int open_udp_socket(PtpDelayMechanism dm, uint16_t port, const char *hint) {
    // prepare socket address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = (dm == PTP_DM_E2E) ? PTP_IGMP_PRIMARY : PTP_IGMP_PEER_DELAY;

    // create socket
    // man 2 socket
    int sfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sfd < 0) {
        MSG("Could not open the %s socket!\n", hint);
        return -1;
    }

    // enable the reuseaddr option
    // man 3 setsockopt
    int optval = 1;
    int err = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (err < 0) {
        MSG("Could not set the %s socket REUSEADDR option!\n", hint);
        goto cleanup;
    }

    // assign the multicast transmit interface with the socket
    // man 7 ip
    err = setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_IF, &if_ipaddr.sin_addr, sizeof(struct in_addr));
    if (err < 0) {
        MSG("Could not set the %s socket IP_MULTICAST_IF option!\n", hint);
        goto cleanup;
    }

    // bind the socket
    // man 2 bind
    addr.sin_port = htons(port);
    err = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        MSG("Could not bind the %s socket!\n", hint);
        goto cleanup;
    }

    // normal return
    return sfd;

cleanup:
    close(sfd);
    return -1;
}

static int open_raw_socket(PtpDelayMechanism dm, bool bind_socket, const char *hint) {
    const uint8_t *ethaddr = (dm == PTP_DM_E2E) ? PTP_ETHERNET_PRIMARY : PTP_ETHERNET_PEER_DELAY;

    // create socket
    // SOCK_DGRAM: use the kernel features to fill in the Ethernet header
    int sfd = socket(AF_PACKET, SOCK_DGRAM, htons(PTP_ETHERTYPE));
    if (sfd < 0) {
        MSG("Could not open the %s socket!\n", hint);
        return -1;
    }

    // setup address
    // man 7 packet
    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_ifindex = if_idx;
    addr.sll_halen = ETH_ALEN;
    addr.sll_protocol = htons(PTP_ETHERTYPE);
    addr.sll_family = AF_PACKET;
    addr.sll_pkttype = PACKET_MULTICAST;
    memcpy(addr.sll_addr, ethaddr, ETH_ALEN);

    // bind socket if requested
    // man 2 bind
    int err;
    if (bind_socket) {
        err = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
        if (err < 0) {
            MSG("Could not bind the %s socket!\n", hint);
        }
    }

    // assign the socket the multicast membership
    // man 6 packet
    struct packet_mreq mreq;
    mreq.mr_ifindex = if_idx;
    mreq.mr_type = PACKET_MR_MULTICAST;
    mreq.mr_alen = ETH_ALEN;
    memcpy(mreq.mr_address, ethaddr, ETH_ALEN);

    err = setsockopt(sfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0) {
        MSG("Could not set the %s socket ADD_MEMBERSHIP option!\n", hint);
    }

    return sfd;
}

static void enable_timestamping(int sfd) {
    // enable timestamping on the socket
    // https://www.kernel.org/doc/html/latest/networking/timestamping.html#scm-timestamping-records
    int optval = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE | SOF_TIMESTAMPING_TX_HARDWARE;
    int err = setsockopt(sfd, SOL_SOCKET, SO_TIMESTAMPING, &optval, sizeof(optval));
    if (err < 0) {
        MSG("Failed to enable timestamping\n");
    }

    // enable timestamping in the hardware
    struct ifreq ifreq;
    struct hwtstamp_config cfg;
    memset(&ifreq, 0, sizeof(ifreq));
    memset(&cfg, 0, sizeof(cfg));
    strncpy(ifreq.ifr_name, if_name, IFNAMSIZ - 1);
    ifreq.ifr_data = (void *)&cfg;

    // get current timestamping settings
    err = ioctl(sfd, SIOCGHWTSTAMP, &ifreq);
    if (err < 0) {
        MSG("Failed to get timestamping settings.\n");
        return;
    }

    // turn on TX and RX timestamping
    // https://www.kernel.org/doc/html/latest/networking/timestamping.html#hardware-timestamping-configuration-ethtool-msg-tsconfig-set-get
    cfg.flags = 0;
    cfg.tx_type = HWTSTAMP_TX_ON;
    cfg.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
    err = ioctl(sfd, SIOCSHWTSTAMP, &ifreq);
    if (err < 0) {
        MSG("Failed to set timestamping settings.\n");
    }

    // enable TX timestamp communication through the socket error queue
    // man 7 socket
    optval = 1;
    err = setsockopt(sfd, SOL_SOCKET, SO_SELECT_ERR_QUEUE, &optval, sizeof(optval));
    if (err < 0) {
        MSG("Could not enable TX timestamp communication through the error queue!\n");
    }
}

#define NAME_BUF_SIZE (256)
static char name_buf[NAME_BUF_SIZE];
#define MSG_BUF_SIZE (1600)
static char msg_buf[MSG_BUF_SIZE];
#define CTRL_BUF_SIZE (256)
static char rx_ctrl_buf[CTRL_BUF_SIZE];

static void *nsd_thread(void *arg) {
    bool run = true;
    while (run) {
        // populate the poll list
        struct pollfd pfd[] = {
            {.fd = notif_q[0], .events = POLLIN},
            {.fd = event_fd, .events = POLLIN | POLLPRI},
            {.fd = general_fd, .events = POLLIN},
        };

        // in IEEE 802.3 mode only the first two slots are used
        int n = (TP == PTP_TP_IPv4) ? 3 : 2;

        // make the poll
        int pret = poll(pfd, n, -1);
        if (pret > 0) {
            // notifications
            if (pfd[0].revents & POLLIN) {
                char c;
                read(notif_q[0], &c, sizeof(char));
                if (c == NOTIF_QUIT_TRANSCEIVER_THREAD) {
                    run = false;
                    continue;
                }
            }

            // something had happened on the event message socket
            if (pfd[1].revents != 0) {

                // prepare for message reception
                struct iovec iov = {msg_buf, MSG_BUF_SIZE};
                struct msghdr msg;

                memset(&msg, 0, sizeof(msg));
                memset(msg_buf, 0, sizeof(msg_buf));

                msg.msg_name = name_buf;
                msg.msg_namelen = NAME_BUF_SIZE;
                msg.msg_iov = &iov;
                msg.msg_iovlen = 1;
                msg.msg_control = rx_ctrl_buf;
                msg.msg_controllen = CTRL_BUF_SIZE;

                // event message TRANSMISSION timestamp feedback
                // https://www.kernel.org/doc/html/latest/networking/timestamping.html#scm-timestamping-records
                if (pfd[1].revents & POLLPRI) {
                    ssize_t size = recvmsg(event_fd, &msg, MSG_ERRQUEUE); // get transmit timestamps from the error queue
                    struct cmsghdr *cm; // iterate over the chain of control messages
                    for (cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm)) {
                        int level = cm->cmsg_level;
                        int type = cm->cmsg_type;
                        if ((level == SOL_SOCKET) && (type == SO_TIMESTAMPING)) {
                            struct timespec *ts = (struct timespec *)CMSG_DATA(cm); // get data from the timestamp control message
                            uint32_t uid = 0;
                            read(matching_q[0], &uid, sizeof(uint32_t));

                            struct timespec now;
                            clock_gettime(CLOCK_REALTIME, &now);
                            CLILOG(LINUX_NSD_TS_DEBUG, "[%lu.%09lu] TX TS: (%u) %lu.%09lu\n", now.tv_sec, now.tv_nsec, uid, ts[2].tv_sec, ts[2].tv_nsec);

                            // invoke the transmit timestamp callback, the hardware timestamp always comes in ts[2]
                            ptp_transmit_timestamp_cb(uid, ts[2].tv_sec, ts[2].tv_nsec);
                        }
                    }
                }

                // event message RECEPTION
                if (pfd[1].revents & POLLIN) {
                    ssize_t size = recvmsg(event_fd, &msg, 0);
                    struct cmsghdr *cm;
                    struct timespec ts;
                    memset(&ts, 0, sizeof(ts));
                    bool ts_found = false;
                    for (cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm)) {
                        int level = cm->cmsg_level;
                        int type = cm->cmsg_type;
                        if ((level == SOL_SOCKET) && (type == SO_TIMESTAMPING)) {
                            struct timespec *tsa = (struct timespec *)CMSG_DATA(cm); // get pointer to the timestamps
                            ts = tsa[2];                                             // extract the hardware timestamp
                            ts_found = true;                                         // indicate that timestamp was found
                            CLILOG(LINUX_NSD_TS_DEBUG, "RX TS: %lu.%09lu\n", ts.tv_sec, ts.tv_nsec);
                        }
                    }

                    // forward only event messages over IPv4 and ALL messages over Ethernet
                    if (((TP == PTP_TP_IPv4) && (ts_found)) || (TP == PTP_TP_802_3)) {
                        ptp_receive_enqueue(msg_buf, size, ts.tv_sec, ts.tv_nsec, TP);
                    }
                }
            }

            // general message reception (in IPv4 mode)
            if (TP == PTP_TP_IPv4) {
                if (pfd[2].revents & POLLIN) {
                    ssize_t size = recv(general_fd, msg_buf, MSG_BUF_SIZE, 0);
                    if (size > 0) {
                        ptp_receive_enqueue(msg_buf, size, 0, 0, TP);
                    }
                }
            }
        }
    }

    return NULL;
}

void ptp_nsd_init(PtpTransportType tp, PtpDelayMechanism dm) {
    // leave current IGMP group if applicable
    ptp_nsd_igmp_join_leave(false);

    // first, close all open connection blocks (zero CBDs won't cause trouble)
    if (transceiver_thread != 0) {
        post_notification(NOTIF_QUIT_TRANSCEIVER_THREAD);
        pthread_join(transceiver_thread, NULL);
        transceiver_thread = 0;
    }
    if (event_fd > 0) {
        close(event_fd);
        event_fd = -1;
    }
    if (general_fd > 0) {
        close(general_fd);
        general_fd = -1;
    }

    // calling either parameter with -1 just closes connections
    if ((tp == -1) || (dm == -1)) {
        // message transmission and reception is turned off
        TP = -1;
        DM = -1;
        return;
    }

    // open event and general connections
    if (tp == PTP_TP_IPv4) {
        event_fd = open_udp_socket(dm, PTP_PORT_EVENT, "EVENT");
        general_fd = open_udp_socket(dm, PTP_PORT_GENERAL, "GENERAL");
    } else if (tp == PTP_TP_802_3) {
        event_fd = open_raw_socket(dm, true, "EVENT");
        general_fd = open_raw_socket(dm, false, "GENERAL");
    }

    // enable timestamping on the event socket
    enable_timestamping(event_fd);

    // create the transceiver thread
    transceiver_thread = 0;
    if (pthread_create(&transceiver_thread, NULL, nsd_thread, NULL) != 0) {
        MSG("Failed to create the transceiver thread!\n");
    }

    // store configuration
    TP = tp;
    DM = dm;

    // join new IGMP group
    ptp_nsd_igmp_join_leave(true);
}

void ptp_nsd_transmit_msg(RawPtpMessage *pMsg, uint32_t uid) {
    if (pMsg == NULL) {
        return;
    }

    // indicates if the transmission was successful
    bool send_ok = false;

    // get the message class
    PtpMessageClass mc = pMsg->tx_mc;

    // select connection by message type
    int sfd = (mc == PTP_MC_EVENT) ? event_fd : general_fd;

    // narrow down by transport type
    if (TP == PTP_TP_IPv4) {
        // configure the address
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = PF_INET;
        addr.sin_addr.s_addr = (DM == PTP_DM_E2E) ? PTP_IGMP_PRIMARY : PTP_IGMP_PEER_DELAY; // select destination IP-address by delmech.
        addr.sin_port = htons((mc == PTP_MC_EVENT) ? PTP_PORT_EVENT : PTP_PORT_GENERAL);    // select port by message class

        // send packet
        if (sendto(sfd, pMsg->data, pMsg->size, 0, (struct sockaddr *)&addr, sizeof(addr)) == pMsg->size) {
            send_ok = true;
        }
    } else if (TP == PTP_TP_802_3) {
        // destination address
        const uint8_t *ethaddr = (DM == PTP_DM_E2E) ? PTP_ETHERNET_PRIMARY : PTP_ETHERNET_PEER_DELAY; // select destination address by delmech.

        // prepare address object
        struct sockaddr_ll addr;
        memset(&addr, 0, sizeof(addr));
        addr.sll_ifindex = if_idx;
        addr.sll_halen = ETH_ALEN;
        addr.sll_protocol = htons(PTP_ETHERTYPE);
        memcpy(addr.sll_addr, ethaddr, ETH_ALEN);

        if (sendto(sfd, pMsg->data, pMsg->size, 0, (struct sockaddr *)&addr, sizeof(addr)) == pMsg->size) {
            send_ok = true;
        };
    }

    // send message UID to the queue or invoke the TX callback
    if (send_ok) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        CLILOG(LINUX_NSD_TX_ENQUEUE_DEBUG, "[%lu.%09lu] TX enqueue! %u\n", now.tv_sec, now.tv_nsec, uid);
        if (mc == PTP_MC_EVENT) {
            write(matching_q[1], &uid, sizeof(uint32_t));
        } else if (mc == PTP_MC_GENERAL) {
            ptp_transmit_timestamp_cb(uid, 0, 0);
        }
    }
}

void ptp_nsd_get_interface_address(uint8_t *hwa) {
    memcpy(hwa, if_hwaddr, IFHWADDRLEN);
}

// ------------------------

/* man 2 clock_adjtime (ADJ_FREQUENCY) */
#define PPB_TO_TUNING_SCALER (((double)(1 << 16)) / 1000.0)

void linux_adjust_clock(double tuning_ppb) {
    struct timex tx;
    memset(&tx, 0, sizeof(struct timex));
    if (clock_adjtime(phc_clkid, &tx) < 0) {
        MSG("Failed to retrieve PHC tuning!\n");
    }
    memset(&tx, 0, sizeof(struct timex));
    tx.modes = ADJ_FREQUENCY;
    tx.freq = (__syscall_slong_t)(tuning_ppb * PPB_TO_TUNING_SCALER);
    if (clock_adjtime(phc_clkid, &tx) != 0) {
        MSG("Failed to adjust PHC frequency!\n");
    }
}

void linux_set_time(uint32_t seconds, uint32_t nanoseconds) {
    struct timespec ts = {.tv_sec = seconds, .tv_nsec = nanoseconds};
    if (clock_settime(phc_clkid, &ts) < 0) {
        MSG("Failed to set the PHC time!\n");
    }
}

void linux_get_time(TimestampU *pTime) {
    struct timespec ts;
    if (clock_gettime(phc_clkid, &ts) < 0) {
        MSG("Failed to get the PHC time!\n");
    }

    pTime->sec = ts.tv_sec;
    pTime->nanosec = ts.tv_nsec;
}