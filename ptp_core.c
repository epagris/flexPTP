/* (C) András Wiesner, 2020-2022 */

#include <flexptp/ptp_core.h>
#include <math.h>

#include <flexptp_options.h>

#ifndef SIMULATION
#include "FreeRTOS.h"
#include "timers.h"
#endif

#include <flexptp/ptp_defs.h>
#include <flexptp/ptp_types.h>
#include <flexptp/clock_utils.h>
#include <flexptp/format_utils.h>
#include <flexptp/msg_utils.h>
#include <flexptp/sbmc.h>
#include <flexptp/stats.h>
#include <flexptp/timeutils.h>
#include <flexptp/cli_cmds.h>
#include <flexptp/logging.h>

#ifndef SIMULATION
#include <flexptp/ptp_msg_tx.h>
#endif

// --------------

// --------------

void ptp_init();

// global state
PtpCoreState gPtpCoreState;
#define S (gPtpCoreState)

static PtpHeader sDelayReqHeader;       // header for sending Delay_Reg messages

static SyncCallback sSyncCallback = NULL;       // callback function on synchronization

// --------------------------

#define T1 (0)
#define T2 (1)
#define T3 (2)
#define T4 (3)

// --------------------------

void ptp_reset();

// --------------------------

void ptp_set_sync_callback(SyncCallback syncCB)
{
    sSyncCallback = syncCB;
}

static void ptp_sbmc_tmr_tick(TimerHandle_t timer);
static void ptp_delreq_tmr_tick(TimerHandle_t xTimer);

//#define RESP_TIMEOUT (2000) // allowed maximal Delay_Resp response time
#define SBMC_TICKRATE (1000)    // 1000ms period for SBMC-ticking

void ptp_create_timers()
{
    // create smbc timer
    S.timers.sbmc = xTimerCreate("sbmctimer", pdMS_TO_TICKS(SBMC_TICKRATE),     // timeout
                                 true,  // timer operates in repeat mode
                                 (void *)2,     // ID
                                 ptp_sbmc_tmr_tick);    // callback-function

    // create delreq timer
    S.timers.delreq = xTimerCreate("delreq", pdMS_TO_TICKS(ptp_logi2ms(0)), true, (void *)3, ptp_delreq_tmr_tick);
}

void ptp_delete_timers()
{
    xTimerDelete(S.timers.sbmc, 0);
    xTimerDelete(S.timers.delreq, 0);
}

// initialize Delay_Req header
void ptp_init_delay_req_header()
{
    sDelayReqHeader.messageType = PTP_MT_Delay_Req;
    sDelayReqHeader.transportSpecific = (uint8_t) S.profile.transportSpecific;
    sDelayReqHeader.versionPTP = 2;     // PTPv2
    sDelayReqHeader.messageLength = PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH;
    sDelayReqHeader.domainNumber = S.profile.domainNumber;
    ptp_clear_flags(&(sDelayReqHeader.flags));  // no flags
    sDelayReqHeader.correction_ns = 0;
    sDelayReqHeader.correction_subns = 0;

    memcpy(&sDelayReqHeader.clockIdentity, &S.hwoptions.clockIdentity, 8);

    sDelayReqHeader.sourcePortID = 1;   // TODO? No more ports...
    sDelayReqHeader.sequenceID = 0;     // will change in every sync cycle
    sDelayReqHeader.control = PTP_CON_Delay_Req;
    sDelayReqHeader.logMessagePeriod = 0;
}

// --------------------------------------

#define DEFAULT_SERVO_OFFSET (2800)

// initialize PTP module
void ptp_init(const uint8_t * hwa)
{
    // create clock identity
    ptp_create_clock_identity(hwa);

    // seed the randomizer
    srand(S.hwoptions.clockIdentity);

    // reset options
    nsToTsI(&S.hwoptions.offset, DEFAULT_SERVO_OFFSET);

    // initialize hardware
    PTP_HW_INIT(PTP_INCREMENT_NSEC, PTP_ADDEND_INIT);

    // initialize controller
    PTP_SERVO_INIT();

    // create timers
    ptp_create_timers();
    xTimerStart(S.timers.sbmc, 0);      // TODO!!

    // reset PTP subsystem
    ptp_reset();

#ifdef CLI_REG_CMD
    // register cli commands
    ptp_register_cli_commands();
#endif                          // CLI_REG_CMD
}

// deinit PTP module
void ptp_deinit()
{
#ifdef CLI_REG_CMD
    // remove cli commands
    ptp_remove_cli_commands();
#endif                          // CLI_REG_CMD

    // deinitialize controller
    PTP_SERVO_DEINIT();

    // delete timers
    ptp_delete_timers();
}

