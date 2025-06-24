/**
  ******************************************************************************
  * @file    common.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   This module defines messaging functions for both the slave and
  * master modules.
  ******************************************************************************
  */

#ifndef FLEXPTP_COMMON
#define FLEXPTP_COMMON

#include "ptp_types.h"

/**
 * Initialize Delay_Request header.
 */
void ptp_init_delay_req_header();

/**
 * Construct and send Delay_Req message.
 */
void ptp_send_delay_req_message();

/**
 * Send PDelay_Resp_Follow_Up based on already transmitted PDelay_Resp message.
 *
 * @param pMsg pointer to the PDelay_Resp message that forms the base of the Pdelay_Resp_Follow_Up message
 */
void ptp_send_pdelay_resp_follow_up(const RawPtpMessage *pMsg);

/**
 * Send PDelay_Resp based on PDelay_Req.
 *
 * @param pMsg pointer to PDelay_Req message object
 */
void ptp_send_pdelay_resp(const RawPtpMessage *pMsg);

/**
 * Compute Mean Path Delay when operating in E2E mode.
 *
 * @param pTs pointer to array holding T1-T4 timestamps
 * @param pCf pointer to array holding correction fields associated with T1-T4 timestamps
 * @param pMPD pointer to area where the MPD gets stored
 */
void ptp_compute_mean_path_delay_e2e(const TimestampI *pTs, const uint64_t *pCf, TimestampI *pMPD);

/**
 * Compute Mean Path Delay when operating in P2P mode.
 *
 * @param pTs pointer to array holding T1-T4 timestamps
 * @param pCf pointer to array holding correction fields associated with T1-T4 timestamps
 * @param pMPD pointer to area where the MPD gets stored
 */
void ptp_compute_mean_path_delay_p2p(const TimestampI *pTs, const uint64_t *pCf, TimestampI *pMPD);

/**
 * Reset functionality shared with both slave and master.
 */
void ptp_common_reset();

#endif /* FLEXPTP_COMMON */
