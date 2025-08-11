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
#include <time.h>

#include "minmax.h"

#ifdef __linux__
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#endif

///\cond 0
// ----- TASK PROPERTIES -----
#ifdef FLEXPTP_NON_LINUX_OS
#ifdef FLEXPTP_FREERTOS
static TaskHandle_t sTH; // task handle in direct FreeRTOS mode
#elif defined(FLEXPTP_CMSIS_OS2)
static osThreadId_t sTH; // task handle in CMSIS OS2 mode
#endif
static uint8_t sPrio = FLEXPTP_TASK_PRIORITY; // priority
static uint16_t sStkSize = 2048;              // stack size
static void task_ptp(void *pParam);           // task routine function
#elif defined(FLEXPTP_LINUX)
static pthread_t sTH;

static void *task_ptp(void *pParam);
#endif

// ---------------------------

static bool sPTP_operating = false; // does the PTP subsystem operate?

// ---------------------------

#define S (gPtpCoreState)

///\endcond

// ---------------------------

#define RX_PACKET_FIFO_LENGTH (32) ///< Receive packet FIFO length
#define TX_PACKET_FIFO_LENGTH (32) ///< Transmit packet FIFO length

// FIFO for incoming packets
#ifdef FLEXPTP_NON_LINUX_OS
#define EVENT_FIFO_LENGTH (32)        ///< Event FIFO length
#define NOTIFICATION_FIFO_LENGTH (16) ///< Notification FIFO length
#define TX_CALLBACK_FIFO_LENGTH (10)  ///< Transmit callback FIFO length
#endif

#define TX_TTL_MS (2000) ///< TTL for outbound packets
#define RX_TTL_MS (2000) ///< TTL for inbound packets

// -----------------------------

/**
 * @brief Notifications for the processing thread.
 */
typedef enum {
    PTN_NONE = 0x00,          ///< Empty notification
    PTN_RECEIVE = 0x01,       ///< A message has been received
    PTN_TRANSMIT = 0x02,      ///< A message awaits transmission
    PTN_TRANSMIT_DONE = 0x04, ///< A transmit timestamp awaits delegation
    PTN_EVENT = 0x08          ///< An event has occurred
} ProcThreadNotification;

/**
 * @brief Structure for communicating transmit timestamp writeback.
 */
typedef struct {
    uint32_t uid;         ///< Message UID
    uint32_t seconds;     ///< Timestamp seconds
    uint32_t nanoseconds; ///< Timestamp nanoseconds
} TxTs;

// -----------------------------

///\cond 0

// hearbeat timer
#ifdef FLEXPTP_FREERTOS
static TimerHandle_t sHeartBeatTmr;
#elif defined(FLEXPTP_CMSIS_OS2)
static osTimerId_t sHeartBeatTmr;
#elif defined(FLEXPTP_LINUX)
static timer_t sHeartBeatTmr;
#endif

// queues for message reception and transmission
#ifdef FLEXPTP_FREERTOS
static QueueHandle_t sRxPacketFIFO, sEventFIFO;
static QueueHandle_t sTxPacketFIFO;
static QueueHandle_t sNotificationFIFO;
static QueueHandle_t sTxCbFIFO;
#elif defined(FLEXPTP_CMSIS_OS2)
static osMessageQueueId_t sRxPacketFIFO, sEventFIFO;
static osMessageQueueId_t sTxPacketFIFO;
static osMessageQueueId_t sNotificationFIFO;
static osMessageQueueId_t sTxCbFIFO;
#elif defined(FLEXPTP_LINUX)
static int sRxPacketFIFO[2];
static int sTxPacketFIFO[2];
static int sEventFIFO[2];
static int sTxCbFIFO[2];
static sem_t sTxCbSem;
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
#elif defined(FLEXPTP_LINUX)
    union sigval data
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
    sHeartBeatTmr = NULL;
#if FLEXPTP_FREERTOS
    sHeartBeatTmr = xTimerCreate("ptp_heartbeat", pdMS_TO_TICKS(PTP_HEARTBEAT_TICKRATE_MS), // timeout
                                 true,                                                      // timer operates in repeat mode
                                 NULL,                                                      // ID
                                 ptp_heartbeat_tmr_cb);                                     // callback-function
#elif defined(FLEXPTP_CMSIS_OS2)
    sHeartBeatTmr = osTimerNew(ptp_heartbeat_tmr_cb, osTimerPeriodic, NULL, NULL);
