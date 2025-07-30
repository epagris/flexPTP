#include "task_ptp.h"

#include <string.h>

#include "config.h"
#include "event.h"
#include "network_stack_driver.h"
#include "profiles.h"
#include "ptp_core.h"
#include "ptp_types.h"
#include "ptp_defs.h"
#include "ptp_raw_msg_circbuf.h"
#include "settings_interface.h"

#include <flexptp_options.h>

#include "minmax.h"

///\cond 0
// ----- TASK PROPERTIES -----
#ifdef FLEXPTP_FREERTOS
static TaskHandle_t sTH; // task handle in direct FreeRTOS mode
#elif defined(FLEXPTP_CMSIS_OS2)
static osThreadId_t sTH; // task handle in CMSIS OS2 mode
#endif
static uint8_t sPrio = FLEXPTP_TASK_PRIORITY; // priority
static uint16_t sStkSize = 2048;              // stack size
static void task_ptp(void *pParam);           // task routine function
// ---------------------------

static bool sPTP_operating = false; // does the PTP subsystem operate?

// ---------------------------

///\endcond

// FIFO for incoming packets
#define RX_PACKET_FIFO_LENGTH (32)    ///< Receive packet FIFO length
#define TX_PACKET_FIFO_LENGTH (16)    ///< Transmit packet FIFO length
#define EVENT_FIFO_LENGTH (32)        ///< Event FIFO length
#define NOTIFICATION_FIFO_LENGTH (16) ///< Notification FIFO length

/**
 * Notifications for the processing thread.
 */
typedef enum {
    PTN_RECEIVE,  ///< A message has been received
    PTN_TRANSMIT, ///< A message waits transmission
    PTN_EVENT     ///< An event has occurred
} ProcThreadNotification;

///\cond 0
// queues for message reception and transmission
#ifdef FLEXPTP_FREERTOS
static QueueHandle_t sRxPacketFIFO, sEventFIFO;
static QueueHandle_t gTxPacketFIFO;
static QueueHandle_t sNotificationFIFO;
#elif defined(FLEXPTP_CMSIS_OS2)
static osMessageQueueId_t sRxPacketFIFO, sEventFIFO;
static osMessageQueueId_t gTxPacketFIFO;
static osMessageQueueId_t sNotificationFIFO;
#endif

// "ring" buffer for PTP-messages
PtpCircBuf gRawRxMsgBuf, gRawTxMsgBuf;
static RawPtpMessage sRawRxMsgBufPool[RX_PACKET_FIFO_LENGTH];
static RawPtpMessage sRawTxMsgBufPool[TX_PACKET_FIFO_LENGTH];
///\endcond

// create message queues
static void ptp_create_message_queues() {
    // create packet FIFO
#ifdef FLEXPTP_FREERTOS
    sRxPacketFIFO = xQueueCreate(RX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    gTxPacketFIFO = xQueueCreate(TX_PACKET_FIFO_LENGTH, sizeof(uint8_t));
    sEventFIFO = xQueueCreate(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent));
    sNotificationFIFO = xQueueCreate(NOTIFICATION_FIFO_LENGTH, sizeof(ProcThreadNotification));
#elif defined(FLEXPTP_CMSIS_OS2)
    sRxPacketFIFO = osMessageQueueNew(RX_PACKET_FIFO_LENGTH, sizeof(uint8_t), NULL);
    gTxPacketFIFO = osMessageQueueNew(TX_PACKET_FIFO_LENGTH, sizeof(uint8_t), NULL);
    sEventFIFO = osMessageQueueNew(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent), NULL);
    sNotificationFIFO = osMessageQueueNew(NOTIFICATION_FIFO_LENGTH, sizeof(ProcThreadNotification), NULL);
#endif

    // initalize packet buffers
    ptp_circ_buf_init(&gRawRxMsgBuf, sRawRxMsgBufPool, RX_PACKET_FIFO_LENGTH);
    ptp_circ_buf_init(&gRawTxMsgBuf, sRawTxMsgBufPool, TX_PACKET_FIFO_LENGTH);
}

// destroy message queues
static void ptp_destroy_message_queues() {
    // destroy packet FIFO
#ifdef FLEXPTP_FREERTOS
    vQueueDelete(sRxPacketFIFO);
    vQueueDelete(gTxPacketFIFO);
    vQueueDelete(sEventFIFO);
    vQueueDelete(sNotificationFIFO);
#elif defined(FLEXPTP_CMSIS_OS2)
    osMessageQueueDelete(sRxPacketFIFO);
    osMessageQueueDelete(gTxPacketFIFO);
    osMessageQueueDelete(sEventFIFO);
    osMessageQueueDelete(sNotificationFIFO);
#endif

    // packet buffers cannot be released since nothing had been allocated for them
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
    sTH = NULL;
#ifdef FLEXPTP_FREERTOS
    BaseType_t result = xTaskCreate(task_ptp, "ptp", sStkSize / sizeof(BaseType_t), NULL, sPrio, &sTH);
    if (result != pdPASS) {
        MSG("Failed to create PTP task! (errcode: %d)\n", result);
        unreg_task_ptp();
        return;
    }
#elif defined(FLEXPTP_CMSIS_OS2)
    osThreadAttr_t attr;
    memset(&attr, 0, sizeof(osThreadAttr_t));
    attr.name = "ptp";
    attr.stack_size = sStkSize;
    attr.priority = sPrio;
    sTH = osThreadNew(task_ptp, NULL, &attr);
    if (sTH == NULL) {
        MSG("Failed to create PTP task!\n");
        unreg_task_ptp();
        return;
    }
#endif

    sPTP_operating = true; // the PTP subsystem is operating
}

