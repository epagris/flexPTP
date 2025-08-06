#include "common.h"

#include "ptp_core.h"
#include "task_ptp.h"

#include "msg_utils.h"
#include "ptp_defs.h"
#include "ptp_types.h"

///\cond 0
#define S (gPtpCoreState)
///\endcond

static PtpHeader delReqHeader; ///< header for sending Delay_Reg messages
static RawPtpMessage pdelRespMsg; ///< whole, compiled PDelay_Resp message
static RawPtpMessage pdelRespFUpMsg; ///< whole, compiled PDelay_Resp_Follow_Up message

// ------------------

void ptp_init_delay_req_header() {
    delReqHeader.messageType = (S.profile.delayMechanism == PTP_DM_E2E) ? PTP_MT_Delay_Req : PTP_MT_PDelay_Req;
    delReqHeader.transportSpecific = (uint8_t)S.profile.transportSpecific;
    delReqHeader.versionPTP = 2; // PTPv2
    delReqHeader.messageLength = PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH +
                                ((S.profile.delayMechanism == PTP_DM_P2P) ? PTP_TIMESTAMP_LENGTH : 0);
    delReqHeader.domainNumber = S.profile.domainNumber;
    ptp_clear_flags(&(delReqHeader.flags)); // no flags
    delReqHeader.correction_ns = 0;
    delReqHeader.correction_subns = 0;

    memcpy(&delReqHeader.clockIdentity, &S.hwoptions.clockIdentity, 8);

    delReqHeader.sourcePortID = PTP_PORT_ID;
    delReqHeader.sequenceID = 0; // will increase in every sync cycle
    delReqHeader.control = (S.profile.delayMechanism == PTP_DM_E2E) ? PTP_CON_Delay_Req : PTP_CON_Other;
    delReqHeader.logMessagePeriod = 0;
}

void ptp_send_delay_req_message() {
    // PTP message
    RawPtpMessage delReqMsg = {0};
    delReqMsg.tag = RPMT_DELAY_REQ;
    delReqMsg.size = delReqHeader.messageLength;
    //delReqMsg.pTs = (S.bmca.state == PTP_BMCA_SLAVE) ? (&(S.slave.scd.t[T3])) : (&(S.master.scd.t[T1])); // timestamp writeback address
    delReqMsg.tx_dm = S.profile.delayMechanism;
    delReqMsg.tx_mc = PTP_MC_EVENT;

    // increment sequenceID
    delReqHeader.sequenceID = (S.bmca.state == PTP_BMCA_SLAVE) ? (++S.slave.messaging.delay_reqSequenceID) : (++S.master.pdelay_reqSequenceID);
    delReqHeader.domainNumber = S.profile.domainNumber;

    // fill in header
    ptp_construct_binary_header(delReqMsg.data, &delReqHeader);

    // fill in timestamp
    ptp_write_binary_timestamps(delReqMsg.data, &zeroTs, 1);

    // send message
    ptp_transmit_enqueue(&delReqMsg);
}

void ptp_send_pdelay_resp_follow_up(const RawPtpMessage *pMsg) {
    // fetch header from sent PDelay_Resp message
    PtpHeader header;
    ptp_extract_header(&header, pMsg->data);
    TimestampI t3 = pMsg->ts;

    // modify header fields
    header.messageType = PTP_MT_PDelay_Resp_Follow_Up; // change message type
    ptp_clear_flags(&header.flags);                    // clear flags
    header.messageLength = PTP_PCKT_SIZE_PDELAY_RESP_FOLLOW_UP;

    // write fields
    ptp_construct_binary_header(pdelRespFUpMsg.data, &header); // HEADER
    ptp_write_binary_timestamps(pdelRespFUpMsg.data, &t3, 1);  // t3 TIMESTAMP
    uint32_t reqPortIdOffset = PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH;
    memcpy(pdelRespFUpMsg.data + reqPortIdOffset, pMsg->data + reqPortIdOffset, PTP_PORT_ID_LENGTH);

    // setup packet
    pdelRespFUpMsg.tag = RPMT_RANDOM;
    pdelRespFUpMsg.size = PTP_PCKT_SIZE_PDELAY_RESP_FOLLOW_UP;
    pdelRespFUpMsg.tx_dm = PTP_DM_P2P;
    pdelRespFUpMsg.tx_mc = PTP_MC_GENERAL;
    pdelRespFUpMsg.pTxCb = NULL;

    // MSG("PDelRespFollowUp: %d.%09d\n", (int32_t) t3.sec, t3.nanosec);

    // send!
    ptp_transmit_enqueue(&pdelRespFUpMsg);
}

