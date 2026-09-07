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
static cbd PTP_L4_PRIMARY_EVENT = 0;
static cbd PTP_L4_PRIMARY_GENERAL = 0;
static cbd PTP_L4_PDELAY_EVENT = 0;
static cbd PTP_L4_PDELAY_GENERAL = 0;
static cbd PTP_L2 = 0;

// store current settings
static PtpTransportType TP = -1;
static PtpDelayMechanism DM = -1;
static bool custom_p2p_8023_primary_dest_valid = false;
static uint8_t custom_p2p_8023_primary_dest[6] = {};
static bool custom_p2p_8023_pdel_dest_valid = false;
static uint8_t custom_p2p_8023_pdel_dest[6] = {};

static const uint8_t zero_mac[6] = {};

static void ptp_transmit_cb(uint32_t ts_s, uint32_t ts_ns, uint32_t tag);
static int ptp_receive_cb(const Pckt *packet, PcktSieveLayerTag tag);

void ptp_nsd_igmp_join_leave(bool join) {
    // only join IGMP if Transport Type is IP
    if (TP == PTP_TP_IPv4) {
        ConnBlock cb = igmp_new_connblock(get_default_interface()); // open IGMP

        void (*igmp_fn)(ConnBlock *, ip4_addr) = join ? igmp_report_membership : igmp_leave_group; // join or leave

        igmp_fn(&cb, PTP_IGMP_PRIMARY); // join/leave PRIMARY messaging group group

        if (DM == PTP_DM_P2P) {                // with P2P delay mechanism...
            igmp_fn(&cb, PTP_IGMP_PEER_DELAY); // join PDELAY* message group
        }

        connb_remove(&cb); // close IGMP
    }
}

#define CLOSE_CONNECTION_IF_EXISTS(d) \
    if (d >= CBD_LOWEST_DESCRIPTOR) { \
        close_connection(d);          \
        d = 0;                        \
    }

void ptp_nsd_init(const NsdInitSettings *init) {
    // leave current IGMP group if applicable
    ptp_nsd_igmp_join_leave(false);

    // first, close all open connection blocks (zero CBDs won't cause trouble)
    CLOSE_CONNECTION_IF_EXISTS(PTP_L4_PRIMARY_EVENT);
    CLOSE_CONNECTION_IF_EXISTS(PTP_L4_PRIMARY_GENERAL);
    CLOSE_CONNECTION_IF_EXISTS(PTP_L4_PDELAY_EVENT);
    CLOSE_CONNECTION_IF_EXISTS(PTP_L4_PDELAY_GENERAL);
    CLOSE_CONNECTION_IF_EXISTS(PTP_L2);

    // calling either parameter with -1 just closes connections
    if ((init->tp == -1) || (init->dm == -1)) {
        // message transmission and reception is turned off
        TP = -1;
        DM = -1;
        return;
    }

    // open only the necessary ones
    EthInterface *intf = get_default_interface();

    switch (init->tp) {
    case PTP_TP_IPv4: {
        // open event and general PRIMARY connections
        PTP_L4_PRIMARY_EVENT = udp_new_connblock(intf, PTP_IGMP_PRIMARY, PTP_PORT_EVENT, ptp_receive_cb);
        PTP_L4_PRIMARY_GENERAL = udp_new_connblock(intf, PTP_IGMP_PRIMARY, PTP_PORT_GENERAL, ptp_receive_cb);

        // set transmit callbacks
        ts_set_tx_callback(PTP_L4_PRIMARY_EVENT, ptp_transmit_cb);
        ts_set_tx_callback(PTP_L4_PRIMARY_GENERAL, ptp_transmit_cb);

        // open event and general PDELAY* connections
        if (init->dm == PTP_DM_P2P) {
            PTP_L4_PDELAY_EVENT = udp_new_connblock(intf, PTP_IGMP_PEER_DELAY, PTP_PORT_EVENT, ptp_receive_cb);
            PTP_L4_PDELAY_GENERAL = udp_new_connblock(intf, PTP_IGMP_PEER_DELAY, PTP_PORT_GENERAL, ptp_receive_cb);

            // set transmit callbacks
            ts_set_tx_callback(PTP_L4_PDELAY_EVENT, ptp_transmit_cb);
            ts_set_tx_callback(PTP_L4_PDELAY_GENERAL, ptp_transmit_cb);
        }

    } break;
    case PTP_TP_802_3:
        PTP_L2 = cet_new_connblock(intf, ETHERTYPE_PTP, ptp_receive_cb); // open connection
        ts_set_tx_callback(PTP_L2, ptp_transmit_cb);                     // set transmit callback
        break;
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

    // store settings
    TP = init->tp;
    DM = init->dm;

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
    ptp_transmit_timestamp_cb(tag, ts_s, ts_ns);
}

void ptp_nsd_transmit_msg(RawPtpMessage *pMsg, uint32_t uid) {
    PtpMessageClass mc = pMsg->tx_mc;
    PtpMessageType mt = pMsg->tx_mt;

    // is it a Peer Delay Mechanism related message?
    bool isPDel_ = (mt == PTP_MT_PDelay_Req) || (mt == PTP_MT_PDelay_Resp) || (mt == PTP_MT_PDelay_Resp_Follow_Up);

    // narrow down by transport type
    if (TP == PTP_TP_IPv4) {
        cbd conn = (mc == PTP_MC_EVENT) ? PTP_L4_PRIMARY_EVENT : PTP_L4_PRIMARY_GENERAL; // select connection by message type
        uint16_t port = (mc == PTP_MC_EVENT) ? PTP_PORT_EVENT : PTP_PORT_GENERAL;        // select port by message class
        ip_addr_t ipaddr = isPDel_ ? PTP_IGMP_PEER_DELAY : PTP_IGMP_PRIMARY;             // select destination IP-address by PDel*/primary message types
        udp_sendto_arg(conn, pMsg->data, pMsg->size, ipaddr, port, uid);                 // send packet
    } else if (TP == PTP_TP_802_3) {
        const uint8_t *ethaddr = isPDel_ ? (custom_p2p_8023_pdel_dest_valid ? custom_p2p_8023_pdel_dest : PTP_ETHERNET_PEER_DELAY) : (custom_p2p_8023_primary_dest_valid ? custom_p2p_8023_primary_dest : PTP_ETHERNET_PRIMARY); // select destination address by PDel*/primary message types
        cet_send_arg(PTP_L2, ethaddr, pMsg->data, pMsg->size, uid);                                                                                                                                                              // send frame
    }
}

void ptp_nsd_get_interface_address(uint8_t *hwa) {
    memcpy(hwa, get_default_interface()->mac, ETH_HW_ADDR_LEN);
}