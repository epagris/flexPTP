#include <flexptp/ptp_core.h>
#include <flexptp/ptp_defs.h>
#include <flexptp/ptp_msg_tx.h>
#include <flexptp/ptp_raw_msg_circbuf.h>
#include <flexptp/settings_interface.h>
#include <flexptp/task_ptp.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <flexptp_options.h>

#ifdef LWIP
#include "lwip/igmp.h"
#elif defined(ETHLIB)
#include <etherlib/etherlib.h>
#endif

// ----- TASK PROPERTIES -----
static TaskHandle_t sTH;        // task handle
static uint8_t sPrio = 5;       // priority
static uint16_t sStkSize = 2048;        // stack size
void task_ptp(void *pParam);    // task routine function
// ---------------------------

static bool sPTP_operating = false;     // does the PTP subsystem operate?

// ---------------------------

// udp control blocks
#ifdef LWIP
static struct udp_pcb *spPTP_PRIMARY_EVENT_pcb = NULL;
static struct udp_pcb *spPTP_PRIMARY_GENERAL_pcb = NULL;

#elif defined(ETHLIB)
static cbd spPTP_PRIMARY_EVENT_pcb = 0;
static cbd spPTP_PRIMARY_GENERAL_pcb = 0;

#endif

// callback function receiveing data from udp "sockets"
#ifdef LWIP
void ptp_recv_cb(void *pArg, struct udp_pcb *pPCB, struct pbuf *pP, const ip_addr_t * pAddr, uint16_t port);
#elif defined(ETHLIB)
int ptp_recv_cb(const Pckt * packet, PcktSieveLayerTag tag);
#endif

// FIFO for incoming packets
#define RX_PACKET_FIFO_LENGTH (32)
#define TX_PACKET_FIFO_LENGTH (16)
static QueueHandle_t sRxPacketFIFO;
QueueHandle_t gTxPacketFIFO;
static QueueSetHandle_t sRxTxFIFOSet;