// construct and send Delay_Req message (NON-REENTRANT!)
void ptp_send_delay_req_message()
{
    static TimestampI zeroTs = { 0, 0 };        // timestamp appended at the end of packet

    // PTP message
    static RawPtpMessage delReqMsg = { 0 };
    delReqMsg.size = sDelayReqHeader.messageLength;
    delReqMsg.pTs = &(S.scd.t[T3]);
    delReqMsg.tx_dm = S.profile.delayMechanism;
    delReqMsg.tx_mc = PTP_MC_EVENT;

    // increment sequenceID
    sDelayReqHeader.sequenceID = ++S.messaging.delay_reqSequenceID;
    sDelayReqHeader.domainNumber = S.profile.domainNumber;

    // fill in header
    ptp_construct_binary_header(delReqMsg.data, &sDelayReqHeader);

    // fill in timestamp
    ptp_write_binary_timestamps(delReqMsg.data, &zeroTs, 1);

    // send message
    ptp_transmit_enqueue(&delReqMsg);
}

// perform clock correction based on gathered timestamps (NON-REENTRANT!)
void ptp_perform_correction()
{
    // don't do any processing if no delay_request data is present
    if (!nonZeroI(&S.network.meanPathDelay)) {
        return;
    }

    static TimestampI syncMa_prev = { 0 };

    // timestamps and time intervals
    TimestampI d, syncMa, syncSl, delReqSl, delReqMa;

    // copy timestamps to assign them with meaningful names
    syncMa = S.scd.t[T1];
    syncSl = S.scd.t[T2];
    delReqSl = S.scd.t[T3];
    delReqMa = S.scd.t[T4];

    // log timestamps (if enabled)

    CLILOG(S.logging.timestamps,
           "seqID: %u\n"
           "T1: %d.%09d <- Sync TX (master)\n"
           "T2: %d.%09d <- Sync RX (slave) \n"
           "T3: %d.%09d <- Del_Req TX (slave) \n"
           "T4: %d.%09d <- Del_Req RX (master)\n\n",
           (uint32_t) S.messaging.sequenceID,
           (int32_t) syncMa.sec, syncMa.nanosec, (int32_t) syncSl.sec, syncSl.nanosec, (int32_t) delReqSl.sec, delReqSl.nanosec, (int32_t) delReqMa.sec, delReqMa.nanosec);

    // compute difference between master and slave clock
    /*subTime(&d, &syncSl, &syncMa); // t2 - t1 ...
       subTime(&d, &d, &delReqMa); // - t4 ...
       addTime(&d, &d, &delReqSl); // + t3
       divTime(&d, &d, 2); // division by 2 */

    subTime(&d, &syncSl, &syncMa);      // t2 - t1 ...
    subTime(&d, &d, &S.network.meanPathDelay);  // - MPD

    // substract offset
    subTime(&d, &d, &S.hwoptions.offset);

    // normalize time difference (eliminate malformed time value issues)
    normTime(&d);

    // translate time difference into clock tick unit
    int32_t d_ticks = tsToTick(&d, PTP_CLOCK_TICK_FREQ_HZ);

    // if time difference is at least one second, then jump the clock
    int64_t d_ns = nsI(&d);
    if (llabs(d_ns) > 20000000) {
        PTP_SET_CLOCK(syncMa.sec, syncMa.nanosec);

        CLILOG(S.logging.info, "Time difference is over 20ms [%ldns], performing coarse correction!\n", d_ns);

        syncMa_prev = syncMa;

        return;
    }

    // prepare data to pass to the controller
    double measSyncPeriod_ns;

    TimestampI measSyncPeriod;
    subTime(&measSyncPeriod, &syncMa, &syncMa_prev);
    measSyncPeriod_ns = nsI(&measSyncPeriod);

    PtpServoAuxInput saux = { S.scd, S.messaging.logSyncPeriod, S.messaging.syncPeriodMs, measSyncPeriod_ns };

    // run controller
    float corr_ppb = PTP_SERVO_RUN(nsI(&d), &saux);

    // compute addend value
    int64_t compAddend = (int64_t) S.hwclock.addend + (int64_t) (corr_ppb * PTP_ADDEND_CORR_PER_PPB_F); // compute addend value
    S.hwclock.addend = MIN(compAddend, 0xFFFFFFFF);     // limit to 32-bit range

    // write addend into hardware
    PTP_SET_ADDEND(S.hwclock.addend);

    // collect statistics
    ptp_collect_stats(nsI(&d));

    // log on cli (if enabled)
    CLILOG(S.logging.def, "%d %09d %d %09d %d % 9d %d %u %f %ld %09lu\n",
           (int32_t) syncMa.sec, syncMa.nanosec, (int32_t) delReqMa.sec, delReqMa.nanosec,
           (int32_t) d.sec, d.nanosec, d_ticks, S.hwclock.addend, corr_ppb, nsI(&S.network.meanPathDelay), (uint64_t) measSyncPeriod_ns);

    // call sync callback if defined
    //if (sSyncCallback != NULL) {
    //  sSyncCallback(nsI(&d), &scd, S.addend);
    //}

    syncMa_prev = syncMa;
}

