/* The flexPTP project, (C) András Wiesner, 2024 */

#include <flexptp/format_utils.h>
#include <flexptp/msg_utils.h>
#include <flexptp/ptp_defs.h>
#include <string.h>

// load ptp flags from bitfield
void ptp_load_flags(PTPFlags * pFlags, uint16_t bitfield)
{
#define GET_FLAG_FROM_BITFIELD(flag,n) (pFlags->flag) = (bitfield >> (n)) & 1

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

/*	pFlags->PTP_SECURITY = (bitfield >> 15) & 1;
	pFlags->PTP_ProfileSpecific_2 = (bitfield >> 14) & 1;
	pFlags->PTP_ProfileSpecific_1 = (bitfield >> 13) & 1;

	pFlags->PTP_UNICAST = (bitfield >> 10) & 1;
	pFlags->PTP_TWO_STEP = (bitfield >> 9) & 1;
	pFlags->PTP_ALTERNATE_MASTER = (bitfield >> 8) & 1;

	pFlags->FREQUENCY_TRACEABLE = (bitfield >> 5) & 1;
	pFlags->TIME_TRACEABLE = (bitfield >> 4) & 1;

	pFlags->PTP_TIMESCALE = (bitfield >> 3) & 1;
	pFlags->PTP_UTC_REASONABLE = (bitfield >> 2) & 1;
	pFlags->PTP_LI_59 = (bitfield >> 1) & 1;
	pFlags->PTP_LI_61 = (bitfield >> 0) & 1;*/
}

// write flags to bitfield
uint16_t ptp_write_flags(PTPFlags * pFlags)
{
#define SET_BIT_IN_FLAG_BITFIELD(flag,n) bitfield |= (pFlags->flag) ? (1 << (n)) : 0

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
void ptp_extract_header(PtpHeader * pHeader, const void *pPayload)
{
    // cast header to byte accessible form
    uint8_t *p = (uint8_t *) pPayload;

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
    ptp_load_flags(&pHeader->flags, ntohs(flags));

    // read correction field
    pHeader->correction_subns = ntohll(pHeader->correction_ns) & 0xffff;
    pHeader->correction_ns = ntohll(pHeader->correction_ns) >> 16;

    pHeader->messageLength = ntohs(pHeader->messageLength);
    pHeader->sourcePortID = ntohs(pHeader->sourcePortID);
    pHeader->sequenceID = ntohs(pHeader->sequenceID);
}

// extract announce message
void ptp_extract_announce_message(PtpAnnounceBody * pAnnounce, void *pPayload)
{
    // cast header to byte accessible form
    uint8_t *p = (uint8_t *) pPayload + (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH);

    // copy header fields
    memcpy(&pAnnounce->originCurrentUTCOffset, p + 0, 2);
    memcpy(&pAnnounce->priority1, p + 3, 1);
    memcpy(&pAnnounce->grandmasterClockClass, p + 4, 1);
    memcpy(&pAnnounce->grandmasterClockAccuracy, p + 5, 1);
    memcpy(&pAnnounce->grandmasterClockVariance, p + 6, 2);
    memcpy(&pAnnounce->priority2, p + 8, 1);
    memcpy(&pAnnounce->grandmasterClockIdentity, p + 9, 8);
    memcpy(&pAnnounce->localStepsRemoved, p + 17, 2);
    memcpy(&pAnnounce->timeSource, p + 19, 1);

    pAnnounce->originCurrentUTCOffset = ntohs(pAnnounce->originCurrentUTCOffset);
    pAnnounce->grandmasterClockVariance = ntohs(pAnnounce->grandmasterClockVariance);
    pAnnounce->grandmasterClockIdentity = ntohll(pAnnounce->grandmasterClockIdentity);
    pAnnounce->localStepsRemoved = ntohs(pAnnounce->localStepsRemoved);
}

// construct binary header from header structure
void ptp_construct_binary_header(void *pData, const PtpHeader * pHeader)
{
    uint8_t *p = (uint8_t *) pData;
    uint8_t firstByte;

    // host->network
    uint16_t messageLength = htons(pHeader->messageLength);
    uint16_t sourcePortID = htons(pHeader->sourcePortID);
    uint16_t sequenceID = htons(pHeader->sequenceID);

    // fill in flags FIXME
    uint16_t flags = htons(ptp_write_flags(&pHeader->flags));   // convert from header fields

    // fill in correction value
    uint64_t correction = htonll((pHeader->correction_ns << 16) | (pHeader->correction_subns)); // TODO: ...

    // copy fields
    firstByte = (pHeader->transportSpecific << 4) | (pHeader->messageType & 0x0f);
    memcpy(p, &firstByte, 1);
    memcpy(p + 1, &pHeader->versionPTP, 1);
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

// write n timestamps following the header in to packet
void ptp_write_binary_timestamps(void *pPayload, TimestampI * ts, uint8_t n)
{
    uint8_t *p = ((uint8_t *) pPayload) + PTP_HEADER_LENGTH;

    // write n times
    uint8_t i;
    for (i = 0; i < n; i++) {
        // get timestamp data
        uint64_t sec = htonll(ts->sec << 16);
        uint64_t nanosec = htonl(ts->nanosec);

        // fill in time data
        memcpy(p, &sec, 6);     // 48-bit
        p += 6;

        memcpy(p, &nanosec, 4);
        p += 4;

        // step onto next element
        ts++;
    }
}

// extract n timestamps from a message
void ptp_extract_timestamps(TimestampI * ts, void *pPayload, uint8_t n)
{
    uint8_t *p = ((uint8_t *) pPayload) + PTP_HEADER_LENGTH;    // pointer at the beginning of first timestamp

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
        ts->sec = ntohll(ts->sec << 16);
        ts->nanosec = ntohl(ts->nanosec);

        // step to next timestamp
        ts++;
    }
}

// extract Delay_Resp ID data
void ptp_read_delay_resp_id_data(Delay_RespIdentification * pDRData, void *pPayload)
{
    uint8_t *p = (uint8_t *) pPayload;
    memcpy(&pDRData->requestingSourceClockIdentity, p + 44, 8);
    memcpy(&pDRData->requestingSourcePortIdentity, p + 52, 2);

    // network->host
    pDRData->requestingSourcePortIdentity = ntohs(pDRData->requestingSourcePortIdentity);
}

// insert Delay_Resp ID data
void ptp_write_delay_resp_id_data(void *pPayload, const Delay_RespIdentification * pDRData)
{
    uint8_t *p = (uint8_t *) pPayload;
    uint16_t reqSrcPortId = htons(pDRData->requestingSourcePortIdentity);       // host->network
    memcpy(p + 44, &pDRData->requestingSourceClockIdentity, 8);
    memcpy(p + 52, &reqSrcPortId, 2);
}

// clear flag structure
void ptp_clear_flags(PTPFlags * pFlags)
{
    memset(pFlags, 0, sizeof(PTPFlags));
}

// construct Sync message (TWO_STEP-mode only!)
void ptp_construct_binary_sync(void *pData, const PtpHeader * pHeader)
{
    // insert header
    ptp_construct_binary_header(pData, pHeader);

    // insert empty timestamps
    TimestampI zeroTs = { 0, 0 };
    ptp_write_binary_timestamps(pData, &zeroTs, 1);
}
