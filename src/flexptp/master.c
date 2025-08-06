#include "master.h"

#include "common.h"
#include "flexptp_options.h"
#include "format_utils.h"
#include "msg_utils.h"
#include "ptp_core.h"
#include "ptp_defs.h"
#include "ptp_profile_presets.h"
#include "ptp_sync_cycle_data.h"
#include "ptp_types.h"
#include "task_ptp.h"
#include "timeutils.h"
#include "tlv.h"

#include <string.h>

#include "minmax.h"

///\cond 0
#define S (gPtpCoreState)
///\endcond

/*
 * Message headaers and compiled bodies. They can be statically
 * allocated, since nothing depends on the Announce message
 * and a Follow_Up is always triggered by the transmission of
 * a Sync. TODO: Maybe Sync transmissions should be constrained,
 * so that no new Sync transmission would be scheduled if the Follow_Up
 * was in the queue.
 */
static PtpHeader announceHeader;
static RawPtpMessage announce;
static PtpHeader syncHeader;
static RawPtpMessage sync;
static RawPtpMessage followUp;
static const PtpProfileTlvElement *tlvChain;

static void ptp_init_announce_header() {
    announceHeader.messageType = PTP_MT_Announce;
    announceHeader.transportSpecific = (uint8_t)S.profile.transportSpecific;
    announceHeader.versionPTP = 2;
    announceHeader.domainNumber = S.profile.domainNumber;
    // announceHeader.flags.PTP_TWO_STEP = true;
    announceHeader.flags.PTP_TIMESCALE = true;
    announceHeader.correction_ns = 0;
    announceHeader.correction_subns = 0;

    memcpy(&announceHeader.clockIdentity, &S.hwoptions.clockIdentity, 8);

    announceHeader.sourcePortID = PTP_PORT_ID;
    announceHeader.control = PTP_CON_Other;
    announceHeader.logMessagePeriod = S.profile.logAnnouncePeriod;

    // insert TLVs from the profile
    uint16_t tlvSize = ptp_tlv_insert(announce.data + PTP_PCKT_SIZE_ANNOUNCE,
                                      tlvChain,
                                      PTP_MT_Announce,
                                      MAX_PTP_MSG_SIZE - PTP_PCKT_SIZE_ANNOUNCE);

    announce.size = PTP_PCKT_SIZE_ANNOUNCE + tlvSize;
    announceHeader.messageLength = announce.size;
}

static void ptp_init_sync_header() {
    syncHeader.transportSpecific = S.profile.transportSpecific;
    syncHeader.messageType = PTP_MT_Sync;
    syncHeader.transportSpecific = (uint8_t)S.profile.transportSpecific;
    syncHeader.versionPTP = 2;
    syncHeader.domainNumber = S.profile.domainNumber;
    syncHeader.flags.PTP_TWO_STEP = true;
    // syncHeader.flags.PTP_TIMESCALE = false;
    syncHeader.correction_ns = 0;
    syncHeader.correction_subns = 0;

    memcpy(&syncHeader.clockIdentity, &S.hwoptions.clockIdentity, 8);

    syncHeader.sourcePortID = PTP_PORT_ID;
    syncHeader.control = PTP_CON_Sync;
    syncHeader.logMessagePeriod = S.profile.logSyncPeriod;

    // insert TLVs from the profile
    uint16_t tlvSize = ptp_tlv_insert(announce.data + PTP_PCKT_SIZE_SYNC,
                                      tlvChain,
                                      PTP_MT_Sync,
                                      MAX_PTP_MSG_SIZE - PTP_PCKT_SIZE_SYNC);

    // save message sizes
    sync.size = PTP_PCKT_SIZE_SYNC + tlvSize;
    syncHeader.messageLength = sync.size;
}

static void ptp_init_follow_up_message() {
    // insert TLVs from the profile
    uint16_t tlvSize = ptp_tlv_insert(followUp.data + PTP_PCKT_SIZE_FOLLOW_UP,
                                      tlvChain,
                                      PTP_MT_Follow_Up,
                                      MAX_PTP_MSG_SIZE - PTP_PCKT_SIZE_FOLLOW_UP);
    followUp.size = PTP_PCKT_SIZE_FOLLOW_UP + tlvSize;
}