static void ptp_delreq_tmr_tick(TimerHandle_t xTimer)
{
    // check that our last Delay_Req has been responded
    CLILOG(S.messaging.delay_reqSequenceID != S.messaging.lastRespondedDelReqId, "(P)Del_Req #%d was unresponded!\n", S.messaging.delay_reqSequenceID);
    ptp_send_delay_req_message();
}

static void ptp_start_delreq_timer()
{
    // start DelReq timer if not running yet
    if (S.profile.logDelayReqPeriod != PTP_LOGPER_SYNCMATCHED) {
        xTimerReset(S.timers.delreq, 0);
        xTimerChangePeriod(S.timers.delreq, pdMS_TO_TICKS(ptp_logi2ms(S.profile.logDelayReqPeriod)), 0);
        xTimerStart(S.timers.delreq, 0);
    }
}

static void ptp_stop_delreq_timer()
{
    // stop DelReq timer (since there's no master to talk to)
    xTimerStop(S.timers.delreq, portMAX_DELAY);
}

static void ptp_sbmc_tmr_tick(TimerHandle_t timer)
{
    PtpSBmcState *s = &(S.sbmc);

    // main state machine dropout
    if (s->mstState == SBMC_MASTER_OK) {
        s->masterTOCntr += SBMC_TICKRATE;
        if (s->masterTOCntr > 2 * s->masterAnnPer_ms + SBMC_TICKRATE) {
            s->mstState = SBMC_NO_MASTER;
            s->masterProps.priority1 = 255;
            s->masterProps.grandmasterClockIdentity = 0;
            CLILOG(S.logging.info, "Master lost!\n");

            ptp_stop_delreq_timer();
        }
    }

    // candidate switchover machine dropout
    if (s->candState == SBMC_CANDIDATE_COLLECTION) {
        s->candTOCntr += SBMC_TICKRATE;
        if (s->candTOCntr > 2 * s->candAnnPer_ms + SBMC_TICKRATE) {
            s->candState = SBMC_NO_CANDIDATE;
        }
    }
}

// handle announce messages
void ptp_handle_announce_msg(PtpAnnounceBody * pAnn, PtpHeader * pHeader)
{
    PtpSBmcState *s = &(S.sbmc);
    bool masterChanged = false;

    switch (s->mstState) {
    case SBMC_NO_MASTER:       // no master, accept the first one announcing itself
        s->masterProps = *pAnn; // save master settings
        s->mstState = SBMC_MASTER_OK;   // master found
        s->candState = SBMC_NO_CANDIDATE;       // stop candidate processing, since master is selected
        s->masterAnnPer_ms = ptp_logi2ms(pHeader->logMessagePeriod);
        masterChanged = true;   // indicate that master has changed
        // no break here!
    case SBMC_MASTER_OK:       // already bound to master
        if (pAnn->grandmasterClockIdentity == s->masterProps.grandmasterClockIdentity) {        // only clear when receiving from elected master
            s->masterTOCntr = 0;        // clear counter
        }
    }

    // run only candidate state evaluation if no main state changed took place
    if (!masterChanged) {
        switch (s->candState) {
        case SBMC_NO_CANDIDATE:{
                if (ptp_select_better_master(pAnn, &s->masterProps) == 0) {
                    s->candProps = *pAnn;
                    s->candAnnPer_ms = ptp_logi2ms(pHeader->logMessagePeriod);
                    s->candState = SBMC_CANDIDATE_COLLECTION;   // switch to next syncState
                    s->candCntr = 1;
                }
                break;
            }

        case SBMC_CANDIDATE_COLLECTION:{
                // determine that the received master clock dataset is not better than the previously found one
                if (ptp_select_better_master(pAnn, &s->candProps) == 0) {       // if better, reset counter and stay in current syncState
                    s->candProps = *pAnn;
                    s->candCntr = 1;
                } else {
                    // verify, that announce message comes from the same source
                    if (pAnn->grandmasterClockIdentity == s->candProps.grandmasterClockIdentity) {
                        s->candCntr++;  // advance counter
                        s->candTOCntr = 0;      // clear counter
                    }

                    // if counter expired...
                    if (s->candCntr == ANNOUNCE_COLLECTION_WINDOW) {
                        s->masterProps = s->candProps;  // change master
                        s->candProps.priority1 = 255;   // set to worst value, i.e. make other values meaningless
                        s->candProps.grandmasterClockIdentity = 0;      // also clear ID

                        masterChanged = true;
                        s->candState = SBMC_NO_CANDIDATE;       // switch back to NO_CANDIDATE syncState
                    }
                }
                break;
            }
        }
    }

    if (masterChanged) {
        if (S.logging.info) {
            MSG("Switched to new master: ");
            ptp_print_clock_identity(s->masterProps.grandmasterClockIdentity);
            MSG("\n");
        }

        ptp_start_delreq_timer();
    }
}

