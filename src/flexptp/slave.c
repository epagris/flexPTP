#include "slave.h"
#include "common.h"

#include "format_utils.h"
#include "msg_utils.h"
#include "portmacro.h"
#include "ptp_types.h"
#include "settings_interface.h"
#include "stats.h"
#include "task_ptp.h"
#include "timeutils.h"

#include "ptp_core.h"
#include "ptp_defs.h"
#include <math.h>

#ifdef MIN
#undef MIN
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

///\cond 0
#define S (gPtpCoreState)
///\endcond

// --------------

/**
 * Clock tuning.
 *
 * @param tuning_ppb clock tuning in PPB
 */
static void ptp_tune_clock(float tuning_ppb) {
#ifdef PTP_ADDEND_INTERFACE
    int64_t compAddend = (int64_t)S.hwclock.addend + (int64_t)(corr_ppb * PTP_ADDEND_CORR_PER_PPB_F); // compute addend value
    S.hwclock.addend = MIN(compAddend, 0xFFFFFFFF);                                                   // limit to 32-bit range
    PTP_SET_ADDEND(S.hwclock.addend);                                                                 // write addend into hardware
#elif defined(PTP_HLT_INTERFACE)
    S.hwclock.tuning_ppb += tuning_ppb;
    PTP_SET_TUNING(S.hwclock.tuning_ppb);
#endif
}

#define PTP_FC_SKEW_CORRECTION_CYCLES (4)  ///< Fast compensation: no. of skew correction cycles
#define PTP_FC_TIME_CORRECTION_CYCLES (1)  ///< Fast compensation: no. of time correction cycles
#define PTP_FC_TIME_PROPAGATION_CYCLES (2) //< Fast compensation: no. of time propagation cycles

/**
 * Perform clock correction based on gathered timestamps.
 */
