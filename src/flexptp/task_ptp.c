#include "task_ptp.h"

#include <string.h>

#include "config.h"
#include "event.h"
#include "msg_buf.h"
#include "network_stack_driver.h"
#include "profiles.h"
#include "ptp_core.h"
#include "ptp_defs.h"
#include "ptp_types.h"
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

#define S (gPtpCoreState)

///\endcond

// ---------------------------

// FIFO for incoming packets
#define RX_PACKET_FIFO_LENGTH (32)    ///< Receive packet FIFO length
#define TX_PACKET_FIFO_LENGTH (16)    ///< Transmit packet FIFO length
#define EVENT_FIFO_LENGTH (32)        ///< Event FIFO length
#define NOTIFICATION_FIFO_LENGTH (16) ///< Notification FIFO length
#define TX_CALLBACK_FIFO_LENGTH (10)  ///< Transmit callback FIFO length

#define TX_TTL_MS (2000) ///< TTL for outbound packets
#define RX_TTL_MS (2000) ///< TTL for inbound packets

// -----------------------------

/**
 * Notifications for the processing thread.
 */
typedef enum {
    PTN_RECEIVE,       ///< A message has been received
    PTN_TRANSMIT,      ///< A message awaits transmission
    PTN_TRANSMIT_DONE, ///< A transmit timestamp awaits delegation
    PTN_EVENT          ///< An event has occurred
} ProcThreadNotification;

// -----------------------------

///\cond 0

// hearbeat timer
#ifdef FLEXPTP_FREERTOS
static TimerHandle_t sHeartBeatTmr;
#elif defined(FLEXPTP_CMSIS_OS2)
static osTimerId_t sHeartBeatTmr;
#endif

// queues for message reception and transmission
#ifdef FLEXPTP_FREERTOS
static QueueHandle_t sRxPacketFIFO, sEventFIFO;
static QueueHandle_t sTxPacketFIFO;
static QueueHandle_t sNotificationFIFO;
static QueueHandle_t sTxCbFIFO;
#elif defined(FLEXPTP_CMSIS_OS2)
static osMessageQueueId_t sRxPacketFIFO, sEventFIFO;
static osMessageQueueId_t gTxPacketFIFO;
static osMessageQueueId_t sNotificationFIFO;
static osMessageQueueId_t sTxCbFIFO;
#endif

// buffer for PTP-messages
static PtpMsgBuf sRawRxMsgBuf, sRawTxMsgBuf;
static PtpMsgBufBlock sRawRxMsgBufPool[RX_PACKET_FIFO_LENGTH];
static PtpMsgBufBlock sRawTxMsgBufPool[TX_PACKET_FIFO_LENGTH];
///\endcond

// ----------------------------

/**
 * Heartbeat callback.
 *
 * @param timer timer handle
 */
static void ptp_heartbeat_tmr_cb(
#ifdef FLEXPTP_FREERTOS
    TimerHandle_t timer
#elif defined(FLEXPTP_CMSIS_OS2)
    void *arg
#endif
) {
    PtpCoreEvent event = {.code = PTP_CEV_HEARTBEAT, .w = 0, .dw = 0};
    ptp_event_enqueue(&event);
}

/**
 * Construct the heartbeat timer.
 */
static void ptp_create_heartbeat_tmr() {
// create smbc timer
#if FLEXPTP_FREERTOS
    sHeartBeatTmr = xTimerCreate("ptp_heartbeat", pdMS_TO_TICKS(PTP_HEARTBEAT_TICKRATE_MS), // timeout
                                 true,                                                      // timer operates in repeat mode
                                 NULL,                                                      // ID
                                 ptp_heartbeat_tmr_cb);                                     // callback-function
#elif defined(FLEXPTP_CMSIS_OS2)
    sHeartBeatTmr = osTimerNew(ptp_heartbeat_tmr_cb, osTimerPeriodic, NULL, NULL);
#endif
    if (sHeartBeatTmr == NULL) {
        MSG("Failed to create the PTP heartbeat timer!\n");
    }
}

/**
 * Remove the heartbeat timer.
 */
static void ptp_remove_heartbeat_tmr() {
#ifdef FLEXPTP_FREERTOS
    xTimerStop(sHeartBeatTmr, 0);
    xTimerDelete(sHeartBeatTmr, 0);
#elif defined(FLEXPTP_CMSIS_OS2)
    osTimerStop(sHeartBeatTmr);
    osTimerDelete(sHeartBeatTmr);
#endif
    sHeartBeatTmr = NULL;
}

void ptp_start_heartbeat_tmr() {
#ifdef FLEXPTP_FREERTOS
    xTimerStart(sHeartBeatTmr, 0);
#elif defined(FLEXPTP_CMSIS_OS2)
    osTimerStart(sHeartBeatTmr, (PTP_HEARTBEAT_TICKRATE_MS * 1000) / osKernelGetTickFreq());
#endif
}