#elif defined(FLEXPTP_LINUX)
    struct sigevent sev = {
        .sigev_notify = SIGEV_THREAD,
        .sigev_notify_function = ptp_heartbeat_tmr_cb,
        .sigev_value = {.sival_ptr = NULL}};
    if (timer_create(CLOCK_REALTIME, &sev, &sHeartBeatTmr) < 0) {
        sHeartBeatTmr = NULL;
    }
#endif
    if (sHeartBeatTmr == NULL) {
        MSG("Failed to create the PTP heartbeat timer!\n");
    }
}

/**
 * Remove the heartbeat timer.
 */
static void ptp_remove_heartbeat_tmr() {
    ptp_stop_heartbeat_tmr();
#ifdef FLEXPTP_FREERTOS
    xTimerDelete(sHeartBeatTmr, 0);
#elif defined(FLEXPTP_CMSIS_OS2)
    osTimerDelete(sHeartBeatTmr);
#elif defined(FLEXPTP_LINUX)
    timer_delete(sHeartBeatTmr);
#endif
    sHeartBeatTmr = NULL;
}

void ptp_start_heartbeat_tmr() {
#ifdef FLEXPTP_FREERTOS
    xTimerStart(sHeartBeatTmr, 0);
#elif defined(FLEXPTP_CMSIS_OS2)
    osTimerStart(sHeartBeatTmr, (PTP_HEARTBEAT_TICKRATE_MS * 1000) / osKernelGetTickFreq());
#elif defined(FLEXPTP_LINUX)
    struct itimerspec its = {
        .it_interval = {
            .tv_sec = PTP_HEARTBEAT_TICKRATE_MS / 1000, .tv_nsec = (PTP_HEARTBEAT_TICKRATE_MS % 1000) * 1000000},
        .it_value = {.tv_sec = 1, .tv_nsec = 0} // just some non-zero value
    };
    timer_settime(sHeartBeatTmr, 0, &its, NULL);
#endif
}

void ptp_stop_heartbeat_tmr() {
#ifdef FLEXPTP_FREERTOS
    xTimerStop(sHeartBeatTmr, 0);
#elif defined(FLEXPTP_CMSIS_OS2)
    osTimerStop(sHeartBeatTmr);
#elif defined(FLEXPTP_LINUX)
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    timer_settime(sHeartBeatTmr, 0, &its, NULL);
#endif
}

// ----------------------------

// create message queues
static void ptp_create_message_queues() {
    // create packet FIFO
#ifdef FLEXPTP_FREERTOS
    sRxPacketFIFO = xQueueCreate(RX_PACKET_FIFO_LENGTH, sizeof(uint32_t));
    sTxPacketFIFO = xQueueCreate(TX_PACKET_FIFO_LENGTH, sizeof(uint32_t));
    sEventFIFO = xQueueCreate(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent));
    sNotificationFIFO = xQueueCreate(NOTIFICATION_FIFO_LENGTH, sizeof(ProcThreadNotification));
    sTxCbFIFO = xQueueCreate(TX_CALLBACK_FIFO_LENGTH, sizeof(TxTs));
#elif defined(FLEXPTP_CMSIS_OS2)
    sRxPacketFIFO = osMessageQueueNew(RX_PACKET_FIFO_LENGTH, sizeof(uint32_t), NULL);
    sTxPacketFIFO = osMessageQueueNew(TX_PACKET_FIFO_LENGTH, sizeof(uint32_t), NULL);
    sEventFIFO = osMessageQueueNew(EVENT_FIFO_LENGTH, sizeof(PtpCoreEvent), NULL);
    sNotificationFIFO = osMessageQueueNew(NOTIFICATION_FIFO_LENGTH, sizeof(ProcThreadNotification), NULL);
    sTxCbFIFO = osMessageQueueNew(TX_CALLBACK_FIFO_LENGTH, sizeof(TxTs), NULL);
#elif defined(FLEXPTP_LINUX)
    pipe(sRxPacketFIFO);
    pipe(sTxPacketFIFO);
    pipe(sEventFIFO);
    pipe(sTxCbFIFO);
    sem_init(&sTxCbSem, 0, 0);
#endif

    // initalize packet buffers
    msgb_init(&sRawRxMsgBuf, sRawRxMsgBufPool, RX_PACKET_FIFO_LENGTH);
    msgb_init(&sRawTxMsgBuf, sRawTxMsgBufPool, TX_PACKET_FIFO_LENGTH);
}

#ifdef FLEXPTP_LINUX
#define CLOSE_PIPE(pipefd) \
    close(pipefd[0]);      \
    close(pipefd[1])
#endif

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
    osMessageQueueDelete(sTxPacketFIFO);
    osMessageQueueDelete(sEventFIFO);
    osMessageQueueDelete(sNotificationFIFO);
    osMessageQueueDelete(sTxCbFIFO);