static void ptp_perform_correction() {
    // don't do any processing if no delay_request data is present
    if (!nonZeroI(&S.network.meanPathDelay)) {
        return;
    }

    // timestamps and time intervals
    TimestampI d, syncMa, syncSl, delReqSl, delReqMa;

    // copy timestamps to assign them with meaningful names
    syncMa = S.slave.scd.t[T1];
    syncSl = S.slave.scd.t[T2];
    delReqSl = S.slave.scd.t[T3];
    delReqMa = S.slave.scd.t[T4];

    // log timestamps (if enabled)
    if (S.profile.delayMechanism == PTP_DM_E2E) {
        CLILOG(S.logging.timestamps,
               "seqID: %u\n"
               "T1: %d.%09d <- Sync TX (master)\n"
               "T2: %d.%09d <- Sync RX (slave) \n"
               "T3: %d.%09d <- Del_Req TX (slave) \n"
               "T4: %d.%09d <- Del_Req RX (master)\n\n",
               (uint32_t)S.slave.messaging.sequenceID,
               (int32_t)syncMa.sec, syncMa.nanosec,
               (int32_t)syncSl.sec, syncSl.nanosec,
               (int32_t)delReqSl.sec, delReqSl.nanosec,
               (int32_t)delReqMa.sec, delReqMa.nanosec);
    } else if (S.profile.delayMechanism == PTP_DM_P2P) {
        CLILOG(S.logging.timestamps,
               "seqID: %u\n"
               "T1: %d.%09d <- Sync TX (master)\n"
               "T2: %d.%09d <- Sync RX (slave)\n"
               "t1: %d.%09d <- PDel_Req TX (our clock)\n"
               "t2: %d.%09d <- PDel_Req RX (their clock)\n"
               "t3: %d.%09d <- PDel_Resp TX (their clock)\n"
               "t4: %d.%09d <- PDel_Resp RX (our clock)\n\n",
               (uint32_t)S.slave.messaging.sequenceID,
               (int32_t)S.slave.scd.t[0].sec, S.slave.scd.t[0].nanosec,
               (int32_t)S.slave.scd.t[1].sec, S.slave.scd.t[1].nanosec,
               (int32_t)S.slave.scd.t[2].sec, S.slave.scd.t[2].nanosec,
               (int32_t)S.slave.scd.t[3].sec, S.slave.scd.t[3].nanosec,
               (int32_t)S.slave.scd.t[4].sec, S.slave.scd.t[4].nanosec,
               (int32_t)S.slave.scd.t[5].sec, S.slave.scd.t[5].nanosec);
    }

    // ------------------------------

    // variable for later substraction of summed correction fields
    TimestampI cf = {0, 0};
    nsToTsI(&cf, S.slave.scd.cf[T1] + S.slave.scd.cf[T2]);

    // compute difference between master and slave clocks
    subTime(&d, &syncSl, &syncMa);             // t2 - t1 ...
    subTime(&d, &d, &S.network.meanPathDelay); // - MPD
    subTime(&d, &d, &cf);                      // - CF of (Sync + Follow_Up)

    // substract offset
    subTime(&d, &d, &S.hwoptions.offset);

    // normalize time difference (eliminate malformed time value issues)
    normTime(&d);

    // ------------------------------

    // fill the previous state variables
    if (!nonZeroI(&S.slave.prevSyncMa) || !nonZeroI(&S.slave.prevSyncSl) || !nonZeroI(&S.slave.prevTimeError)) {
        goto retain_cycle_data;
    }

    // ------------------------------

    // determine the Sync period
    int64_t measSyncPeriod_ns;
    // // if momentarily sync interval is not directly measurable, use the nominal value
    // if (S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED && S.profile.delayMechanism == PTP_DM_E2E) {
    //     measSyncPeriod_ns = S.slave.messaging.syncPeriodMs * 1E+06;
    // } else { // if accurately measurable...
    TimestampI measSyncPeriod;
    subTime(&measSyncPeriod, &syncMa, &(S.slave.prevSyncMa));
    measSyncPeriod_ns = nsI(&measSyncPeriod);
    // }

    // ------------------------------

    // if time difference is greater than the predefined threshold then jump the clock
    int64_t d_ns = nsI(&d);
    PtpFastCompState fcs = S.slave.fastCompState;
    if ((llabs(d_ns) > S.slave.coarseLimit) || (fcs != PTP_FC_IDLE)) {
        if (fcs == PTP_FC_IDLE) {
            // reset the servo
            PTP_SERVO_RESET();

            // print info
            CLILOG(S.logging.info, "Time difference has exceeded the coarse correction threshold [%ldns], compensation commenced!\n", d_ns);
        }

        uint8_t fccntr = S.slave.fastCompCntr;
        switch (fcs) {
        case PTP_FC_IDLE:
            fcs = PTP_FC_SKEW_CORRECTION;
            fccntr = 0;
        case PTP_FC_SKEW_CORRECTION:
            if (fccntr == PTP_FC_SKEW_CORRECTION_CYCLES) {
                fcs = PTP_FC_TIME_CORRECTION;
                fccntr = 0;
            }
            break;
        case PTP_FC_TIME_CORRECTION:
            if (fccntr == PTP_FC_TIME_CORRECTION_CYCLES) {
                fcs = PTP_FC_TIME_CORRECTION_PROPAGATION;
                fccntr = 0;
            }
            break;
        case PTP_FC_TIME_CORRECTION_PROPAGATION:
            if (fccntr == PTP_FC_TIME_PROPAGATION_CYCLES) {
                fcs = PTP_FC_IDLE;
                fccntr = 0;
            }
            break;
        }

        // skew correction
        if (fcs == PTP_FC_SKEW_CORRECTION) {
            // calculate clock skew
            TimestampI dt2;
            subTime(&dt2, &syncSl, &S.slave.prevSyncSl);
            int64_t dt2_ns = nsI(&dt2);
            double skew = (double)(dt2_ns - measSyncPeriod_ns) / (double)(measSyncPeriod_ns);
            double skew_compensation_ppb = -skew * 1E+09;

            // compensate the clock skew
            ptp_tune_clock(skew_compensation_ppb);

            // log skew compensation
            CLILOG(S.logging.info, "[%u/%u] Skew compensation: % 6.4f ppb\n", fccntr + 1, PTP_FC_SKEW_CORRECTION_CYCLES, skew_compensation_ppb);
        } else if (fcs == PTP_FC_TIME_CORRECTION) { // time correction
            // compensate time error
            TimestampU tu;
            PTP_HW_GET_TIME(&tu);
            uint64_t t_ns = nsU(&tu);
            t_ns -= d_ns;
            TimestampI ti;
            nsToTsI(&ti, t_ns);
            PTP_SET_CLOCK((uint32_t)ti.sec, ti.nanosec);

            // log time compensation
            CLILOG(S.logging.info, "[%u/%u] Time compensation: %ld ns\n", fccntr + 1, PTP_FC_TIME_CORRECTION_CYCLES, d_ns);
        } else if (fcs == PTP_FC_TIME_CORRECTION_PROPAGATION) {
            CLILOG(S.logging.info, "[%u/%u] Waiting for time compensation to propagate.\n", fccntr + 1, PTP_FC_TIME_PROPAGATION_CYCLES, d_ns);
        }

        // maintain FC state
        S.slave.fastCompState = fcs;
        S.slave.fastCompCntr = fccntr + 1;

        // retain sync cycle data
        goto retain_cycle_data;
    }

    // ------------------------------

    // prepare data to pass to the controller
    PtpServoAuxInput saux = {S.slave.scd,
                             S.slave.messaging.logSyncPeriod,
                             S.slave.messaging.syncPeriodMs,
                             measSyncPeriod_ns};

    // run controller
    float corr_ppb = PTP_SERVO_RUN(nsI(&d), &saux);

    // set clock tuning
    ptp_tune_clock(corr_ppb);

    // collect statistics
    ptp_collect_stats(nsI(&d));

    // log on cli (if enabled)
#ifdef PTP_ADDEND_INTERFACE
    int32_t d_ticks = tsToTick(&d, PTP_CLOCK_TICK_FREQ_HZ);
    CLILOG(S.logging.def, "%d %09d %d %09d %d " PTP_COLOR_BYELLOW "% 9d" PTP_COLOR_RESET " % 9d % 12u % 8.4f % 9ld % 9lu\n",
           (int32_t)syncMa.sec, syncMa.nanosec, (int32_t)delReqMa.sec, delReqMa.nanosec,
           (int32_t)d.sec, d.nanosec, d_ticks,
           S.hwclock.addend, corr_ppb, nsI(&S.network.meanPathDelay), (uint64_t)measSyncPeriod_ns);
#elif defined(PTP_HLT_INTERFACE)
    CLILOG(S.logging.def, "%d %09d %d %09d %d " PTP_COLOR_BYELLOW "% 9d" PTP_COLOR_RESET " % 8.4f % 8.4f % 9ld % 9lu\n",
           (int32_t)syncMa.sec, syncMa.nanosec, (int32_t)delReqMa.sec, delReqMa.nanosec,
           (int32_t)d.sec, d.nanosec,
           S.hwclock.tuning_ppb, corr_ppb, nsI(&S.network.meanPathDelay), (uint64_t)measSyncPeriod_ns);
#endif

    // call sync callback if defined
    if (S.slave.syncCb != NULL) {
#ifdef PTP_ADDEND_INTERFACE
        S.slave.syncCb(nsI(&d), &S.slave.scd, S.hwclock.addend);
#elif defined(PTP_HLT_INTERFACE)
        S.slave.syncCb(nsI(&d), &S.slave.scd, S.hwclock.tuning_ppb);
#endif
    }

    // ---------------------

retain_cycle_data:
    S.slave.prevSyncMa = syncMa;
    S.slave.prevSyncSl = syncSl;
    S.slave.prevTimeError = d;
}

