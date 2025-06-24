/* (C) András Wiesner, 2020-2022 */

#include "ptp_types.h"
#include "format_utils.h"

#include <string.h>

#include "msg_utils.h"
#include "ptp_defs.h"

// load ptp flags from bitfield
void ptp_load_flags(PtpFlags *pFlags, uint16_t bitfield) {
///\cond 0
#define GET_FLAG_FROM_BITFIELD(flag, n) (pFlags->flag) = (bitfield >> (n)) & 1
    ///\endcond

    GET_FLAG_FROM_BITFIELD(PTP_SECURITY, 15);
    GET_FLAG_FROM_BITFIELD(PTP_ProfileSpecific_2, 14);
    GET_FLAG_FROM_BITFIELD(PTP_ProfileSpecific_1, 13);
    GET_FLAG_FROM_BITFIELD(PTP_UNICAST, 10);
    GET_FLAG_FROM_BITFIELD(PTP_TWO_STEP, 9);
    GET_FLAG_FROM_BITFIELD(PTP_ALTERNATE_MASTER, 8);
    GET_FLAG_FROM_BITFIELD(FREQUENCY_TRACEABLE, 7);
    GET_FLAG_FROM_BITFIELD(TIME_TRACEABLE, 4);
    GET_FLAG_FROM_BITFIELD(PTP_TIMESCALE, 3);
    GET_FLAG_FROM_BITFIELD(PTP_UTC_REASONABLE, 2);
    GET_FLAG_FROM_BITFIELD(PTP_LI_59, 1);
    GET_FLAG_FROM_BITFIELD(PTP_LI_61, 0);
}

// write flags to bitfield
uint16_t ptp_write_flags(const PtpFlags *pFlags) {
///\cond 0
#define SET_BIT_IN_FLAG_BITFIELD(flag, n) bitfield |= (pFlags->flag) ? (1 << (n)) : 0
    ///\endcond

    uint16_t bitfield = 0;
    SET_BIT_IN_FLAG_BITFIELD(PTP_SECURITY, 15);
    SET_BIT_IN_FLAG_BITFIELD(PTP_ProfileSpecific_2, 14);
    SET_BIT_IN_FLAG_BITFIELD(PTP_ProfileSpecific_1, 13);
    SET_BIT_IN_FLAG_BITFIELD(PTP_UNICAST, 10);
    SET_BIT_IN_FLAG_BITFIELD(PTP_TWO_STEP, 9);
    SET_BIT_IN_FLAG_BITFIELD(PTP_ALTERNATE_MASTER, 8);
    SET_BIT_IN_FLAG_BITFIELD(FREQUENCY_TRACEABLE, 7);
    SET_BIT_IN_FLAG_BITFIELD(TIME_TRACEABLE, 4);
    SET_BIT_IN_FLAG_BITFIELD(PTP_TIMESCALE, 3);
    SET_BIT_IN_FLAG_BITFIELD(PTP_UTC_REASONABLE, 2);
    SET_BIT_IN_FLAG_BITFIELD(PTP_LI_59, 1);
    SET_BIT_IN_FLAG_BITFIELD(PTP_LI_61, 0);

    return bitfield;
}