#elif defined(FLEXPTP_LINUX)
    CLOSE_PIPE(sRxPacketFIFO);
    CLOSE_PIPE(sTxPacketFIFO);
    CLOSE_PIPE(sEventFIFO);
    CLOSE_PIPE(sTxCbFIFO);
    sem_destroy(&sTxCbSem);
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
    ptp_init();

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
#ifdef FLEXPTP_FREERTOS
    sTH = NULL;
    BaseType_t result = xTaskCreate(task_ptp, "ptp", sStkSize / sizeof(BaseType_t), NULL, sPrio, &sTH);
    if (result != pdPASS) {
        MSG("Failed to create the PTP task! (errcode: %d)\n", result);
        unreg_task_ptp();
        return;
    }
#elif defined(FLEXPTP_CMSIS_OS2)
    sTH = NULL;
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
#elif defined(FLEXPTP_LINUX)
    sTH = 0;
    if (pthread_create(&sTH, NULL, task_ptp, NULL) != 0) {
        MSG("Failed to create the PTP thread!\n");
        unreg_task_ptp();
        return;
    }
    // struct sched_param sched_param = { .sched_priority = 10 };
    // if (sched_setscheduler(getpid(), SCHED_FIFO, &sched_param)) {
    //     MSG("Could not switch to realtime scheduling!\n");
    //     return;
    // }
#endif

    // the PTP subsystem is operating
    sPTP_operating = true;
}

// unregister PTP task
void unreg_task_ptp() {
    ptp_remove_heartbeat_tmr(); // remove the heartbeat timer
    ptp_nsd_init(-1, -1);       // de-initialize the network stack driver
#if defined(FLEXPTP_NON_LINUX_OS)
    if (sTH != NULL) {
#ifdef FLEXPTP_FREERTOS
        vTaskDelete(sTH); // delete task
#elif defined(FLEXPTP_CMSIS_OS2)
        osThreadTerminate(sTH);
#endif
    }
    sTH = NULL;
#elif defined(FLEXPTP_LINUX)
    if (sTH != 0) {
        PtpCoreEvent event = {.code = PTP_CEV_TERMINATE, .w = 0, .dw = 0};
        ptp_event_enqueue(&event);
        pthread_join(sTH, NULL);
    }
#endif
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
    ok = osMessageQueuePut(sEventFIFO, event, 0, osWaitForever) == osOK;
    if (ok) {
        osMessageQueuePut(sNotificationFIFO, &notif, osWaitForever);
    }
#elif defined(FLEXPTP_LINUX)
    size_t len = sizeof(PtpCoreEvent);
    ok = write(sEventFIFO[1], event, len) == len;
#endif
    return ok;
}

// put ptp message onto processing queue
void ptp_receive_enqueue(const void *pPayload, uint32_t len, uint32_t ts_sec, uint32_t ts_ns, int tp) {
    // only consider messages received on the matching transport layer
    if ((!sPTP_operating) || (tp != ptp_get_transport_type())) {
        return;
    }

    // enqueue message
    RawPtpMessage *pMsgAlloc = msgb_alloc(&sRawRxMsgBuf, RPMT_RANDOM, FLEXPTP_MS_TO_TICKS(RX_TTL_MS));
    if (pMsgAlloc) {
        // copy payload and timestamp
        uint32_t copyLen = MIN(len, MAX_PTP_MSG_SIZE);
        memcpy(pMsgAlloc->data, pPayload, copyLen);
        pMsgAlloc->size = copyLen;
        pMsgAlloc->ts.sec = ts_sec;
        pMsgAlloc->ts.nanosec = ts_ns;
        pMsgAlloc->tag = RPMT_RANDOM;
        pMsgAlloc->pTxCb = NULL; // not meaningful...

        // commit the allocation
        msgb_commit(&sRawRxMsgBuf, pMsgAlloc);

        // get the UID
        uint32_t uid = msgb_get_uid(&sRawRxMsgBuf, pMsgAlloc);

        // set the notification
        ProcThreadNotification notif = PTN_RECEIVE;
#ifdef FLEXPTP_FREERTOS
        xQueueSend(sRxPacketFIFO, &uid, portMAX_DELAY);       // send index
        xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY); // send notification
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueuePut(sRxPacketFIFO, &uid, 0, osWaitForever);
        osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#elif defined(FLEXPTP_LINUX)
        write(sRxPacketFIFO[1], &uid, sizeof(uint32_t));
#endif
    } else {
        if (msgb_get_error(&sRawRxMsgBuf) == MSGB_ERR_FULL) {
            CLILOG(S.logging.info, "The PTP receive packet buffer is full, a packet was lost!\n");
        }
    }
}

