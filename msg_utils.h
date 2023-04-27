/* (C) András Wiesner, 2020-2022 */

#ifndef FLEXPTP_MSG_UTILS_H_
#define FLEXPTP_MSG_UTILS_H_

#include <flexptp/ptp_types.h>
#include <flexptp/timeutils.h>
#include <stdint.h>

void ptp_load_flags(PTPFlags * pFlags, uint16_t bitfield);      // load ptp flags from bitfield
void ptp_extract_header(PtpHeader * pHeader, const void *pPayload);     // extract fields from a PTP header
void ptp_extract_announce_message(PtpAnnounceBody * pAnnounce, void *pPayload); // extract announce message
void ptp_construct_binary_header(void *pData, const PtpHeader * pHeader);       // construct binary header from header structure
void ptp_write_binary_timestamps(void *pPayload, TimestampI * ts, uint8_t n);   // write n timestamps following the header in to packet
void ptp_extract_timestamps(TimestampI * ts, void *pPayload, uint8_t n);        // extract n timestamps from a message
void ptp_read_delay_resp_id_data(Delay_RespIdentification * pDRData, void *pPayload);   // extract Delay_Resp ID data
void ptp_write_delay_resp_id_data(void *pPayload, const Delay_RespIdentification * pDRData);    // insert Delay_Resp ID data
void ptp_clear_flags(PTPFlags * pFlags);        // clear flag structure
void ptp_construct_binary_sync(void *pData, const PtpHeader * pHeader); // create Sync message

#endif                          /* FLEXPTP_MSG_UTILS_H_ */