// extract fields from a PTP header
void ptp_extract_header(PtpHeader *pHeader, const void *pPayload) {
    // cast header to byte accessible form
    uint8_t *p = (uint8_t *)pPayload;

    uint16_t flags;

    // copy header fields
    memcpy(&pHeader->messageType, p + 0, 1);
    memcpy(&pHeader->versionPTP, p + 1, 1);
    memcpy(&pHeader->messageLength, p + 2, 2);
    memcpy(&pHeader->domainNumber, p + 4, 1);
    memcpy(&flags, p + 6, 2);
    memcpy(&pHeader->correction_ns, p + 8, 8);
    memcpy(&pHeader->clockIdentity, p + 20, 8);
    memcpy(&pHeader->sourcePortID, p + 28, 2);
    memcpy(&pHeader->sequenceID, p + 30, 2);
    memcpy(&pHeader->control, p + 32, 1);
    memcpy(&pHeader->logMessagePeriod, p + 33, 1);

    pHeader->transportSpecific = (0xf0 & pHeader->messageType) >> 4;
    pHeader->messageType &= 0x0f;

    // read flags
    ptp_load_flags(&pHeader->flags, FLEXPTP_ntohs(flags));

    // read correction field
    pHeader->correction_subns = FLEXPTP_ntohll(pHeader->correction_ns) & 0xffff;
    pHeader->correction_ns = FLEXPTP_ntohll(pHeader->correction_ns) >> 16;

    pHeader->messageLength = FLEXPTP_ntohs(pHeader->messageLength);
    pHeader->sourcePortID = FLEXPTP_ntohs(pHeader->sourcePortID);
    pHeader->sequenceID = FLEXPTP_ntohs(pHeader->sequenceID);
}

// construct binary header from header structure
void ptp_construct_binary_header(void *pData, const PtpHeader *pHeader) {
    uint8_t *p = (uint8_t *)pData;
    uint8_t firstByte, secondByte;

    // host->network
    uint16_t messageLength = FLEXPTP_htons(pHeader->messageLength);
    uint16_t sourcePortID = FLEXPTP_htons(pHeader->sourcePortID);
    uint16_t sequenceID = FLEXPTP_htons(pHeader->sequenceID);

    // fill in flags
    uint16_t flags = FLEXPTP_htons(ptp_write_flags(&pHeader->flags)); // convert from header fields

    // fill in correction value
    uint64_t correction = FLEXPTP_htonll((pHeader->correction_ns << 16) | (pHeader->correction_subns));

    // copy fields
    firstByte = (pHeader->transportSpecific << 4) | (pHeader->messageType & 0x0f);
    memcpy(p, &firstByte, 1);
    secondByte = (pHeader->minorVersionPTP << 4) | (pHeader->versionPTP & 0x0f);
    memcpy(p + 1, &secondByte, 1);
    memcpy(p + 2, &messageLength, 2);
    memcpy(p + 4, &pHeader->domainNumber, 1);
    memcpy(p + 6, &flags, 2);
    memcpy(p + 8, &correction, 8);
    memcpy(p + 20, &pHeader->clockIdentity, 8);
    memcpy(p + 28, &sourcePortID, 2);
    memcpy(p + 30, &sequenceID, 2);
    memcpy(p + 32, &pHeader->control, 1);
    memcpy(p + 33, &pHeader->logMessagePeriod, 1);
}

// extract announce message
void ptp_extract_announce_message(PtpAnnounceBody *pAnnounce, void *pPayload) {
    // cast header to byte accessible form
    uint8_t *p = (uint8_t *)pPayload + (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH);

    // copy header fields
    memcpy(&pAnnounce->currentUTCOffset, p + 0, 2);
    memcpy(&pAnnounce->priority1, p + 3, 1);
    memcpy(&pAnnounce->grandmasterClockClass, p + 4, 1);
    memcpy(&pAnnounce->grandmasterClockAccuracy, p + 5, 1);
    memcpy(&pAnnounce->grandmasterClockVariance, p + 6, 2);
    memcpy(&pAnnounce->priority2, p + 8, 1);
    memcpy(&pAnnounce->grandmasterClockIdentity, p + 9, 8);
    memcpy(&pAnnounce->localStepsRemoved, p + 17, 2);
    memcpy(&pAnnounce->timeSource, p + 19, 1);

    pAnnounce->currentUTCOffset = FLEXPTP_ntohs(pAnnounce->currentUTCOffset);
    pAnnounce->grandmasterClockVariance = FLEXPTP_ntohs(pAnnounce->grandmasterClockVariance);
    pAnnounce->grandmasterClockIdentity = FLEXPTP_ntohll(pAnnounce->grandmasterClockIdentity);
    pAnnounce->localStepsRemoved = FLEXPTP_ntohs(pAnnounce->localStepsRemoved);
}