void ptp_handle_correction_field(TimestampI * ts, const PtpHeader * pHeader)
{
    TimestampI correctionField;
    correctionField.sec = 0;
    correctionField.nanosec = pHeader->correction_ns;
    subTime(ts, ts, &correctionField);
    normTime(ts);
}

// TODO TODO TODO ...
void ptp_compute_mean_path_delay(const TimestampI * pTs, TimestampI * pMPD)
{
    //static double a = 0.533488091091103; // fc = 10Hz
    static double a = 0.00186744273170799;      // fc = 1Hz

    double mpd_prev_ns = nsI(pMPD);

    //MSG("%f\n", mpd_prev_ns);

    // compute difference between master and slave clock
    subTime(pMPD, &pTs[T2], &pTs[T1]);  // t2 - t1 ...
    subTime(pMPD, pMPD, &pTs[T3]);      // - t3 ...
    addTime(pMPD, pMPD, &pTs[T4]);      // + t4
    divTime(pMPD, pMPD, 2);     // division by 2

    // performing time error filtering
    double mpd_new_ns = nsI(pMPD);
    mpd_new_ns = a * mpd_prev_ns + (1 - a) * mpd_new_ns;        // filtering equation
    nsToTsI(pMPD, (int64_t) mpd_new_ns);
}

// reset PTP subsystem
void ptp_reset()
{
    // reset subsystem states...
    memset(&S.messaging, 0, sizeof(PtpMessagingState)); // messaging state
    S.hwclock.addend = PTP_ADDEND_INIT; // HW clock state
    PTP_SET_ADDEND(PTP_ADDEND_INIT);
    memset(&S.network, 0, sizeof(PtpNetworkState));     // network state
    memset(&S.sbmc, 0, sizeof(PtpSBmcState));   // SBMC state
    memset(&S.scd, 0, sizeof(PtpSyncCycleData));        // Sync cycle data

    // remove delreq time
    ptp_stop_delreq_timer();

    // reset controller
    PTP_SERVO_RESET();

    // (re)init header for sending (P)Delay_Req messages
    ptp_init_delay_req_header();

    // reset statistics
    ptp_clear_stats();
}