/**
 * Initiate the E2E correction.
 * This piece of code has been extracted from the ptp_slave_process_message() to prevent code duplication.
 */
static void ptp_commence_e2e_correction() {
    // send Delay_Req message
    if (S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED) {
        // send (P)Delay_Req message
        ptp_send_delay_req_message();

        // dispatch (P)DELAY_REQ_SENT event
        PTP_IUEV((S.profile.delayMechanism == PTP_DM_E2E) ? PTP_UEV_DELAY_REQ_SENT : PTP_UEV_PDELAY_REQ_SENT);
    }

    // jump clock if error is way too big...
    TimestampI d;
    subTime(&d, &S.slave.scd.t[T2], &S.slave.scd.t[T1]);
    if (d.sec != 0) {
        PTP_SET_CLOCK((int32_t)S.slave.scd.t[T1].sec, S.slave.scd.t[T1].nanosec);
    }

    // run servo only if issuing Delay_Requests is not syncmatched
    if (S.profile.logDelayReqPeriod != PTP_LOGPER_SYNCMATCHED) {
        ptp_perform_correction();
    }
}

/**
 * Initiate the P2P correction.
 * This piece of code has been extracted from the ptp_slave_process_message() to prevent code duplication.
 *
 * @param pdelRespSeqId sequence ID of the last completed PDel_Req...PDel_Resp(_Follow_Up) cycle
 */