static void ptp_send_announce_message() {
    PtpHeader header;

    // set sequence ID
    announceHeader.sequenceID = S.master.messaging.announceSequenceID++;
    announceHeader.transportSpecific = S.profile.transportSpecific;

    // fill-in fields
    ptp_construct_binary_header(announce.data, &announceHeader);           // insert header
    ptp_write_binary_timestamps(announce.data, &zeroTs, 1);                // insert an empty timestamp
    ptp_construct_binary_announce_message(announce.data, &S.capabilities); // insert Announce body

    // setup packet
    announce.tag = RPMT_RANDOM;
    announce.pTxCb = NULL;
    announce.tx_dm = S.profile.delayMechanism;
    announce.tx_mc = PTP_MC_GENERAL;

    // send message
    ptp_transmit_enqueue(&announce);
}

static void ptp_send_follow_up(const RawPtpMessage *pMsg) {
    // fetch header from preceding Sync
    PtpHeader header;
    ptp_extract_header(&header, pMsg->data);
    TimestampI t1 = pMsg->ts;

    // modify header fields
    header.transportSpecific = S.profile.transportSpecific;
    header.messageType = PTP_MT_Follow_Up;
    header.messageLength = followUp.size;
    header.control = PTP_CON_Follow_Up;
    // ptp_clear_flags(&(header.flags));

    // write fields
    ptp_construct_binary_header(followUp.data, &header); // insert header
    ptp_write_binary_timestamps(followUp.data, &t1, 1);  // insert t1 timestamp

    // setup packet
    followUp.tag = RPMT_RANDOM;
    followUp.pTxCb = NULL;
    followUp.tx_dm = S.profile.delayMechanism;
    followUp.tx_mc = PTP_MC_GENERAL;

    // transmit
    ptp_transmit_enqueue(&followUp);
}

static void ptp_send_sync_message() {
    PtpHeader header;

    // set sequence ID
    syncHeader.sequenceID = S.master.messaging.syncSequenceID++;

    // fill-in fields
    ptp_construct_binary_header(sync.data, &syncHeader); // insert header
    ptp_write_binary_timestamps(sync.data, &zeroTs, 1);  // insert an empty timestamp (TWO_STEP -> "reserved")

    // setup packet
    sync.tag = RPMT_SYNC;
    sync.pTxCb = ptp_send_follow_up;
    sync.tx_dm = S.profile.delayMechanism;
    sync.tx_mc = PTP_MC_EVENT;

    // send message
    ptp_transmit_enqueue(&sync);
}

static void ptp_send_delay_resp_message(const RawPtpMessage *pRawMsg, const PtpHeader *pHeader) {
    RawPtpMessage delRespMsg = {0};

    PtpHeader header = *pHeader; // make a copy
    TimestampI t4 = pRawMsg->ts; // fetch t4 timestamp

    // create requestingSourcePortIdentity based on clockId from the header
    PtpDelay_RespIdentification reqDelRespId = {header.clockIdentity, header.sourcePortID};

    // modify header fields
    header.messageType = PTP_MT_Delay_Resp;           // change message type
    ptp_clear_flags(&header.flags);                   // clear flags
    header.flags.PTP_TWO_STEP = true;                 // set TWO_STEP flag
    header.sourcePortID = PTP_PORT_ID;                // set source port number
    header.messageLength = PTP_PCKT_SIZE_DELAY_RESP;  // set appropriate size
    header.clockIdentity = S.hwoptions.clockIdentity; // set our clock identity
    header.control = PTP_CON_Delay_Resp;

    // write fields
    ptp_construct_binary_header(delRespMsg.data, &header);        // HEADER
    ptp_write_binary_timestamps(delRespMsg.data, &t4, 1);         // t4 TIMESTAMP
    ptp_write_delay_resp_id_data(delRespMsg.data, &reqDelRespId); // REQ.SRC.PORT.ID

    // setup packet
    delRespMsg.tag = RPMT_RANDOM;
    delRespMsg.size = PTP_PCKT_SIZE_DELAY_RESP;
    delRespMsg.pTxCb = NULL;
    delRespMsg.tx_dm = PTP_DM_E2E;
    delRespMsg.tx_mc = PTP_MC_GENERAL;

    // send packet
    ptp_transmit_enqueue(&delRespMsg);
}

