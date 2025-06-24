/**
  ******************************************************************************
  * @file    msg_utils.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module defines functions that deal with actual PTP messages;
  * they can extract or insert headers, timestamps or any data of other kind.
  ******************************************************************************
  */

#ifndef FLEXPTP_MSG_UTILS_H_
#define FLEXPTP_MSG_UTILS_H_

#include <stdint.h>

#include "ptp_types.h"
#include "timeutils.h"

/**
 * Extract PTP flags from the rendered bitfield found in particular PTP messages.
 *
 * @param pFlags Pointer to an existing PTPFlags object. It is the destination where the separated bits are getting stored to.
 * @param bitfield The direct copy of the PTP flags field from the PTP message of interest.
 */
void ptp_load_flags(PtpFlags *pFlags, uint16_t bitfield);

/**
 * Render PTPFlags into binary form.
 * 
 * @param pFlags pointer to a filled-out PTPFlags object
 * @return binary, bitfield form of the PTPFlags
 */
uint16_t ptp_write_flags(const PtpFlags *pFlags);

/**
 * Extract the PTP message header from the whole messages payload.
 *
 * @param pHeader Pointer to an existing PtpHeader object, where the fetched header will get stored to.
 * @param pPayload Pointer to the beginning of the whole PTP message's payload.
 *
*/
void ptp_extract_header(PtpHeader *pHeader, const void *pPayload);

/**
 * Render PtpHeader object to binary (raw) form.
 * 
 * @param pData pointer to the beginning of the binary destination
 * @param pHeader pointer to a filled-out PtpHeader object
 */
void ptp_construct_binary_header(void *pData, const PtpHeader *pHeader);

/**
 * Extract PTP Announce message body from the rendered payload.
 *
 * @param pAnnounce Pointer to an existing PtpAnnounceBody object, where the contents of the Announce message of interest will be copied to.
 * @param pPayload Pointer to the beginning of the whole PTP message's payload.
*/
void ptp_extract_announce_message(PtpAnnounceBody * pAnnounce, void *pPayload);

/**
 * Construct PTP Announce message body from its contents.
 * 
 * @param pAnnounce Pointer to an existing, filled PtpAnnounceBody object.
 * @param pData Pointer to the binary storage area.
 */
void ptp_construct_binary_announce_message(void * pData, const PtpAnnounceBody * pAnnounce);

/**
 * Place n pieces of timestamps following the PTP header in the binary packet.
 * 
 * @param pPayload pointer to the BEGINNING of the binary PTP packet
 * @param ts pointer to an array of timestamps
 * @param n length of the array
 */
void ptp_write_binary_timestamps(void *pPayload, const TimestampI *ts, uint8_t n);

/**
 * Fetch n pieces of timestamps from a PTP message.
 * 
 * @param ts Pointer to an array of timestamps that can support at least n elements.
 * @param pPayload pointer to the BEGINNING of the binary PTP packet
 * @param n number of timestamps to fetch.
 */
void ptp_extract_timestamps(TimestampI *ts, void *pPayload, uint8_t n);

/**
 * Read (P)Delay_Response identification data from a raw PTP message.
 * 
 * @param pDRData pointer to an existing Delay_RespIdentification object
 * @param pPayload pointer to the BEGINNING of a (P)Delay_Response PTP message
 */
void ptp_read_delay_resp_id_data(PtpDelay_RespIdentification *pDRData, void *pPayload); // extract Delay_Resp ID data

/**
 * Write (P)Delay_Response identification data from a raw PTP message.
 * 
 * @param pPayload pointer to the BEGINNING of a (P)Delay_Response PTP message
 * @param pDRData pointer to an existing, filled-out Delay_RespIdentification object
 */
void ptp_write_delay_resp_id_data(void * pPayload , const PtpDelay_RespIdentification *pDRData); // insert Delay_Resp ID data

/**
 * Clear PTPFlags object.
 * 
 * @param pFlags pointer to an existing PTPFlags object
 */
void ptp_clear_flags(PtpFlags *pFlags);

#endif /* FLEXPTP_MSG_UTILS_H_ */