static void ptp_commence_p2p_correction(uint32_t pdelRespSeqId) {
    // compute mean path delay
    ptp_compute_mean_path_delay_p2p(S.slave.scd.t + 2, S.slave.scd.cf + 2, &S.network.meanPathDelay);

    // store last response ID
    S.slave.messaging.lastRespondedDelReqId = pdelRespSeqId;

    if (S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED) {
        ptp_perform_correction();
    }
}

// packet processing
void ptp_slave_process_message(RawPtpMessage *pRawMsg, PtpHeader *pHeader) {
    PtpMessageType mt = pHeader->messageType;
    PtpDelayMechanism dm = S.profile.delayMechanism;

    // process non-Announce messages
    if (mt == PTP_MT_Sync || mt == PTP_MT_Follow_Up) {
        switch (S.slave.messaging.m2sState) {
        // wait for Sync message
        case SIdle: {
            // switch into next state if Sync packet has arrived
            if (mt == PTP_MT_Sync) {
                // save sync interval
                S.slave.messaging.logSyncPeriod = pHeader->logMessagePeriod;
                S.slave.messaging.syncPeriodMs = ptp_logi2ms(pHeader->logMessagePeriod);

                // MSG("%d\n", header.logMessagePeriod);

                // save reception time
                S.slave.scd.t[T2] = pRawMsg->ts;

                // switch to next syncState
                S.slave.messaging.sequenceID = pHeader->sequenceID;

                // save correction field
                S.slave.scd.cf[T1] = pHeader->correction_ns;

                // handle two step/one step messaging
                if (pHeader->flags.PTP_TWO_STEP) {
                    S.slave.messaging.m2sState = SWaitFollowUp;
                } else {
                    ptp_extract_timestamps(&S.slave.scd.t[T1], pRawMsg->data, 1); // extract t1
                    S.slave.scd.cf[T2] = 0;                                       // clear Follow_Up correction field, since no Follow_Up is expected
                    ptp_commence_e2e_correction();                                // commence executing the E2E correction
                }

                // dispatch SYNC_RECVED event
                PTP_IUEV(PTP_UEV_SYNC_RECVED);
            }

            break;
        }
            // wait for Follow_Up message
        case SWaitFollowUp:
            if (mt == PTP_MT_Follow_Up) {
                // check sequence ID if the response is ours
                if (pHeader->sequenceID == S.slave.messaging.sequenceID) {
                    ptp_extract_timestamps(&S.slave.scd.t[T1], pRawMsg->data, 1); // read t1
                    S.slave.scd.cf[T2] = pHeader->correction_ns;                  // retain correction field

                    // initiate the correction
                    ptp_commence_e2e_correction();

                    // log correction field (if enabled)
                    CLILOG(S.logging.corr, "C [Follow_Up]: %09lu\n", pHeader->correction_ns);

                    // dispatch FOLLOW_UP_RECVED event
                    PTP_IUEV(PTP_UEV_FOLLOW_UP_RECVED);
                }

                // on ID mismatch, just skip the cycle, and expect a new Sync coming

                // switch to next syncState
                S.slave.messaging.m2sState = SIdle;
            }

            break;
        }
    }

    // ------ (P)DELAY_RESPONSE PROCESSING --------

    // wait for (P)Delay_Resp message
    if (((mt == PTP_MT_Delay_Resp) && (dm == PTP_DM_E2E)) ||
        (((mt == PTP_MT_PDelay_Resp) || (mt == PTP_MT_PDelay_Resp_Follow_Up)) && (dm == PTP_DM_P2P))) {
        if (pHeader->sequenceID == S.slave.messaging.delay_reqSequenceID) { // read clock ID of requester
            PtpDelay_RespIdentification delay_respID;                       // identification received in every Delay_Resp packet

            if (mt == PTP_MT_Delay_Resp) { // Delay_Resp processing

                ptp_read_delay_resp_id_data(&delay_respID, pRawMsg->data);

                // if the response was sent to us as a response to our Delay_Req then continue processing
                if (delay_respID.requestingSourceClockIdentity == S.hwoptions.clockIdentity &&
                    delay_respID.requestingSourcePortIdentity == PTP_PORT_ID) {

                    ptp_extract_timestamps(&S.slave.scd.t[T4], pRawMsg->data, 1); // store t4
                    S.slave.scd.cf[T4] = pHeader->correction_ns;                  // store correction field

                    // compute mean path delay
                    ptp_compute_mean_path_delay_e2e(S.slave.scd.t, S.slave.scd.cf, &S.network.meanPathDelay);

                    // store last response ID
                    S.slave.messaging.lastRespondedDelReqId = pHeader->sequenceID;

                    // perform correction if operating on syncmatched mode
                    if (S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED) {
                        ptp_perform_correction();
                    }

                    // dispatch DELAY_RESP_RECVED event
                    PTP_IUEV(PTP_UEV_DELAY_RESP_RECVED);

                    // log correction field (if enabled)
                    CLILOG(S.logging.corr, "C [Del_Resp]: %09lu\n", pHeader->correction_ns);
                }

            } else if (mt == PTP_MT_PDelay_Resp) {  // PDelay_Resp processing
                TimestampI *pT = &S.slave.scd.t[2]; // skip the first 2 timestamps
                uint64_t *cf = &S.slave.scd.cf[2];  // skip the first 2 correction fields

                pT[T4] = pRawMsg->ts;            // save t4 (P2P)
                cf[T2] = pHeader->correction_ns; // save correction field of the PDelay_Resp

                // if the responder is a one-step clock, then...
                if (!pHeader->flags.PTP_TWO_STEP) {
                    // no t2 and t3 will be involved with the calculations
                    pT[T3] = pT[T2] = zeroTs;
                    ptp_commence_p2p_correction(pHeader->sequenceID); // commence correction
                } else {
                    ptp_extract_timestamps(&(pT[T2]), pRawMsg->data, 1); // retrieve t2 (P2P)
                }

                // dispatch PDELAY_RESP_RECVED event
                PTP_IUEV(PTP_UEV_PDELAY_RESP_RECVED);

                // log correction field (if enabled)
                CLILOG(S.logging.corr, "C [PDel_Resp]: %09lu\n", pHeader->correction_ns);

            } else if (mt == PTP_MT_PDelay_Resp_Follow_Up) { // PDelay_Resp_Follow_Up processing
                ptp_read_delay_resp_id_data(&delay_respID, pRawMsg->data);

                // if sent to us as a response to our Delay_Req then continue processing
                if (delay_respID.requestingSourceClockIdentity == S.hwoptions.clockIdentity &&
                    delay_respID.requestingSourcePortIdentity == PTP_PORT_ID) {

                    TimestampI *pT = &S.slave.scd.t[2]; // skip the first 2 timestamps
                    uint64_t *cf = &S.slave.scd.cf[2];  // skip the first 2 correction fields

                    ptp_extract_timestamps(&(pT[T3]), pRawMsg->data, 1); // retrieve t3 (P2P)
                    cf[T3] = pHeader->correction_ns;                     // retain correction field from the PDelay_Resp_Follow_Up

                    // commence correction
                    ptp_commence_p2p_correction(pHeader->sequenceID);

                    // dispatch PDELAY_RESP_FOLLOW_UP_RECVED event
                    PTP_IUEV(PTP_UEV_PDELAY_RESP_FOLLOW_UP_RECVED);

                    // log correction field (if enabled)
                    CLILOG(S.logging.corr, "C [PDel_Resp_Follow_Up]: %09lu\n", pHeader->correction_ns);
                }
            }
        }
    }
}

