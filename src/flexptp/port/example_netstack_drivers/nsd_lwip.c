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
static struct udp_pcb *PTP_L4_EVENT = NULL;
static struct udp_pcb *PTP_L4_GENERAL = NULL;

// store current settings
static PtpTransportType TP = -1;
static PtpDelayMechanism DM = -1;

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, void * tag);
static void ptp_receive_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

void ptp_nsd_igmp_join_leave(bool join) {
    // only join IGMP if Transport Type is IP
    if (TP == PTP_TP_IPv4) {
        err_t (*igmp_fn)(const ip_addr_t *, const ip_addr_t *) = join ? igmp_joingroup : igmp_leavegroup; // join or leave

        if (DM == PTP_DM_E2E) {
            igmp_fn(&netif_default->ip_addr, &PTP_IGMP_PRIMARY); // join E2E DM message group
        } else if (DM == PTP_DM_P2P) {
            igmp_fn(&netif_default->ip_addr, &PTP_IGMP_PEER_DELAY); // join P2P DM message group
        }
    }
}

void ptp_nsd_init(PtpTransportType tp, PtpDelayMechanism dm) {
    // lock LWIP core
    LOCK_TCPIP_CORE();

    // leave current IGMP group if applicable
    ptp_nsd_igmp_join_leave(false);

    // first, close all open connection blocks (zero CBDs won't cause trouble)
    if (PTP_L4_EVENT != NULL) {
        udp_disconnect(PTP_L4_EVENT);
        udp_remove(PTP_L4_EVENT);
        PTP_L4_EVENT = NULL;
    }
    if (PTP_L4_GENERAL != NULL) {
        udp_disconnect(PTP_L4_GENERAL);
        udp_remove(PTP_L4_GENERAL);
        PTP_L4_GENERAL = NULL;
    }

    // calling either parameter with -1 just closes connections
    if ((tp == -1) || (dm == -1)) {
        // message transmission and reception is turned off
        TP = -1;
        DM = -1;
        return;
    }

    // open only the necessary ones
    if (tp == PTP_TP_IPv4) {
        // open event and general connections
        ip_addr_t addr = (dm == PTP_DM_E2E) ? PTP_IGMP_PRIMARY : PTP_IGMP_PEER_DELAY;
        PTP_L4_EVENT = udp_new();
        udp_bind(PTP_L4_EVENT, &addr, PTP_PORT_EVENT);
        udp_recv(PTP_L4_EVENT, ptp_receive_cb, NULL);

        PTP_L4_GENERAL = udp_new();
        udp_bind(PTP_L4_GENERAL, &addr, PTP_PORT_GENERAL);
        udp_recv(PTP_L4_GENERAL, ptp_receive_cb, NULL);
    }

    // store configuration
    TP = tp;
    DM = dm;

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

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, void * tag) {
    RawPtpMessage *pMsg = (RawPtpMessage *)(tag);
    ptp_transmit_timestamp_cb(pMsg, ts_s, ts_ns);
}

void ptp_nsd_transmit_msg(RawPtpMessage *pMsg) {
    if (pMsg == NULL) {
        MSG("NULL!!!\n");
        return;
    }

    PtpMessageClass mc = pMsg->tx_mc;

    // allocate buffer
    struct pbuf *p = NULL;
    p = pbuf_alloc((TP == PTP_TP_IPv4) ? PBUF_TRANSPORT : PBUF_LINK, pMsg->size, PBUF_RAM);

    // fill buffer
    memcpy(p->payload, pMsg->data, pMsg->size);

    // set transmit callback
    p->tag = pMsg;
    p->tx_cb = ptp_transmit_cb;

    // lock LWIP core
    LOCK_TCPIP_CORE();

    // narrow down by transport type
    if (TP == PTP_TP_IPv4) {
        struct udp_pcb *conn = (mc == PTP_MC_EVENT) ? PTP_L4_EVENT : PTP_L4_GENERAL;    // select connection by message type
        uint16_t port = (mc == PTP_MC_EVENT) ? PTP_PORT_EVENT : PTP_PORT_GENERAL;       // select port by message class
        ip_addr_t ipaddr = (DM == PTP_DM_E2E) ? PTP_IGMP_PRIMARY : PTP_IGMP_PEER_DELAY; // select destination IP-address by delmech.
        udp_sendto(conn, p, &ipaddr, port);                                             // send packet
    } else if (TP == PTP_TP_802_3) {
        const uint8_t *ethaddr = (DM == PTP_DM_E2E) ? PTP_ETHERNET_PRIMARY : PTP_ETHERNET_PEER_DELAY; // select destination address by delmech.
        ethernet_output(netif_default, p, (struct eth_addr *)netif_default->hwaddr, (struct eth_addr *)ethaddr, ETHERTYPE_PTP);
    }

    // unlock LWIP core
    UNLOCK_TCPIP_CORE();

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