// ------------------------

static char *P2P_SLAVE_STATE_HINTS[] = {
    "NONE",
    "CANDIDATE",
    "ESTABLISHED"};

#define PTP_MASTER_P2P_SLAVE_STATE_LOG() \
    CLILOG(S.logging.def && (si->state != prevState), "%s -> %s\n", P2P_SLAVE_STATE_HINTS[prevState], P2P_SLAVE_STATE_HINTS[si->state])

/**
 * Function to track what's going on with the slave.
 *
 * @param slClockId clock ID of the slave that responded to our PDelay_Req message
 */
static void ptp_master_p2p_slave_reported(uint64_t slClockId) {
    PtpP2PSlaveInfo *si = &(S.master.p2pSlave);
    PtpP2PSlaveState prevState = si->state;

    switch (si->state) {
    case PTP_P2PSS_NONE:
        si->reportCount = 0;                  // increase the report count
        si->dropoutCntr = PTP_PDELAY_DROPOUT; // reload the dropout counter
        si->identity = slClockId;             // save the clock ID
        si->state = PTP_P2PSS_CANDIDATE;      // switch to candidate state
        break;
    case PTP_P2PSS_CANDIDATE:
    case PTP_P2PSS_ESTABLISHED:
        if (si->identity == slClockId) {                                                    // if the slave candidate keep reporting in...
            si->dropoutCntr = PTP_PDELAY_DROPOUT;                                           // reload the counter
            si->reportCount = MIN(PTP_PDELAY_SLAVE_QUALIFICATION + 1, si->reportCount + 1); // increase the report count
            if (si->reportCount > PTP_PDELAY_SLAVE_QUALIFICATION) {                         // switch to ESTABLISHED if qualification has passed
                si->state = PTP_P2PSS_ESTABLISHED;
            }
        } else {                        // if a different slave has also appeared, then it's either a fault, or the previous one had been replaced
            si->state = PTP_P2PSS_NONE; // revert to NONE state
            si->identity = 0;           // clear the identity
            si->dropoutCntr = 0;        // clear the dropout counter
            si->reportCount = 0;        // clear the report counter
        }
        break;
    }

    PTP_MASTER_P2P_SLAVE_STATE_LOG();
}

// /**
//  * Calculate the mean path delay.
//  */
// static void ptp_master_calculate_mean_path_delay() {
//     // fetch value arrays
//     TimestampI * pMPD = &S.network.meanPathDelay;
//     uint64_t * pCf = S.master.scd.cf;

//     // convert correction field values to timestamp objects
//     TimestampI * pTs = S.master.scd.t;
//     TimestampI cf = {0, 0}; // variable for accounting correction fields in
//     nsToTsI(&cf, pCf[T2] + pCf[T3]);

//     // compute difference between master and slave clock
//     subTime(pMPD, &pTs[T4], &pTs[T1]); // t4 - t1 ...
//     subTime(pMPD, pMPD, &pTs[T3]);     // - t3 ...
//     addTime(pMPD, pMPD, &pTs[T2]);     // + t2
//     subTime(pMPD, pMPD, &cf);          // - CF of (PDelay_Resp + PDelay_Resp_Follow_Up)
//     divTime(pMPD, pMPD, 2);            // division by 2
// }

static void ptp_master_commence_mpd_computation() {
    PtpSyncCycleData *scd = &S.master.scd;
    TimestampI *mpd = &S.network.meanPathDelay;
    ptp_compute_mean_path_delay_p2p(scd->t, scd->cf, mpd);

    CLILOG(S.logging.timestamps,
           "seqID: %u\n"
           "T1: %d.%09d <- PDelay_Req TX (master)\n"
           "T2: %d.%09d <- PDelay_Req RX (slave) \n"
           "T3: %d.%09d <- PDelay_Resp TX (slave) \n"
           "T4: %d.%09d <- PDelay_Resp RX (master)\n"
           "    %09lu -- %09lu <- CF in PDelay_Resp and ..._Follow_Up\n\n",
           (uint32_t)S.master.pdelay_reqSequenceID,
           (int32_t)scd->t[T1].sec, scd->t[T1].nanosec,
           (int32_t)scd->t[T2].sec, scd->t[T2].nanosec,
           (int32_t)scd->t[T3].sec, scd->t[T3].nanosec,
           (int32_t)scd->t[T4].sec, scd->t[T4].nanosec,
           scd->cf[T2], scd->cf[T3]);

    CLILOG(S.logging.def, "%ld\n", nsI(mpd));
}

