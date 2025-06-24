/**
  ******************************************************************************
  * @file    bmca.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   This module implements the Best Master Clock Algorithm.
  ******************************************************************************
  */

#ifndef FLEXPTP_SBMC_H_
#define FLEXPTP_SBMC_H_

#include "ptp_types.h"

/**
 * Select better master. (Not "best", with intent!)
 * 
 * @param pMP1: pointer to the first master's PtpMasterProperites object
 * @param pMP2: pointer to the second master's PtpMasterProperites object
 * 
 * @return 0: if pMP1 is better than pMP2, 1: if reversed
 */
int ptp_select_better_master(PtpMasterProperties * pMP1, PtpMasterProperties * pMP2);

/**
 * Handle announce messages.
 *
 * @param pAnn pointer to PtpAnnounceBody object
 * @param pHeader pointer to Announce message header
 */
void ptp_handle_announce_msg(PtpAnnounceBody *pAnn, PtpHeader *pHeader);

/**
 * Initialize SBMC module.
 */
void ptp_bmca_init();

/**
 * Destroy SBMC module.
 */
void ptp_bmca_destroy();

/**
 * Reset the SBMC module.
 */
void ptp_bmca_reset();

/**
 * Call this to notify SBMC about the time passing.
 */
void ptp_bmca_tick();

#endif /* FLEXPTP_SBMC_H_ */