// ------------------------

void ptp_slave_init() {
    // initialize coarse threshold
    ptp_set_coarse_threshold(PTP_DEFAULT_COARSE_TRIGGER_NS);

    // reset the slave module
    ptp_slave_reset();
}

void ptp_slave_destroy() {
    return;
}

void ptp_slave_reset() {
    // disable slave module
    S.slave.enabled = false;

    // clear Sync cycle data
    memset(&S.slave.scd, 0, sizeof(PtpSyncCycleData));
    S.slave.prevSyncMa = zeroTs;
    S.slave.prevTimeError = zeroTs;

    // reset messaging state
    memset(&S.slave.messaging, 0, sizeof(PtpSlaveMessagingState));

    // reset addend/tuning
#ifdef PTP_ADDEND_INTERFACE
    S.hwclock.addend = PTP_ADDEND_INIT; // HW clock state
    PTP_SET_ADDEND(S.hwclock.addend);
#elif defined(PTP_HLT_INTERFACE)
    S.hwclock.tuning_ppb = 0.0;
    PTP_SET_TUNING(0.0);
#endif

    // reset the controller
    PTP_SERVO_RESET();

    // reset fast correction state
    S.slave.fastCompState = PTP_FC_IDLE;
    S.slave.fastCompCntr = 0;
}