void ptp_master_process_message(RawPtpMessage *pRawMsg, PtpHeader *pHeader) {
    PtpMessageType mt = pHeader->messageType;
    PtpDelayMechanism dm = S.profile.delayMechanism;

    if ((mt == PTP_MT_Delay_Req) && (dm == PTP_DM_E2E)) { // pass only Delay_Req through E2E delay mechanism
        // dispatch DELAY_REQ_RECVED user event
        PTP_IUEV(PTP_UEV_DELAY_REQ_RECVED);

        // send Delay_Resp message
        ptp_send_delay_resp_message(pRawMsg, pHeader);

        // dispatch DELAY_RESP_SENT user event
        PTP_IUEV(PTP_UEV_DELAY_RESP_SENT);

    } else if (((mt == PTP_MT_PDelay_Resp) || (mt == PTP_MT_PDelay_Resp_Follow_Up)) && (dm == PTP_DM_P2P)) { // let PDelay_Resp and PDelay_Resp_Follow_Up through in P2P mode
        // try fetching PDelay_Req timestamp
        if ((mt == PTP_MT_PDelay_Resp) && (!ptp_read_and_clear_transmit_timestamp(RPMT_DELAY_REQ, &S.master.scd.t[T1]))) {
            return;
        }

        PtpDelay_RespIdentification delay_respID;                                                            // acquire PDelay_Resp(_Follow_Up) identification info
        ptp_read_delay_resp_id_data(&delay_respID, pRawMsg->data);

        if ((delay_respID.requestingSourceClockIdentity == S.hwoptions.clockIdentity) &&
            (delay_respID.requestingSourcePortIdentity == PTP_PORT_ID) && (pHeader->sequenceID == S.master.pdelay_reqSequenceID)) {
            PtpSyncCycleData *scd = &S.master.scd;

            if (mt == PTP_MT_PDelay_Resp) {
                if (S.master.p2pSlave.state != PTP_P2PSS_NONE) {
                    ptp_extract_timestamps(&(scd->t[T2]), pRawMsg->data, 1); // extract PDelay_Req reception time
                    scd->cf[T2] = pHeader->correction_ns;                    // correction field of the PDelay_Resp
                    scd->t[T4] = pRawMsg->ts;                                // save PDelay_Resp reception time
                }

                if (!pHeader->flags.PTP_TWO_STEP) {
                    ptp_master_p2p_slave_reported(pHeader->clockIdentity);

                    // mean path delay calculation
                    if (S.master.p2pSlave.state != PTP_P2PSS_NONE) { // one-step mode responder
                        scd->t[T2] = scd->t[T3] = zeroTs;
                        scd->cf[T3] = 0;
                        ptp_master_commence_mpd_computation();
                    }

                    // dispatch user event
                    PTP_IUEV(PTP_UEV_PDELAY_RESP_RECVED);
                } else { // expect a ...FollowUp coming in two step mode 
                    S.master.expectPDelRespFollowUp = true;
                }
            } else if (mt == PTP_MT_PDelay_Resp_Follow_Up) {
                if (!S.master.expectPDelRespFollowUp) {
                    return;
                }
                S.master.expectPDelRespFollowUp = false;

                if (S.master.p2pSlave.state != PTP_P2PSS_NONE) {
                    ptp_extract_timestamps(&(scd->t[T3]), pRawMsg->data, 1); // extract PDelay_Resp transmission time
                    scd->cf[T3] = pHeader->correction_ns;                    // correction field of the PDelay_Resp_Follow_Up
                    ptp_master_commence_mpd_computation();
                }

                ptp_master_p2p_slave_reported(pHeader->clockIdentity);

                // dispatch user event
                PTP_IUEV(PTP_UEV_PDELAY_RESP_FOLLOW_UP_RECVED);
            }
        }
    }
}

