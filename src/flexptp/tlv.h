/**
  ******************************************************************************
  * @file    tlv.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   This module implements the TLV-related functionality.
  ******************************************************************************
  */

#ifndef FLEXPTP_TLV
#define FLEXPTP_TLV

#include "ptp_types.h"

/**
 * Unfold TLVs to memory area beginning with dst up to maxLen bytes.
 * The algorithm stops at the first TLV that cannot fit the remaining size. Only
 * TLVs matching mt will be extracted from the list.
 * 
 * @param dst pointer to destination area
 * @param pad pointer to a forward linked list of TLVs
 * @param mt corresponding message type
 * @param maxLen maximum length of extraction.
 * 
 * @return number of bytes extracted
 * 
 */
uint16_t ptp_tlv_insert(void * dst, const PtpProfileTlvElement * pad, PtpMessageType mt, uint16_t maxLen);

#endif /* FLEXPTP_TLV */