void ptp_construct_binary_announce_message(void * pData, const PtpAnnounceBody * pAnnounce) {
    // cast header to byte accessible form
    uint8_t *p = (uint8_t *)pData + (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH);

    // convert to big endian
    uint16_t currentUTCOffset = FLEXPTP_ntohs(pAnnounce->currentUTCOffset);
    uint16_t grandmasterClockVariance = FLEXPTP_ntohs(pAnnounce->grandmasterClockVariance);
    uint64_t grandmasterClockIdentity = FLEXPTP_ntohll(pAnnounce->grandmasterClockIdentity);
    uint16_t localStepsRemoved = FLEXPTP_ntohs(pAnnounce->localStepsRemoved);

    // copy header fields
    memcpy(p + 0, &currentUTCOffset, 2);
    memcpy(p + 3, &pAnnounce->priority1, 1);
    memcpy(p + 4, &pAnnounce->grandmasterClockClass, 1);
    memcpy(p + 5, &pAnnounce->grandmasterClockAccuracy, 1);
    memcpy(p + 6, &grandmasterClockVariance, 2);
    memcpy(p + 8, &pAnnounce->priority2, 1);
    memcpy(p + 9, &grandmasterClockIdentity, 8);
    memcpy(p + 17, &localStepsRemoved, 2);
    memcpy(p + 19, &pAnnounce->timeSource, 1);
}

// write n timestamps following the header in to packet
void ptp_write_binary_timestamps(void *pPayload, const TimestampI *ts, uint8_t n) {
    uint8_t *p = ((uint8_t *)pPayload) + PTP_HEADER_LENGTH;

    // write n times
    uint8_t i;
    for (i = 0; i < n; i++) {
        // get timestamp data
        uint64_t sec = FLEXPTP_htonll(ts->sec << 16);
        uint64_t nanosec = FLEXPTP_htonl(ts->nanosec);

        // fill in time data
        memcpy(p, &sec, 6); // 48-bit
        p += 6;

        memcpy(p, &nanosec, 4);
        p += 4;

        // step onto next element
        ts++;
    }
}

// extract n timestamps from a message
void ptp_extract_timestamps(TimestampI *ts, void *pPayload, uint8_t n) {
    uint8_t *p = ((uint8_t *)pPayload) + PTP_HEADER_LENGTH; // pointer at the beginning of first timestamp

    // read n times
    uint8_t i;
    for (i = 0; i < n; i++) {
        // seconds
        ts->sec = 0;
        memcpy(&ts->sec, p, 6); // 48-bit
        p += 6;

        // nanoseconds
        memcpy(&ts->nanosec, p, 4);
        p += 4;

        // network->host
        ts->sec = FLEXPTP_ntohll(ts->sec << 16);
        ts->nanosec = FLEXPTP_ntohl(ts->nanosec);

        // step to next timestamp
        ts++;
    }
}

// extract Delay_Resp ID data
void ptp_read_delay_resp_id_data(PtpDelay_RespIdentification *pDRData, void *pPayload) {
    uint8_t *p = (uint8_t *)pPayload;
    memcpy(&pDRData->requestingSourceClockIdentity, p + 44, 8);
    memcpy(&pDRData->requestingSourcePortIdentity, p + 52, 2);

    // network->host
    pDRData->requestingSourcePortIdentity = FLEXPTP_ntohs(pDRData->requestingSourcePortIdentity);
}

// insert Delay_Resp ID data
void ptp_write_delay_resp_id_data(void *pPayload, const PtpDelay_RespIdentification *pDRData) {
    uint8_t *p = (uint8_t *)pPayload;
    uint16_t reqSrcPortId = FLEXPTP_htons(pDRData->requestingSourcePortIdentity); // host->network
    memcpy(p + 44, &pDRData->requestingSourceClockIdentity, 8);
    memcpy(p + 52, &reqSrcPortId, 2);
}

// clear flag structure
void ptp_clear_flags(PtpFlags *pFlags) {
    memset(pFlags, 0, sizeof(PtpFlags));
}