// ------------------------

void ptp_master_init() {
    // reset master module
    ptp_master_reset();
}

void ptp_master_destroy() {
    return;
}

void ptp_master_reset() {
    // load the TLV chain based on the profile
    tlvChain = ptp_tlv_chain_preset_get(S.profile.tlvSet);

    // initialize Announce message header
    ptp_init_announce_header();

    // initialize Sync message header
    ptp_init_sync_header();

    // initialze Follow_Up message
    ptp_init_follow_up_message();

    // disable the module
    S.master.enabled = false;

    // don't expect a PDelay_Req_Follow_Up coming
    S.master.expectPDelRespFollowUp = false;

    // clear the messaging state
    memset(&S.master.messaging, 0, sizeof(PtpMasterMessagingState));
}

void ptp_master_enable() {
    // enable the module
    S.master.enabled = true;

    // calculate periods
    S.master.syncTickPeriod = ptp_logi2ms(S.profile.logSyncPeriod) / PTP_HEARTBEAT_TICKRATE_MS;
    S.master.announceTickPeriod = ptp_logi2ms(S.profile.logAnnouncePeriod) / PTP_HEARTBEAT_TICKRATE_MS;
    S.master.pdelayReqTickPeriod = ptp_logi2ms((S.profile.logDelayReqPeriod == PTP_LOGPER_SYNCMATCHED) ? S.profile.logSyncPeriod : S.profile.logDelayReqPeriod) / PTP_HEARTBEAT_TICKRATE_MS;

    // clear counters
    S.master.syncTmr = 0;
    S.master.announceTmr = 0;
    S.master.pdelayReqTmr = 0;

    // reset PDelay_Request's sequence ID
    S.master.pdelay_reqSequenceID = 0;

    // clear Sync cycle data
    memset(&S.master.scd, 0, sizeof(PtpSyncCycleData));

    // clear P2P slave information
    memset(&S.master.p2pSlave, 0, sizeof(PtpP2PSlaveInfo));
}

void ptp_master_disable() {
    S.master.enabled = false;
}

void ptp_master_tick() {
    if (!S.master.enabled) {
        return;
    }

    PtpDelayMechanism dm = S.profile.delayMechanism; // fetch Delay Mechanism

    // issue PDelay_Req messages
    if (dm == PTP_DM_P2P) {
        if (++S.master.pdelayReqTmr >= S.master.pdelayReqTickPeriod) {
            S.master.pdelayReqTmr = 0;

            PtpP2PSlaveInfo *si = &(S.master.p2pSlave);
            PtpP2PSlaveState prevState = si->state;
            si->dropoutCntr = (si->dropoutCntr > 0) ? (si->dropoutCntr - 1) : 0; // decrease the slave dropout counter
            if (si->dropoutCntr == 0) {
                si->state = PTP_P2PSS_NONE;
                si->identity = 0;
            }
            PTP_MASTER_P2P_SLAVE_STATE_LOG();

            ptp_send_delay_req_message(); // send a PDelay_Request message

            // dispatch PDELAY_REQUEST_SENT message
            PTP_IUEV(PTP_UEV_PDELAY_REQ_SENT);
        }
    }

    // gating signal for Sync and Announce transmission
    bool infoEn = (dm == PTP_DM_E2E) || ((dm == PTP_DM_P2P) && ((S.master.p2pSlave.state == PTP_P2PSS_ESTABLISHED) || (!(S.profile.flags & PTP_PF_ISSUE_SYNC_FOR_COMPLIANT_SLAVE_ONLY_IN_P2P))));

    if (infoEn) {
        // Sync transmission
        if (++S.master.syncTmr >= S.master.syncTickPeriod) {
            S.master.syncTmr = 0;

            ptp_send_sync_message();

            PTP_IUEV(PTP_UEV_SYNC_SENT);
        }

        // Announce transmission
        if (++S.master.announceTmr >= S.master.announceTickPeriod) {
            S.master.announceTmr = 0;

            ptp_send_announce_message();

            PTP_IUEV(PTP_UEV_ANNOUNCE_SENT);
        }
    }
}