void ptp_stop_heartbeat_tmr() {
#ifdef FLEXPTP_FREERTOS
    xTimerStop(sHeartBeatTmr, 0);
#elif defined(FLEXPTP_CMSIS_OS2)
    osTimerStop(sHeartBeatTmr);
#endif
}

// ----------------------------

// create message queues
static void ptp_create_message_queues() {
    // create packet FIFO
#ifdef FLEXPTP_FREERTOS
    sRxPacketFIFO = xQueueCreate(RX_PACKET_FIFO_LENGTH, sizeof(RawPtpMessage *));
    sTxPacketFIFO = xQueueCreate(TX_PACKET_FIFO_LENGTH, sizeof(RawPtpMessage *));
    sEventFIFO = xQueueCreate(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent));
    sNotificationFIFO = xQueueCreate(NOTIFICATION_FIFO_LENGTH, sizeof(ProcThreadNotification));
    sTxCbFIFO = xQueueCreate(TX_CALLBACK_FIFO_LENGTH, sizeof(RawPtpMessage *));
#elif defined(FLEXPTP_CMSIS_OS2)
    sRxPacketFIFO = osMessageQueueNew(RX_PACKET_FIFO_LENGTH, sizeof(uint8_t), NULL);
    gTxPacketFIFO = osMessageQueueNew(TX_PACKET_FIFO_LENGTH, sizeof(uint8_t), NULL);
    sEventFIFO = osMessageQueueNew(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent), NULL);
    sNotificationFIFO = osMessageQueueNew(NOTIFICATION_FIFO_LENGTH, sizeof(ProcThreadNotification), NULL);
    sTxCbFIFO = osMessageQueueNew(TX_CALLBACK_FIFO_LENGTH, sizeof(RawPtpMessage *), NULL);
#endif

    // initalize packet buffers
    msgb_init(&sRawRxMsgBuf, sRawRxMsgBufPool, RX_PACKET_FIFO_LENGTH);
    msgb_init(&sRawTxMsgBuf, sRawTxMsgBufPool, TX_PACKET_FIFO_LENGTH);
}

// destroy message queues
static void ptp_destroy_message_queues() {
    // destroy packet FIFO
#ifdef FLEXPTP_FREERTOS
    vQueueDelete(sRxPacketFIFO);
    vQueueDelete(sTxPacketFIFO);
    vQueueDelete(sEventFIFO);
    vQueueDelete(sNotificationFIFO);
    vQueueDelete(sTxCbFIFO);
#elif defined(FLEXPTP_CMSIS_OS2)
    osMessageQueueDelete(sRxPacketFIFO);
    osMessageQueueDelete(gTxPacketFIFO);
    osMessageQueueDelete(sEventFIFO);
    osMessageQueueDelete(sNotificationFIFO);
    osMessageQueueDelete(sTxCbFIFO);
#endif

    // packet buffers cannot be released since nothing had been allocated for them
}

// register PTP task and initialize
void reg_task_ptp() {
    // initialize message queues and buffers
    ptp_create_message_queues();

    // set user event callback
#ifdef PTP_USER_EVENT_CALLBACK
    ptp_set_user_event_callback(PTP_USER_EVENT_CALLBACK);
#endif

    // create heartbeat timer
    ptp_create_heartbeat_tmr();

    // initialize PTP subsystem
    uint8_t hwa[6];
    ptp_nsd_get_interface_address(hwa);
    ptp_init(hwa);

    // load config if provided
#ifdef PTP_CONFIG_PTR 
    MSG("Loading the PTP-configuration!\n");
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
        MSG("Failed to create the PTP task! (errcode: %d)\n", result);
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
        MSG("Failed to create the PTP task!\n");
        unreg_task_ptp();
        return;
    }
#endif

    // the PTP subsystem is operating
    sPTP_operating = true; 
}

// unregister PTP task
void unreg_task_ptp() {
    ptp_remove_heartbeat_tmr(); // remove the heartbeat timer
    ptp_nsd_init(-1, -1);       // de-initialize the network stack driver
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

// ---------------------------

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
    RawPtpMessage *pMsg = msgb_alloc(&sRawRxMsgBuf, RPMT_RANDOM, FLEXPTP_MS_TO_TICKS(RX_TTL_MS));
    if (pMsg) {
        // copy payload and timestamp
        uint32_t copyLen = MIN(len, MAX_PTP_MSG_SIZE);
        memcpy(pMsg->data, pPayload, copyLen);
        pMsg->size = copyLen;
        pMsg->ts.sec = ts_sec;
        pMsg->ts.nanosec = ts_ns;
        pMsg->tag = RPMT_RANDOM;
        pMsg->pTxCb = NULL; // not meaningful...

        msgb_commit(&sRawRxMsgBuf, pMsg);

        ProcThreadNotification notif = PTN_RECEIVE;
#ifdef FLEXPTP_FREERTOS
        xQueueSend(sRxPacketFIFO, &pMsg, portMAX_DELAY);      // send index
        xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY); // send notification
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueuePut(sRxPacketFIFO, &idx, 0, osWaitForever);
        osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#endif
    } else {
        CLILOG(S.logging.info, "The PTP receive packet buffer is full, a packet was lost!\n");
    }
}

