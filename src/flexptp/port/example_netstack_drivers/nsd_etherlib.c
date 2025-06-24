#include "../../network_stack_driver.h"

#include "../../ptp_defs.h"
#include "../../ptp_types.h"
#include "../../task_ptp.h"

#include "etherlib/cbd_table.h"
#include "etherlib/connection_block.h"
#include "etherlib/eth_interface.h"
#include "etherlib/global_state.h"
#include "etherlib/prefab/conn_blocks/custom_ethertype_connblock.h"
#include "etherlib/prefab/conn_blocks/igmp_connblock.h"
#include "etherlib/prefab/packet_parsers/ethernet_frame.h"
#include "etherlib/prefab/packet_parsers/ipv4_types.h"
#include "etherlib/timestamping.h"

#include <stdbool.h>
#include <string.h>

// initialize connection blocks to invalid states
static cbd PTP_L4_EVENT = 0;
static cbd PTP_L4_GENERAL = 0;
static cbd PTP_L2 = 0;

// store current settings
static PtpTransportType TP = -1;
static PtpDelayMechanism DM = -1;

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, uint32_t tag);
static int ptp_receive_cb(const Pckt *packet, PcktSieveLayerTag tag);

void ptp_nsd_igmp_join_leave(bool join) {
    // only join IGMP if Transport Type is IP
    if (TP == PTP_TP_IPv4) {
        ConnBlock cb = igmp_new_connblock(get_default_interface()); // open IGMP

        void (*igmp_fn)(ConnBlock *, ip4_addr) = join ? igmp_report_membership : igmp_leave_group; // join or leave

        if (DM == PTP_DM_E2E) {
            igmp_fn(&cb, PTP_IGMP_PRIMARY); // join E2E DM message group
        } else if (DM == PTP_DM_P2P) {
            igmp_fn(&cb, PTP_IGMP_PEER_DELAY); // join P2P DM message group
        }
        connb_remove(&cb); // close IGMP
    }
}

#define CLOSE_CONNECTION_IF_EXISTS(d) if (d >= CBD_LOWEST_DESCRIPTOR) { close_connection(d); d = 0; }

void ptp_nsd_init(PtpTransportType tp, PtpDelayMechanism dm) {
    // leave current IGMP group if applicable
    ptp_nsd_igmp_join_leave(false);

    // first, close all open connection blocks (zero CBDs won't cause trouble)
    CLOSE_CONNECTION_IF_EXISTS(PTP_L4_EVENT);
    CLOSE_CONNECTION_IF_EXISTS(PTP_L4_GENERAL);
    CLOSE_CONNECTION_IF_EXISTS(PTP_L2);

    // calling either parameter with -1 just closes connections
    if ((tp == -1) || (dm == -1)) {
        // message transmission and reception is turned off
        TP = -1;
        DM = -1;
        return;
    }

    // open only the necessary ones
    EthInterface *intf = get_default_interface();

    switch (tp) {
    case PTP_TP_IPv4: {
        // open event and general connections
        ip4_addr addr = (dm == PTP_DM_E2E) ? PTP_IGMP_PRIMARY : PTP_IGMP_PEER_DELAY;
        PTP_L4_EVENT = udp_new_connblock(intf, addr, PTP_PORT_EVENT, ptp_receive_cb);
        PTP_L4_GENERAL = udp_new_connblock(intf, addr, PTP_PORT_GENERAL, ptp_receive_cb);

        // set transmit callbacks
        ts_set_tx_callback(PTP_L4_EVENT, ptp_transmit_cb);
        ts_set_tx_callback(PTP_L4_GENERAL, ptp_transmit_cb);
    } break;
    case PTP_TP_802_3:
        PTP_L2 = cet_new_connblock(intf, ETHERTYPE_PTP, ptp_receive_cb); // open connection
        ts_set_tx_callback(PTP_L2, ptp_transmit_cb);                  // set transmit callback
        break;
    }

    // store settings
    TP = tp;
    DM = dm;

    // join new IGMP group
    ptp_nsd_igmp_join_leave(true);
}

static int ptp_receive_cb(const Pckt *packet, PcktSieveLayerTag tag) {
    // put msg into the queue
    int tp = -1;
    uint16_t pcktClass = packet->header->props.ownPacketClass;
    switch (pcktClass) {
    case ETH_UDP_PACKET_CLASS:
        tp = PTP_TP_IPv4;
        break;
    case 0:
        tp = PTP_TP_802_3;
        break;
    default:
        break;
    }

    if (tp != -1) {
        ptp_receive_enqueue(packet->payload, packet->payloadSize, packet->time_s, packet->time_ns, tp);
    } else {
        MSG("Unknown PTP packet class: '%d'!\n", tp);
    }

    return 0;
}

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, uint32_t tag) {
    RawPtpMessage *pMsg = (RawPtpMessage *)tag;
    pMsg->ts.sec = ts_s;
    pMsg->ts.nanosec = ts_ns;
    if (pMsg->pTs != NULL) {
        pMsg->pTs->sec = ts_s;
        pMsg->pTs->nanosec = ts_ns;
    }
    if (pMsg->pTxCb) {
        pMsg->pTxCb(pMsg);
    }

    // free buffer
    ptp_circ_buf_free(&gRawTxMsgBuf);
}

void ptp_nsd_transmit_msg(RawPtpMessage *pMsg) {
    PtpMessageClass mc = pMsg->tx_mc;

    // narrow down by transport type
    if (TP == PTP_TP_IPv4) {
        cbd conn = (mc == PTP_MC_EVENT) ? PTP_L4_EVENT : PTP_L4_GENERAL;                // select connection by message type
        uint16_t port = (mc == PTP_MC_EVENT) ? PTP_PORT_EVENT : PTP_PORT_GENERAL;       // select port by message class
        ip_addr_t ipaddr = (DM == PTP_DM_E2E) ? PTP_IGMP_PRIMARY : PTP_IGMP_PEER_DELAY; // select destination IP-address by delmech.
        udp_sendto_arg(conn, pMsg->data, pMsg->size, ipaddr, port, (uint32_t)pMsg);     // send packet
    } else if (TP == PTP_TP_802_3) {
        const uint8_t *ethaddr = (DM == PTP_DM_E2E) ? PTP_ETHERNET_PRIMARY : PTP_ETHERNET_PEER_DELAY; // select destination address by delmech.
        cet_send_arg(PTP_L2, ethaddr, pMsg->data, pMsg->size, (uint32_t)pMsg);                        // send frame
    }
}

void ptp_nsd_get_interface_address(uint8_t * hwa) {
    memcpy(hwa, get_default_interface()->mac, ETH_HW_ADDR_LEN);
}