// packet processing (NON-REENTRANT!!)
void ptp_process_packet(RawPtpMessage * pRawMsg)
{
    static PtpHeader header;    // PTP header
    static Delay_RespIdentification delay_respID;       // identification received in every Delay_Resp packet
    //TimestampI correctionField;

    // header readout
    ptp_extract_header(&header, pRawMsg->data);

    // if other than Announce received
    //MSG("%d\n", header.messageType);

    // consider only messages in our domain
    if (header.domainNumber != S.profile.domainNumber || header.transportSpecific != S.profile.transportSpecific) {
        return;
    }

    if (header.messageType == PTP_MT_Announce) {
        PtpMasterProperties newMstProp;
        ptp_extract_announce_message(&newMstProp, pRawMsg->data);
        ptp_handle_announce_msg(&newMstProp, &header);
        return;
    }

    // process non-Announce messages
    if (header.messageType == PTP_MT_Sync || header.messageType == PTP_MT_Follow_Up) {
        switch (S.messaging.m2sState) {
            // wait for Sync message
        case SIdle:{
                // switch into next state if Sync packet has arrived
                if (header.messageType == PTP_MT_Sync) {
                    // save sync interval
                    S.messaging.logSyncPeriod = header.logMessagePeriod;
                    S.messaging.syncPeriodMs = ptp_logi2ms(header.logMessagePeriod);

                    //MSG("%d\n", header.logMessagePeriod);

                    // save reception time
                    S.scd.t[T2] = pRawMsg->ts;

                    // switch to next syncState
                    S.messaging.sequenceID = header.sequenceID;

                    // handle two step/one step messaging
                    //if (header.flags.PTP_TWO_STEP) {
                    S.messaging.m2sState = SWaitFollowUp;
                    //} else {
                    /*ptp_extract_timestamps(&sSyncData.t1, pRawMsg->data, 1); // extract t1
                       ptp_handle_correction_field(&sSyncData.t2, &header); // process correction field
                       ptp_perform_correction(); // run clock correction */
                    //}
                }

                break;
            }
            // wait for Follow_Up message
        case SWaitFollowUp:
            if (header.messageType == PTP_MT_Follow_Up) {
                // check sequence ID if the response is ours
                if (header.sequenceID == S.messaging.sequenceID) {
                    ptp_extract_timestamps(&S.scd.t[T1], pRawMsg->data, 1);     // read t1
                    ptp_handle_correction_field(&S.scd.t[T2], &header); // process correction field

                    // log correction field (if enabled)
                    CLILOG(S.logging.corr, "C [Follow_Up]: %09lu\n", header.correction_ns);

                    // delay Delay_Req transmission with a random amount of time
                    //vTaskDelay(pdMS_TO_TICKS(rand() % (S.syncIntervalMs / 2)));

                    // send Delay_Req message
                    if (S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED) {
                        ptp_send_delay_req_message();
                    }

                    // jump clock if error is way too big...
                    TimestampI d;
                    subTime(&d, &S.scd.t[T2], &S.scd.t[T1]);
                    if (d.sec != 0) {
                        PTP_SET_CLOCK(S.scd.t[T1].sec, S.scd.t[T1].nanosec);
                    }

                    // run servo only if not syncmatched
                    if (S.profile.logDelayReqPeriod != PTP_LOGPER_SYNCMATCHED) {
                        ptp_perform_correction();
                    }

                    // switch to next syncState
                    S.messaging.m2sState = SIdle;
                }
            }

            break;

        }

    }

    // ------ (P)DELAY_RESPONSE PROCESSING --------

    // wait for (P)Delay_Resp message
    if ((header.messageType == PTP_MT_Delay_Resp && S.profile.delayMechanism == PTP_DM_E2E)) {
        if (header.sequenceID == S.messaging.delay_reqSequenceID) {     // read clock ID of requester

            ptp_read_delay_resp_id_data(&delay_respID, pRawMsg->data);

            // if the response was sent to us as a response to our Delay_Req then continue processing
            if (delay_respID.requestingSourceClockIdentity == S.hwoptions.clockIdentity && delay_respID.requestingSourcePortIdentity == sDelayReqHeader.sourcePortID) {

                ptp_extract_timestamps(&S.scd.t[T4], pRawMsg->data, 1); // store t4
                ptp_handle_correction_field(&S.scd.t[T4], &header);     // substract correction field from t4

                // compute mean path delay
                ptp_compute_mean_path_delay(S.scd.t, &S.network.meanPathDelay);

                // store last response ID
                S.messaging.lastRespondedDelReqId = header.sequenceID;

                // perform correction if operating on syncmatched mode
                if (S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED) {
                    ptp_perform_correction();
                }

                // log correction field (if enabled)
                CLILOG(S.logging.corr, "C [Del_Resp]: %09lu\n", header.correction_ns);
            }

        }
    }

}

void ptp_store_config(PtpConfig * pConfig)
{
    pConfig->profile = S.profile;
    pConfig->offset = S.hwoptions.offset;
    pConfig->logging = (int)(S.logging.def) | (int)(S.logging.info) << 1 | (int)(S.logging.corr) << 2 | (int)(S.logging.timestamps) << 3 | (int)(S.logging.locked) << 4;
}

void ptp_load_config(const PtpConfig * pConfig)
{
    S.profile = pConfig->profile;
    S.hwoptions.offset = pConfig->offset;

    S.logging.def = pConfig->logging & 1;
    S.logging.info = (pConfig->logging >> 1) & 1;
    S.logging.corr = (pConfig->logging >> 2) & 1;
    S.logging.timestamps = (pConfig->logging >> 3) & 1;
    S.logging.locked = (pConfig->logging >> 4) & 1;
}

void ptp_load_config_from_dump(const void *pDump)
{
    PtpConfig config;
    memcpy(&config, pDump, sizeof(PtpConfig));
    ptp_load_config(&config);
    ptp_reset();
}

// -----------------------------------------------