// create udp listeners
#ifdef LWIP
void create_ptp_listeners()
{
    // create packet FIFO
    sRxPacketFIFO = xQueueCreate(RX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    gTxPacketFIFO = xQueueCreate(TX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    sRxTxFIFOSet = xQueueCreateSet(RX_PACKET_FIFO_LENGTH + TX_PACKET_FIFO_LENGTH);
    xQueueAddToSet(sRxPacketFIFO, sRxTxFIFOSet);
    xQueueAddToSet(gTxPacketFIFO, sRxTxFIFOSet);

    // PRIMARY EVENT (...1.129:319)
    spPTP_PRIMARY_EVENT_pcb = udp_new();
    udp_bind(spPTP_PRIMARY_EVENT_pcb, &PTP_IGMP_PRIMARY, PTP_PORT_EVENT);
    udp_recv(spPTP_PRIMARY_EVENT_pcb, ptp_recv_cb, NULL);

    // PRIMARY GENERAL (...1.129:320)
    spPTP_PRIMARY_GENERAL_pcb = udp_new();
    udp_bind(spPTP_PRIMARY_GENERAL_pcb, &PTP_IGMP_PRIMARY, PTP_PORT_GENERAL);
    udp_recv(spPTP_PRIMARY_GENERAL_pcb, ptp_recv_cb, NULL);

}
#elif defined(ETHLIB)
void create_ptp_listeners()
{
    // create packet FIFO
    sRxPacketFIFO = xQueueCreate(RX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    gTxPacketFIFO = xQueueCreate(TX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    sRxTxFIFOSet = xQueueCreateSet(RX_PACKET_FIFO_LENGTH + TX_PACKET_FIFO_LENGTH);
    xQueueAddToSet(sRxPacketFIFO, sRxTxFIFOSet);
    xQueueAddToSet(gTxPacketFIFO, sRxTxFIFOSet);

    // PRIMARY EVENT (...1.129:319)
    EthInterface *intf = get_default_interface();
    spPTP_PRIMARY_EVENT_pcb = udp_new_connblock(intf, PTP_IGMP_PRIMARY, PTP_PORT_EVENT, ptp_recv_cb);

    // PRIMARY GENERAL (...1.129:320)
    spPTP_PRIMARY_GENERAL_pcb = udp_new_connblock(intf, PTP_IGMP_PRIMARY, PTP_PORT_GENERAL, ptp_recv_cb);

}
#endif

// remove listeners
#ifdef LWIP
void destroy_ptp_listeners()
{
    // disconnect UDP "sockets"
    udp_disconnect(spPTP_PRIMARY_EVENT_pcb);
    udp_disconnect(spPTP_PRIMARY_GENERAL_pcb);

    // destroy UDP sockets
    udp_remove(spPTP_PRIMARY_EVENT_pcb);
    udp_remove(spPTP_PRIMARY_GENERAL_pcb);

    // destroy packet FIFO
    xQueueRemoveFromSet(sRxPacketFIFO, sRxTxFIFOSet);
    xQueueRemoveFromSet(gTxPacketFIFO, sRxTxFIFOSet);
    vQueueDelete(sRxPacketFIFO);
    vQueueDelete(gTxPacketFIFO);
    vQueueDelete(sRxTxFIFOSet);
}
#elif defined(ETHLIB)
void destroy_ptp_listeners()
{
    // disconnect UDP "sockets"
    close_connection(spPTP_PRIMARY_EVENT_pcb);
    close_connection(spPTP_PRIMARY_GENERAL_pcb);

    // destroy packet FIFO
    xQueueRemoveFromSet(sRxPacketFIFO, sRxTxFIFOSet);
    xQueueRemoveFromSet(gTxPacketFIFO, sRxTxFIFOSet);
    vQueueDelete(sRxPacketFIFO);
    vQueueDelete(gTxPacketFIFO);
    vQueueDelete(sRxTxFIFOSet);
}
#endif

// join PTP IGMP groups
#ifdef LWIP
void join_ptp_igmp_groups()
{
    // join group for default set of messages (everything except for peer delay)
    igmp_joingroup(&netif_default->ip_addr, &PTP_IGMP_PRIMARY);

}
#elif defined(ETHLIB)
void join_ptp_igmp_groups()
{
    // open IGMP
    ConnBlock cb = igmp_new_connblock(get_default_interface());

    // join group for default set of messages (everything except for peer delay)
    igmp_report_membership(&cb, PTP_IGMP_PRIMARY);

    // close IGMP
    connb_remove(&cb);
}
#endif

// leave PTP IGMP group
#ifdef LWIP
void leave_ptp_igmp_groups()
{
    // leave default group
    igmp_leavegroup(&netif_default->ip_addr, &PTP_IGMP_PRIMARY);

}
#elif defined(ETHLIB)
void leave_ptp_igmp_groups()
{
    // open IGMP
    ConnBlock cb = igmp_new_connblock(get_default_interface());

    // join group for default set of messages (everything except for peer delay)
    igmp_leave_group(&cb, PTP_IGMP_PRIMARY);

    // join group of peer delay messages
    igmp_leave_group(&cb, PTP_IGMP_PEER_DELAY);

    // close IGMP
    connb_remove(&cb);
}
#endif

// "ring" buffer for PTP-messages
PtpCircBuf gRawRxMsgBuf, gRawTxMsgBuf;
static RawPtpMessage sRawRxMsgBufPool[RX_PACKET_FIFO_LENGTH];
static RawPtpMessage sRawTxMsgBufPool[TX_PACKET_FIFO_LENGTH];

static void init_raw_buffers()
{
    ptp_circ_buf_init(&gRawRxMsgBuf, sRawRxMsgBufPool, RX_PACKET_FIFO_LENGTH);
    ptp_circ_buf_init(&gRawTxMsgBuf, sRawTxMsgBufPool, TX_PACKET_FIFO_LENGTH);
}

// register PTP task and initialize
void reg_task_ptp()
{
    init_raw_buffers();         // initialize raw buffers

    // initialize PTP subsystem
#ifdef LWIP
    ptp_init(netif_default->hwaddr);
#elif defined(ETHLIB)
    ptp_init(E.ethIntf->mac);
#endif

#ifdef PTP_CONFIG_PTR           // load config if provided
    MSG("Loading PTP-config!\n");
    ptp_load_config_from_dump(PTP_CONFIG_PTR());
#endif

    // create UDP sockets regardless the transfer type
    join_ptp_igmp_groups();     // enter PTP IGMP groups
    create_ptp_listeners();     // create listeners
#ifndef SIMULATION
#ifdef LWIP
    ptp_transmit_init(spPTP_PRIMARY_EVENT_pcb, spPTP_PRIMARY_GENERAL_pcb);      // initialize transmit function
#elif defined(ETHLIB)
    ptp_transmit_init(spPTP_PRIMARY_EVENT_pcb, spPTP_PRIMARY_GENERAL_pcb);      // initialize transmit function
#endif
#endif

    // create task
    BaseType_t result = xTaskCreate(task_ptp, "ptp", sStkSize, NULL, sPrio, &sTH);
    if (result != pdPASS) {
        MSG("Failed to create PTP task! (errcode: %d)\n", result);
        unreg_task_ptp();
        return;
    }

    sPTP_operating = true;      // the PTP subsystem is operating
}

// unregister PTP task
void unreg_task_ptp()
{
    vTaskDelete(sTH);           // taszk törlése

    ptp_deinit();               // ptp subsystem de-initialization

    // destroy listeners
    leave_ptp_igmp_groups();    // leave IGMP groups
    destroy_ptp_listeners();    // delete listeners

    sPTP_operating = false;     // the PTP subsystem is operating
}

// callback for packet reception on port 319 and 320
#ifdef LWIP
void ptp_recv_cb(void *pArg, struct udp_pcb *pPCB, struct pbuf *pP, const ip_addr_t * pAddr, uint16_t port)
{
    // put msg into the queue
    ptp_enqueue_msg(pP->payload, pP->len, pP->time_s, pP->time_ns, PTP_TP_IPv4);

    // release pbuf resources
    pbuf_free(pP);
}
#elif defined(ETHLIB)
int ptp_recv_cb(const Pckt * packet, PcktSieveLayerTag tag)
{
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
        ptp_enqueue_msg(packet->payload, packet->payloadSize, packet->time_s, packet->time_ns, tp);
    } else {
        MSG("Unknown PTP packet class: '%d'!\n", tp);
    }

    return 0;
}
#endif

// put ptp message onto processing queue
void ptp_enqueue_msg(void *pPayload, uint32_t len, uint32_t ts_sec, uint32_t ts_ns, int tp)
{
    // only consider messages received on the matching transport layer
    if (tp != ptp_get_transport_type()) {
        return;
    }

    // enqueue message
    RawPtpMessage *pMsg = ptp_circ_buf_alloc(&gRawRxMsgBuf);
    if (pMsg) {
        // copy payload and timestamp
        uint32_t copyLen = MIN(len, MAX_PTP_MSG_SIZE);
        memcpy(pMsg->data, pPayload, copyLen);
        pMsg->size = copyLen;
        pMsg->ts.sec = ts_sec;
        pMsg->ts.nanosec = ts_ns;
        pMsg->pTs = NULL;
        pMsg->pTxCb = NULL;     // not meaningful...

        uint8_t idx = ptp_circ_buf_commit(&gRawRxMsgBuf);

        xQueueSend(sRxPacketFIFO, &idx, portMAX_DELAY); // send index
    } else {
        MSG("PTP-packet buffer full, a packet has been dropped!\n");
    }

    // MSG("TS: %u.%09u\n", (uint32_t)ts_sec, (uint32_t)ts_ns);

    // if the transport layer matches...
}

// taszk függvénye
void task_ptp(void *pParam)
{
    while (1) {
        // wait for received packet or packet to transfer
        QueueHandle_t activeQueue = xQueueSelectFromSet(sRxTxFIFOSet, pdMS_TO_TICKS(200));

        // if packet is on the RX queue
        if (activeQueue == sRxPacketFIFO) {
            // pop packet from FIFO
            uint8_t bufIdx;
            xQueueReceive(sRxPacketFIFO, &bufIdx, portMAX_DELAY);

            // fetch buffer
            RawPtpMessage *pRawMsg = ptp_circ_buf_get(&gRawRxMsgBuf, bufIdx);
            pRawMsg->pTs = &pRawMsg->ts;

            // process packet
            ptp_process_packet(pRawMsg);

            // free buffer
            ptp_circ_buf_free(&gRawRxMsgBuf);
        } else if (activeQueue == gTxPacketFIFO) {
            // pop packet from FIFO
            uint8_t bufIdx;
            xQueueReceive(gTxPacketFIFO, &bufIdx, portMAX_DELAY);

            // fetch buffer
            RawPtpMessage *pRawMsg = ptp_circ_buf_get(&gRawTxMsgBuf, bufIdx);
            ptp_transmit_msg(pRawMsg);
        } else {
            // ....
        }
    }
}

// --------------------------

// function to query PTP operation state
bool task_ptp_is_operating()
{
    return sPTP_operating;
}