void ptp_slave_tick() {
    if (!S.slave.enabled) {
        return;
    }

    // Delay_Req transmission
    if (S.profile.logDelayReqPeriod != PTP_LOGPER_SYNCMATCHED) {
        if (++S.slave.delReqTmr > S.slave.delReqTickPeriod) {
            S.slave.delReqTmr = 0;

            // check that our last Delay_Req has been responded
            if (S.profile.logDelayReqPeriod != PTP_LOGPER_SYNCMATCHED) {
                if (S.slave.messaging.delay_reqSequenceID != S.slave.messaging.lastRespondedDelReqId) {
                    CLILOG(S.logging.info, "(P)Del_Req #%d: no response received!\n", S.slave.messaging.delay_reqSequenceID);
                    PTP_IUEV(PTP_UEV_NETWORK_ERROR); // dispatch network error event
                }

                // transmit (P)Delay_Req message
                ptp_send_delay_req_message();

                // dispatch (P)DELAY_REQ_SENT message
                PTP_IUEV((S.profile.delayMechanism == PTP_DM_E2E) ? PTP_UEV_DELAY_REQ_SENT : PTP_UEV_PDELAY_REQ_SENT);
            }
        }
    }
}

void ptp_slave_enable() {
    S.slave.enabled = true;

    if (S.profile.logDelayReqPeriod != PTP_LOGPER_SYNCMATCHED) {
        S.slave.delReqTickPeriod = ptp_logi2ms(S.profile.logDelayReqPeriod) / PTP_HEARTBEAT_TICKRATE_MS;
    }
}

void ptp_slave_disable() {
    S.slave.enabled = false;
}