// unregister PTP task
void unreg_task_ptp() {
    ptp_nsd_init(-1, -1); // de-initialize the network stack driver
    if (sTH != NULL) {
#ifdef FLEXPTP_FREERTOS
        vTaskDelete(sTH); // delete task
#elif defined(FLEXPTP_CMSIS_OS2)
        osThreadTerminate(sTH);
#endif
    }
    sTH = NULL;
    ptp_deinit();                 // ptp subsystem de-initialization
    ptp_destroy_message_queues(); // destroy message queues and buffers
    sPTP_operating = false;       // the PTP subsystem is NOT operating anymore
}

bool ptp_event_enqueue(const PtpCoreEvent *event) {
    ProcThreadNotification notif = PTN_EVENT;

    bool ok;
#ifdef FLEXPTP_FREERTOS
    ok = xQueueSend(sEventFIFO, event, portMAX_DELAY) == pdPASS;
    if (ok) {
        xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY);
    }
#elif defined(FLEXPTP_CMSIS_OS2)
    ok = osMessageQueuePut(sEventFIFO, &notif, 0, osWaitForever) == osOK;
#endif
    return ok;
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

        ProcThreadNotification notif = PTN_RECEIVE;
#ifdef FLEXPTP_FREERTOS
        xQueueSend(sRxPacketFIFO, &idx, portMAX_DELAY);       // send index
        xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY); // send notification
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueuePut(sRxPacketFIFO, &idx, 0, osWaitForever);
        osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#endif
    } else {
        MSG("The PTP receive packet buffer is full, a packet was lost!\n");
    }

    // MSG("TS: %u.%09u\n", (uint32_t)ts_sec, (uint32_t)ts_ns);

    // if the transport layer matches...
}

bool ptp_transmit_enqueue(const RawPtpMessage *pMsg) {
    RawPtpMessage *pMsgAlloc = ptp_circ_buf_alloc(&gRawTxMsgBuf);
    if (pMsgAlloc) {
        *pMsgAlloc = *pMsg;
        uint8_t idx = ptp_circ_buf_commit(&gRawTxMsgBuf);
        ProcThreadNotification notif = PTN_TRANSMIT;
#ifdef FLEXPTP_FREERTOS
        BaseType_t hptWoken = false;
        if (xPortIsInsideInterrupt()) {
            xQueueSendFromISR(gTxPacketFIFO, &idx, (BaseType_t *)&hptWoken);
            xQueueSendFromISR(sNotificationFIFO, &notif, (BaseType_t *)&hptWoken);
        } else {
            xQueueSend(gTxPacketFIFO, &idx, portMAX_DELAY);
            xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY);
        }
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueuePut(gTxPacketFIFO, &idx, 0, osWaitForever);
        osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#endif
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
        ProcThreadNotification notification;
#ifdef FLEXPTP_FREERTOS
        xQueueReceive(sNotificationFIFO, &notification, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueueGet(sNotificationFIFO, &notification, NULL, osWaitForever);
#endif
        // if packet is on the RX queue
        if (notification == PTN_RECEIVE) { /* ---- RECIEVE ----- */
            // pop packet from FIFO
            uint8_t bufIdx;

#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sRxPacketFIFO, &bufIdx, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sRxPacketFIFO, &bufIdx, NULL, osWaitForever);
#endif

            // fetch buffer
            RawPtpMessage *pRawMsg = ptp_circ_buf_get(&gRawRxMsgBuf, bufIdx);
            pRawMsg->pTs = &pRawMsg->ts;

            // process packet
            ptp_process_packet(pRawMsg);

            // free buffer
            ptp_circ_buf_free(&gRawRxMsgBuf);
        } else if (notification == PTN_TRANSMIT) { /* ---- TRANSMIT ----- */
            // pop packet from FIFO
            uint8_t bufIdx;
#ifdef FLEXPTP_FREERTOS
            xQueueReceive(gTxPacketFIFO, &bufIdx, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(gTxPacketFIFO, &bufIdx, NULL, osWaitForever);
#endif
            // fetch buffer
            RawPtpMessage *pRawMsg = ptp_circ_buf_get(&gRawTxMsgBuf, bufIdx);
            ptp_nsd_transmit_msg(pRawMsg);
        } else if (notification == PTN_EVENT) { /* ---- EVENT ----- */
            // pop event from the FIFO
            PtpCoreEvent event;
#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sEventFIFO, &event, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sEventFIFO, &event, NULL, osWaitForever);
#endif
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