void ptp_send_pdelay_resp(const RawPtpMessage *pMsg) {
    // fetch header from received PDelay_Req message
    PtpHeader header;
    TimestampI t2 = pMsg->ts;
    ptp_extract_header(&header, pMsg->data);
    header.minorVersionPTP = 0; // minorVersionPTP should be fixed to 0

    // create requestingSourcePortIdentity based on clockId from the header
    PtpDelay_RespIdentification reqDelRespId = {header.clockIdentity, header.sourcePortID};

    // modify header fields
    header.messageType = PTP_MT_PDelay_Resp; // change message type
    ptp_clear_flags(&header.flags);          // set flags
    header.flags.PTP_TWO_STEP = true;
    header.sourcePortID = PTP_PORT_ID;                // set source port number
    header.logMessagePeriod = 0x7F;                   // see standard...
    header.messageLength = PTP_PCKT_SIZE_PDELAY_RESP; // set appropriate size
    header.clockIdentity = S.hwoptions.clockIdentity; // set our clock identity

    // write fields
    ptp_construct_binary_header(pdelRespMsg.data, &header);        // HEADER
    ptp_write_binary_timestamps(pdelRespMsg.data, &t2, 1);         // t2 TIMESTAMP
    ptp_write_delay_resp_id_data(pdelRespMsg.data, &reqDelRespId); // REQ.SRC.PORT.ID

    // setup packet
    pdelRespMsg.tag = RPMT_RANDOM;
    pdelRespMsg.size = PTP_PCKT_SIZE_PDELAY_RESP;
    pdelRespMsg.pTxCb = ptp_send_pdelay_resp_follow_up;
    pdelRespMsg.tx_dm = PTP_DM_P2P;
    pdelRespMsg.tx_mc = PTP_MC_EVENT;

    // MSG("PDelResp: %d.%09d\n", (int32_t)t2.sec, t2.nanosec);

    // send packet
    ptp_transmit_enqueue(&pdelRespMsg);
}

void ptp_compute_mean_path_delay_e2e(const TimestampI *pTs, const uint64_t *pCf, TimestampI *pMPD) {
    // convert correction field values to timestamp objects
    TimestampI cf = {0, 0}; // variable for accounting correction fields in
    nsToTsI(&cf, pCf[T1] + pCf[T2] + pCf[T4]);

    // compute difference between master and slave clock
    subTime(pMPD, &pTs[T2], &pTs[T1]); // t2 - t1 ...
    subTime(pMPD, pMPD, &pTs[T3]);     // - t3 ...
    addTime(pMPD, pMPD, &pTs[T4]);     // + t4
    subTime(pMPD, pMPD, &cf);          // - CF of (Sync + Follow_Up + Delay_Resp)
    divTime(pMPD, pMPD, 2);            // division by 2
}

void ptp_compute_mean_path_delay_p2p(const TimestampI *pTs, const uint64_t *pCf, TimestampI *pMPD) {
    // convert correction field values to timestamp objects
    TimestampI cf = {0, 0}; // variable for accounting correction fields in
    nsToTsI(&cf, pCf[T2] + pCf[T3]);

    // compute difference between master and slave clock
    subTime(pMPD, &pTs[T4], &pTs[T1]); // t4 - t1 ...
    subTime(pMPD, pMPD, &pTs[T3]);     // - t3 ...
    addTime(pMPD, pMPD, &pTs[T2]);     // + t2
    subTime(pMPD, pMPD, &cf);          // - CF of (PDelay_Resp + PDelay_Resp_Follow_Up)
    divTime(pMPD, pMPD, 2);            // division by 2
}

// --------------

void ptp_common_reset() {
    ptp_init_delay_req_header();
}