bool ptp_transmit_enqueue(const RawPtpMessage *pMsg) {
    RawPtpMessage *pMsgAlloc = msgb_alloc(&sRawTxMsgBuf, pMsg->tag, pMsg->ttl);
    if (pMsgAlloc) {
        memcpy(pMsgAlloc, pMsg, sizeof(RawPtpMessage));
        msgb_commit(&sRawTxMsgBuf, pMsgAlloc);
        uint32_t uid = msgb_get_uid(&sRawTxMsgBuf, pMsgAlloc);
        ProcThreadNotification notif = PTN_TRANSMIT;
#ifdef FLEXPTP_FREERTOS
        BaseType_t hptWoken = false;
        if (xPortIsInsideInterrupt()) {
            xQueueSendFromISR(sTxPacketFIFO, &uid, &hptWoken);
            xQueueSendFromISR(sNotificationFIFO, &notif, &hptWoken);
        } else {
            xQueueSend(sTxPacketFIFO, &uid, portMAX_DELAY);
            xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY);
        }
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueuePut(sTxPacketFIFO, &uid, 0, osWaitForever);
        osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#elif defined(FLEXPTP_LINUX)
        write(sTxPacketFIFO[1], &uid, sizeof(uint32_t));
#endif
        return true;
    } else {
        if (msgb_get_error(&sRawTxMsgBuf) == MSGB_ERR_FULL) {
            CLILOG(S.logging.info, "PTP TX Enqueue failed, buffer is full! (%u)\n", pMsg->tag);
            PTP_IUEV(PTP_UEV_QUEUE_ERROR); // dispatch QUEUE_ERROR event
        }
        return false;
    }
}

