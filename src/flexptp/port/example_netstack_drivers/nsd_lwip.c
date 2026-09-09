#include "../../network_stack_driver.h"

#include "../../ptp_defs.h"
#include "../../task_ptp.h"

#include "lwip/err.h"
#include "lwip/igmp.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "netif/ethernet.h"

#include <string.h>

// initialize connection blocks to invalid states
static struct udp_pcb *PTP_L4_PRIMARY_EVENT = NULL;
static struct udp_pcb *PTP_L4_PRIMARY_GENERAL = NULL;
static struct udp_pcb *PTP_L4_PDELAY_EVENT = NULL;
static struct udp_pcb *PTP_L4_PDELAY_GENERAL = NULL;

// store current settings
static PtpTransportType TP = -1;
static PtpDelayMechanism DM = -1;
static bool custom_p2p_8023_primary_dest_valid = false;
static uint8_t custom_p2p_8023_primary_dest[6] = {};
static bool custom_p2p_8023_pdel_dest_valid = false;
static uint8_t custom_p2p_8023_pdel_dest[6] = {};

static const uint8_t zero_mac[6] = {};

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, void *tag);
static void ptp_receive_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

void ptp_nsd_igmp_join_leave(bool join) {
    // only join IGMP if Transport Type is IP
    if (TP == PTP_TP_IPv4) {
        err_t (*igmp_fn)(const ip_addr_t *, const ip_addr_t *) = join ? igmp_joingroup : igmp_leavegroup; // join or leave

        igmp_fn(&netif_default->ip_addr, &PTP_IGMP_PRIMARY); // join E2E DM message group

        if (DM == PTP_DM_P2P) {
            igmp_fn(&netif_default->ip_addr, &PTP_IGMP_PEER_DELAY); // join P2P DM message group
        }
    }
}

void ptp_nsd_init(const NsdInitSettings * init) {
    // lock LWIP core
    LOCK_TCPIP_CORE();

    // leave current IGMP group if applicable
    ptp_nsd_igmp_join_leave(false);

    // first, close all open connection blocks (zero CBDs won't cause trouble)
    if (PTP_L4_PRIMARY_EVENT != NULL) {
        udp_disconnect(PTP_L4_PRIMARY_EVENT);
        udp_remove(PTP_L4_PRIMARY_EVENT);
        PTP_L4_PRIMARY_EVENT = NULL;
    }
    if (PTP_L4_PRIMARY_GENERAL != NULL) {
        udp_disconnect(PTP_L4_PRIMARY_GENERAL);
        udp_remove(PTP_L4_PRIMARY_GENERAL);
        PTP_L4_PRIMARY_GENERAL = NULL;
    }
    if (PTP_L4_PDELAY_EVENT != NULL) {
        udp_disconnect(PTP_L4_PDELAY_EVENT);
        udp_remove(PTP_L4_PDELAY_EVENT);
        PTP_L4_PDELAY_EVENT = NULL;
    }
    if (PTP_L4_PDELAY_GENERAL != NULL) {
        udp_disconnect(PTP_L4_PDELAY_GENERAL);
        udp_remove(PTP_L4_PDELAY_GENERAL);
        PTP_L4_PDELAY_GENERAL = NULL;
    }

    // calling either parameter with -1 just closes connections
    if ((init->tp == -1) || (init->dm == -1)) {
        // message transmission and reception is turned off
        TP = -1;
        DM = -1;
        return;
    }

    // open only the necessary ones
    if (init->tp == PTP_TP_IPv4) {
        // open event and general PRIMARY connections
        PTP_L4_PRIMARY_EVENT = udp_new();
        udp_bind(PTP_L4_PRIMARY_EVENT, &PTP_IGMP_PRIMARY, PTP_PORT_EVENT);
        udp_recv(PTP_L4_PRIMARY_EVENT, ptp_receive_cb, NULL);

        PTP_L4_PRIMARY_GENERAL = udp_new();
        udp_bind(PTP_L4_PRIMARY_GENERAL, &PTP_IGMP_PRIMARY, PTP_PORT_GENERAL);
        udp_recv(PTP_L4_PRIMARY_GENERAL, ptp_receive_cb, NULL);

        // open event and general PDELAY* connections
        if (init->dm == PTP_DM_P2P) {
            PTP_L4_PDELAY_EVENT = udp_new();
            udp_bind(PTP_L4_PDELAY_EVENT, &PTP_IGMP_PEER_DELAY, PTP_PORT_EVENT);
            udp_recv(PTP_L4_PDELAY_EVENT, ptp_receive_cb, NULL);

            PTP_L4_PDELAY_GENERAL = udp_new();
            udp_bind(PTP_L4_PDELAY_GENERAL, &PTP_IGMP_PEER_DELAY, PTP_PORT_GENERAL);
            udp_recv(PTP_L4_PDELAY_GENERAL, ptp_receive_cb, NULL);
        }
    }

    // if custom P2P 802.3 destination are given, store them
    uint8_t mac_size = sizeof(zero_mac);
    if (memcmp(init->primary_p2p_8023_dest, zero_mac, mac_size)) {
        memcpy(custom_p2p_8023_primary_dest, &init->primary_p2p_8023_dest, mac_size);
        custom_p2p_8023_primary_dest_valid = true;
    }
    if (memcmp(init->pdelay_p2p_8023_dest, zero_mac, mac_size)) {
        memcpy(custom_p2p_8023_pdel_dest, &init->pdelay_p2p_8023_dest, mac_size);
        custom_p2p_8023_pdel_dest_valid = true;
    }

    // store configuration
    TP = init->tp;
    DM = init->dm;

    // join new IGMP group
    ptp_nsd_igmp_join_leave(true);

    // unlock LWIP core
    UNLOCK_TCPIP_CORE();
}

