#include "task_ptp.h"

#include <string.h>

#include "config.h"
#include "event.h"
#include "network_stack_driver.h"
#include "portmacro.h"
#include "profiles.h"
#include "projdefs.h"
#include "ptp_core.h"
#include "ptp_raw_msg_circbuf.h"
#include "settings_interface.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <flexptp_options.h>

// provide own MIN implementation
#ifdef MIN
#undef MIN
#endif 

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

///\cond 0
// ----- TASK PROPERTIES -----
static TaskHandle_t sTH;                      // task handle
static uint8_t sPrio = FLEXPTP_TASK_PRIORITY; // priority
static uint16_t sStkSize = 2048;              // stack size
static void task_ptp(void *pParam);           // task routine function
// ---------------------------

static bool sPTP_operating = false; // does the PTP subsystem operate?

// ---------------------------

///\endcond

// FIFO for incoming packets
#define RX_PACKET_FIFO_LENGTH (32) ///< Receive packet FIFO length
#define TX_PACKET_FIFO_LENGTH (16) ///< Transmit packet FIFO length
#define EVENT_FIFO_LENGTH (32)     ///< Event FIFO length

///\cond 0
// queues for message reception and transmission
static QueueHandle_t sRxPacketFIFO, sEventFIFO;
QueueHandle_t gTxPacketFIFO;
static QueueSetHandle_t sRxTxEvFIFOSet;

// "ring" buffer for PTP-messages
PtpCircBuf gRawRxMsgBuf, gRawTxMsgBuf;
static RawPtpMessage sRawRxMsgBufPool[RX_PACKET_FIFO_LENGTH];
static RawPtpMessage sRawTxMsgBufPool[TX_PACKET_FIFO_LENGTH];
///\endcond

// create message queues
static void ptp_create_message_queues() {
    // create packet FIFO
    sRxPacketFIFO = xQueueCreate(RX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    gTxPacketFIFO = xQueueCreate(TX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    sEventFIFO = xQueueCreate(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent));
    sRxTxEvFIFOSet = xQueueCreateSet(RX_PACKET_FIFO_LENGTH + TX_PACKET_FIFO_LENGTH);
    xQueueAddToSet(sRxPacketFIFO, sRxTxEvFIFOSet);
    xQueueAddToSet(gTxPacketFIFO, sRxTxEvFIFOSet);
    xQueueAddToSet(sEventFIFO, sRxTxEvFIFOSet);

    // initalize packet buffers
    ptp_circ_buf_init(&gRawRxMsgBuf, sRawRxMsgBufPool, RX_PACKET_FIFO_LENGTH);
    ptp_circ_buf_init(&gRawTxMsgBuf, sRawTxMsgBufPool, TX_PACKET_FIFO_LENGTH);
}

// destroy message queues
static void ptp_destroy_message_queues() {
    // destroy packet FIFO
    xQueueRemoveFromSet(sRxPacketFIFO, sRxTxEvFIFOSet);
    xQueueRemoveFromSet(gTxPacketFIFO, sRxTxEvFIFOSet);
    xQueueRemoveFromSet(sEventFIFO, sRxTxEvFIFOSet);
    vQueueDelete(sRxPacketFIFO);
    vQueueDelete(gTxPacketFIFO);
    vQueueDelete(sEventFIFO);
    vQueueDelete(sRxTxEvFIFOSet);

    // packet buffers cannot be released since nothing has been allocated for them
}

// register PTP task and initialize
void reg_task_ptp() {
    // initialize message queues and buffers
    ptp_create_message_queues();

#ifdef PTP_USER_EVENT_CALLBACK
    ptp_set_user_event_callback(PTP_USER_EVENT_CALLBACK);
#endif

    // initialize PTP subsystem
    uint8_t hwa[6];
    ptp_nsd_get_interface_address(hwa);
    ptp_init(hwa);

#ifdef PTP_CONFIG_PTR // load config if provided
    MSG("Loading PTP-config!\n");
    ptp_load_config_from_dump(PTP_CONFIG_PTR());

    // print profile summary
    MSG("\n\n----\n");
    ptp_print_profile();
    MSG("----\n\n");
#endif

    // initialize network stack driver
    ptp_nsd_init(ptp_get_transport_type(), ptp_get_delay_mechanism());

    // create task
    BaseType_t result = xTaskCreate(task_ptp, "ptp", sStkSize / 4, NULL, sPrio, &sTH);
    if (result != pdPASS) {
        MSG("Failed to create PTP task! (errcode: %d)\n", result);
        unreg_task_ptp();
        return;
    }

    sPTP_operating = true; // the PTP subsystem is operating
}

// unregister PTP task
void unreg_task_ptp() {
    ptp_nsd_init(-1, -1);         // de-initialize the network stack driver
    vTaskDelete(sTH);             // delete task
    ptp_deinit();                 // ptp subsystem de-initialization
    ptp_destroy_message_queues(); // destroy message queues and buffers
    sPTP_operating = false;       // the PTP subsystem is NOT operating anymore
}

bool ptp_event_enqueue(const PtpCoreEvent * event) {
    return xQueueSend(sEventFIFO, event, portMAX_DELAY) == pdPASS;
}

// put ptp message onto processing queue
void ptp_receive_enqueue(const void *pPayload, uint32_t len, uint32_t ts_sec, uint32_t ts_ns, int tp) {
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
        pMsg->pTxCb = NULL; // not meaningful...

        uint8_t idx = ptp_circ_buf_commit(&gRawRxMsgBuf);

        xQueueSend(sRxPacketFIFO, &idx, portMAX_DELAY); // send index
    } else {
        MSG("PTP-packet buffer full, a packet has been dropped!\n");
    }

    // MSG("TS: %u.%09u\n", (uint32_t)ts_sec, (uint32_t)ts_ns);

    // if the transport layer matches...
}

bool ptp_transmit_enqueue(const RawPtpMessage *pMsg) {
    extern PtpCircBuf gRawTxMsgBuf;
    extern QueueHandle_t gTxPacketFIFO;
    RawPtpMessage *pMsgAlloc = ptp_circ_buf_alloc(&gRawTxMsgBuf);
    if (pMsgAlloc) {
        *pMsgAlloc = *pMsg;
        uint8_t idx = ptp_circ_buf_commit(&gRawTxMsgBuf);
        BaseType_t hptWoken = false;
        if (xPortIsInsideInterrupt()) {
            xQueueSendFromISR(gTxPacketFIFO, &idx, (BaseType_t *)&hptWoken);
        } else {
            xQueueSend(gTxPacketFIFO, &idx, portMAX_DELAY);
        }
        return true;
    } else {
        MSG("PTP TX Enqueue failed!\n");
        PTP_IUEV(PTP_UEV_QUEUE_ERROR); // dispatch QUEUE_ERROR event
        return false;
    }
}

// task routine
static void task_ptp(void *pParam) {
    while (1) {
        // wait for received packet or packet to transfer
        QueueHandle_t activeQueue = xQueueSelectFromSet(sRxTxEvFIFOSet, pdMS_TO_TICKS(200));

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
            ptp_nsd_transmit_msg(pRawMsg);
        } else if (activeQueue == sEventFIFO) {
            // pop event from the FIFO
            PtpCoreEvent event;
            xQueueReceive(sEventFIFO, &event, portMAX_DELAY);

            // process event
            ptp_process_event(&event);
        } else {
            // ....
        }
    }
}

// --------------------------

// function to query PTP operation state
bool task_ptp_is_operating() {
    return sPTP_operating;
}

// --------------------------