void ptp_transmit_timestamp_cb(uint32_t uid, uint32_t seconds, uint32_t nanoseconds) {
    // create timestamp association object
    TxTs ts = {.uid = uid, .seconds = seconds, .nanoseconds = nanoseconds};

    // dispatch notification
    ProcThreadNotification notif = PTN_TRANSMIT_DONE;
#ifdef FLEXPTP_FREERTOS
    BaseType_t hptWoken = false;
    if (xPortIsInsideInterrupt()) {
        xQueueSendFromISR(sTxCbFIFO, &ts, &hptWoken);
        xQueueSendFromISR(sNotificationFIFO, &notif, &hptWoken);
    } else {
        xQueueSend(sTxCbFIFO, &ts, portMAX_DELAY);
        xQueueSend(sNotificationFIFO, &notif, portMAX_DELAY);
    }
#elif defined(FLEXPTP_CMSIS_OS2)
    osMessageQueuePut(sTxCbFIFO, &ts, 0, osWaitForever);
    osMessageQueuePut(sNotificationFIFO, &notif, 0, osWaitForever);
#elif defined(FLEXPTP_LINUX)
    write(sTxCbFIFO[1], &ts, sizeof(TxTs));
    sem_post(&sTxCbSem);
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
#ifdef FLEXPTP_NON_LINUX_OS
static void task_ptp(void *pParam) {

#elif defined(FLEXPTP_LINUX)
static void *task_ptp(void *pParam) {
#endif
    bool run = true;
    while (run) {
        // wait for received packet or packet to transfer
        ProcThreadNotification notification = PTN_NONE;
#ifdef FLEXPTP_FREERTOS
        xQueueReceive(sNotificationFIFO, &notification, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
        osMessageQueueGet(sNotificationFIFO, &notification, NULL, osWaitForever);
#elif defined(FLEXPTP_LINUX)

    // prepare polling
#define POLL_N_FD (4)
    struct pollfd pfd[POLL_N_FD] = {
        {.fd = sRxPacketFIFO[0], .events = POLLIN, .revents = 0},
        {.fd = sTxPacketFIFO[0], .events = POLLIN, .revents = 0},
        {.fd = sTxCbFIFO[0], .events = POLLIN, .revents = 0},
        {.fd = sEventFIFO[0], .events = POLLIN, .revents = 0},
    };
    ProcThreadNotification notif_mapping[POLL_N_FD] = {
        PTN_RECEIVE,
        PTN_TRANSMIT,
        PTN_TRANSMIT_DONE,
        PTN_EVENT};

    // call the poll
    int pret = poll(pfd, POLL_N_FD, -1);
    if (pret > 0) {
        // at least one of the pipes have data to read
        for (uint32_t i = 0; i < POLL_N_FD; i++) {
            if (pfd[i].revents & POLLIN) {
                notification |= notif_mapping[i];
            }
        }
    } else {
        // error occurred, just skip this cycle
        CLILOG(S.logging.info, "A polling error occurred!\n");
        continue;
    }

#endif
        /* ---- TRANSMIT DONE ---- */
        if (notification & PTN_TRANSMIT_DONE) {

            // get the timestamp association object
            TxTs ts;

#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sTxCbFIFO, &ts, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sTxCbFIFO, &ts, NULL, osWaitForever);
#elif defined(FLEXPTP_LINUX)
            read(sTxCbFIFO[0], &ts, sizeof(TxTs));
#endif

            // fetch the message
            CLILOG(S.logging.transmission, "[% 8u]---> %u\n", S.ticks, ts.uid);
            RawPtpMessage *pRawMsg = msgb_get_by_uid(&sRawTxMsgBuf, ts.uid);
            if (pRawMsg != NULL) {
                // insert the timestamp
                pRawMsg->ts.sec = ts.seconds;
                pRawMsg->ts.nanosec = ts.nanoseconds;

                // set the 'sent' flag
                msgb_set_sent(&sRawTxMsgBuf, pRawMsg);

                // invoke callback
                if (pRawMsg->pTxCb != NULL) {
                    pRawMsg->pTxCb(pRawMsg);
                }

                // release message
                if ((pRawMsg->tag == RPMT_RANDOM) || (pRawMsg->pTxCb != NULL)) {
                    msgb_free(&sRawTxMsgBuf, pRawMsg);
                    CLILOG(S.logging.transmission, "[% 8u] %u AUTOFREE\n", S.ticks, ts.uid);
                }
            } else {
                // null messages
            }
        }

        /* ---- TRANSMIT ----- */
        if (notification & PTN_TRANSMIT) {

            // pop packet UID from the FIFO
            uint32_t uid = 0;

#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sTxPacketFIFO, &uid, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sTxPacketFIFO, &uid, NULL, osWaitForever);
#elif defined(FLEXPTP_LINUX)
        read(sTxPacketFIFO[0], &uid, sizeof(uint32_t));
#endif
            // fetch the message
            RawPtpMessage *pRawMsg = msgb_get_by_uid(&sRawTxMsgBuf, uid);
            if (pRawMsg != NULL) {
                CLILOG(S.logging.transmission, "[% 8u] %u (%u) --->\n", S.ticks, uid, pRawMsg->tag & (~((uint32_t)MSGBUF_TAG_OVERWRITE)));
                ptp_nsd_transmit_msg(pRawMsg, uid);
#ifdef FLEXPTP_LINUX
                sem_wait(&sTxCbSem);
#endif
            }
        }

        // if packet is on the RX queue
        /* ---- RECIEVE ----- */
        if (notification & PTN_RECEIVE) {
            // pop packet UID from the FIFO
            uint32_t uid = 0;

#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sRxPacketFIFO, &uid, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sRxPacketFIFO, &uid, NULL, osWaitForever);
#elif defined(FLEXPTP_LINUX)
            read(sRxPacketFIFO[0], &uid, sizeof(uint32_t));
#endif

            // fetch message
            RawPtpMessage *pRawMsg = msgb_get_by_uid(&sRawRxMsgBuf, uid);
            if (pRawMsg != NULL) {
                // process packet
                ptp_process_packet(pRawMsg);

                // free buffer
                msgb_free(&sRawRxMsgBuf, pRawMsg);
            }
        }

        /* ---- EVENT ----- */
        if (notification == PTN_EVENT) {

            // pop event from the FIFO
            PtpCoreEvent event;
#ifdef FLEXPTP_FREERTOS
            xQueueReceive(sEventFIFO, &event, portMAX_DELAY);
#elif defined(FLEXPTP_CMSIS_OS2)
            osMessageQueueGet(sEventFIFO, &event, NULL, osWaitForever);
#elif defined(FLEXPTP_LINUX)
        read(sEventFIFO[0], &event, sizeof(PtpCoreEvent));
#endif

            // delegate event processing
            ptp_process_event(&event);

            // tick the storage
            if (event.code == PTP_CEV_HEARTBEAT) {
                msgb_tick(&sRawRxMsgBuf);
                msgb_tick(&sRawTxMsgBuf);
            }

            // handle peaceful termination
            if (event.code == PTP_CEV_TERMINATE) {
#ifdef FLEXPTP_LINUX
                run = false;
#endif
            }
        }
    }

#ifdef FLEXPTP_LINUX
    return NULL;
#endif
}

// --------------------------

// function to query PTP operation state
bool is_flexPTP_operating() {
    return sPTP_operating;
}

// --------------------------