static void ptp_receive_cb(void *pArg, struct udp_pcb *pPCB, struct pbuf *pP, const ip_addr_t *pAddr, uint16_t port) {
    // put msg into the queue
    ptp_receive_enqueue(pP->payload, pP->len, pP->time_s, pP->time_ns, PTP_TP_IPv4);

    // release pbuf resources
    pbuf_free(pP);
}

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, void *tag) {
    ptp_transmit_timestamp_cb((uint32_t)tag, ts_s, ts_ns);
}

void ptp_nsd_transmit_msg(RawPtpMessage *pMsg, uint32_t uid) {
    if (pMsg == NULL) {
        MSG("NULL!!!\n");
        return;
    }

    PtpMessageClass mc = pMsg->tx_mc;
    PtpMessageType mt = pMsg->tx_mt;

    // allocate buffer
    struct pbuf *p = NULL;
    p = pbuf_alloc((TP == PTP_TP_IPv4) ? PBUF_TRANSPORT : PBUF_LINK, pMsg->size, PBUF_RAM);

    /* pbuf_alloc() returns NULL when lwIP's heap is exhausted, which sustained receive pressure
     * will do. Dereferencing it here faulted the board rather than dropping a message -- and a
     * transmit that cannot get memory is exactly the case where dropping is right. */
    if (p == NULL) {
        return;
    }

    // fill buffer
    memcpy(p->payload, pMsg->data, pMsg->size);

    // set transmit callback
    p->tag = (void *)uid;
    p->tx_cb = ptp_transmit_cb;

    // lock LWIP core
    LOCK_TCPIP_CORE();

    // is it a Peer Delay Mechanism related message?
    bool isPDel_ = (mt == PTP_MT_PDelay_Req) || (mt == PTP_MT_PDelay_Resp) || (mt == PTP_MT_PDelay_Resp_Follow_Up);

    // narrow down by transport type
    if (TP == PTP_TP_IPv4) {
        struct udp_pcb *conn = (mc == PTP_MC_EVENT) ? PTP_L4_PRIMARY_EVENT : PTP_L4_PRIMARY_GENERAL; // select connection by message type
        uint16_t port = (mc == PTP_MC_EVENT) ? PTP_PORT_EVENT : PTP_PORT_GENERAL;    // select port by message class
        ip_addr_t ipaddr = isPDel_ ? PTP_IGMP_PEER_DELAY : PTP_IGMP_PRIMARY;         // select destination IP-address by PDel*/primary message types
        udp_sendto(conn, p, &ipaddr, port);                                          // send packet
    } else if (TP == PTP_TP_802_3) {
        const uint8_t *ethaddr = isPDel_ ? 
            (custom_p2p_8023_pdel_dest_valid ? custom_p2p_8023_pdel_dest : PTP_ETHERNET_PEER_DELAY) : 
            (custom_p2p_8023_primary_dest_valid ? custom_p2p_8023_primary_dest : PTP_ETHERNET_PRIMARY); // select destination address by PDel*/primary message types
        ethernet_output(netif_default, p, (struct eth_addr *)netif_default->hwaddr, (struct eth_addr *)ethaddr, ETHERTYPE_PTP);
    }

    // unlock LWIP core
    UNLOCK_TCPIP_CORE();

    /* Untag the pbuf before releasing it.
     *
     * tx_cb and tag live in LWIP_PBUF_CUSTOM_DATA, which lwIP does not initialise on
     * allocation -- it manages its own fields and nothing else. So whatever is left here
     * survives into the next allocation that lands on this memory, and a port that gates
     * hardware transmit timestamping on `p->tx_cb != NULL`, which is the only signal this
     * driver offers, then requests a timestamp for a frame that never asked for one and
     * reports it against a stale uid.
     *
     * The send above is synchronous -- udp_sendto() and ethernet_output() reach
     * netif->linkoutput before returning -- so the driver has already taken both values. */
    p->tx_cb = NULL;
    p->tag = NULL;

    pbuf_free(p); // release buffer
}

void ptp_transmit_free(struct pbuf *pPBuf) {
    pbuf_free(pPBuf);
}

void ptp_nsd_get_interface_address(uint8_t *hwa) {
    memcpy(hwa, netif_default->hwaddr, netif_default->hwaddr_len);
}

#define ETHERNET_HEADER_LENGTH (14)

// hook for L2 PTP messages
err_t hook_unknown_ethertype(struct pbuf *pbuf, struct netif *netif) {
    // aquire ethertype
    uint16_t etherType = 0;
    memcpy(&etherType, ((uint8_t *)pbuf->payload) + 12, 2);
    etherType = FLEXPTP_ntohs(etherType);
    if (etherType == ETHERTYPE_PTP) {
        // verify Ethernet address
        if (!memcmp(PTP_ETHERNET_PRIMARY, pbuf->payload, 6) || !memcmp(PTP_ETHERNET_PEER_DELAY, pbuf->payload, 6)) { //
            ptp_receive_enqueue(((uint8_t *)pbuf->payload) + ETHERNET_HEADER_LENGTH, pbuf->len - ETHERNET_HEADER_LENGTH, pbuf->time_s, pbuf->time_ns, PTP_TP_802_3);
        }
    }

    pbuf_free(pbuf);

    return ERR_OK;
}