bool ptp_transmit_enqueue(const RawPtpMessage *pMsg) {
    RawPtpMessage *pMsgAlloc = msgb_alloc(&sRawTxMsgBuf, pMsg->tag, FLEXPTP_MS_TO_TICKS(TX_TTL_MS));
    if (pMsgAlloc) {
        memcpy(pMsgAlloc, pMsg, sizeof(RawPtpMessage));
        msgb_commit(&sRawTxMsgBuf, pMsgAlloc);

        ProcThreadNotification notif = PTN_TRANSMIT;
#ifdef FLEXPTP_FREERTOS
        BaseType_t hptWoken = false;
        if (xPortIsInsideInterrupt()) {
            xQueueSendFromISR(sTxPacketFIFO, &pMsgAlloc, &hptWoken);
            xQueueSendFromISR(sNotificationFIFO, &notif, &hptWoken);
        } else {
            xQueueSend(sTxPacketFIFO, &pMsgAlloc, portMAX_DELAY);
            xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY);
        }
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueuePut(sTxPacketFIFO, &idx, 0, osWaitForever);
        osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#endif
        return true;
    } else {
        CLILOG(S.logging.info, "PTP TX Enqueue failed!\n");
        PTP_IUEV(PTP_UEV_QUEUE_ERROR); // dispatch QUEUE_ERROR event
        return false;
    }
}

void ptp_transmit_timestamp_cb(RawPtpMessage *pMsg, uint32_t seconds, uint32_t nanoseconds) {
    // write back timestamp
    pMsg->ts.sec = seconds;
    pMsg->ts.nanosec = nanoseconds;

    ProcThreadNotification notif = PTN_TRANSMIT_DONE;
#ifdef FLEXPTP_FREERTOS
    BaseType_t hptWoken = false;
    if (xPortIsInsideInterrupt()) {
        xQueueSendFromISR(sTxCbFIFO, &pMsg, &hptWoken);
        xQueueSendFromISR(sNotificationFIFO, &notif, &hptWoken);
    } else {
        xQueueSend(sTxCbFIFO, &pMsg, portMAX_DELAY);
        xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY);
    }
#elif defined(FLEXPTP_CMSIS_OS2)
    osMessageQueuePut(sTxCbFIFO, &tx_ts, 0, osWaitForever);
    osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#endif
}

bool ptp_read_and_clear_transmit_timestamp(uint32_t tag, TimestampI *pTs) {
    // fetch message
    RawPtpMessage *pRawMsg = msgb_get_sent_by_tag(&sRawTxMsgBuf, tag);
    if (pRawMsg == NULL) {
        return false;
    }

    // copy timestamp
    *pTs = pRawMsg->ts;

    // release message
    msgb_free(&sRawTxMsgBuf, pRawMsg);

    return true;
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
            RawPtpMessage *pRawMsg;

#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sRxPacketFIFO, &pRawMsg, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sRxPacketFIFO, &pRawMsg, NULL, osWaitForever);
#endif
            // process packet
            ptp_process_packet(pRawMsg);

            // free buffer
            msgb_free(&sRawRxMsgBuf, pRawMsg);

        } else if (notification == PTN_TRANSMIT) { /* ---- TRANSMIT ----- */
            // pop packet from FIFO
            RawPtpMessage *pRawMsg;
#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sTxPacketFIFO, &pRawMsg, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(gTxPacketFIFO, &pRawMsg, NULL, osWaitForever);
#endif
            ptp_nsd_transmit_msg(pRawMsg);

        } else if (notification == PTN_TRANSMIT_DONE) { /* ---- TRANSMIT DONE ---- */
            // fetch the message
            RawPtpMessage *pRawMsg;
#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sTxCbFIFO, &pRawMsg, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sTxCbFIFO, &pRawMsg, NULL, osWaitForever);
#endif
            // set the sent flag
            msgb_set_sent(&sRawTxMsgBuf, pRawMsg);

            // invoke callback
            if (pRawMsg->pTxCb != NULL) {
                pRawMsg->pTxCb(pRawMsg);
            }

            // release message
            if ((pRawMsg->tag == RPMT_RANDOM) || (pRawMsg->pTxCb != NULL)) {
                msgb_free(&sRawTxMsgBuf, pRawMsg);
            }

        } else if (notification == PTN_EVENT) { /* ---- EVENT ----- */
            // pop event from the FIFO
            PtpCoreEvent event;
#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sEventFIFO, &event, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sEventFIFO, &event, NULL, osWaitForever);
#endif
            // delegate event processing
            ptp_process_event(&event);

            // tick the storage
            if (event.code == PTP_CEV_HEARTBEAT) {
                msgb_tick(&sRawRxMsgBuf);
                msgb_tick(&sRawTxMsgBuf);
            }
        }
    }
}

// --------------------------

// function to query PTP operation state
bool is_flexPTP_operating() {
    return sPTP_operating;
}

// --------------------------
