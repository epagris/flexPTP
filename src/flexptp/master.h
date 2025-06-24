/**
  ******************************************************************************
  * @file    master.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   This module implements the master clock functionality.
  ******************************************************************************
  */

#ifndef FLEXPTP_MASTER
#define FLEXPTP_MASTER

#include "ptp_types.h"

/**
 * Initialize the Master module.
 */
void ptp_master_init();

/**
 * Destroy th eMaster module.
 */
void ptp_master_destroy();

/**
 * Reset the Master module.
 */
void ptp_master_reset();

/**
 * Pass ticks to the Master module.
 */
void ptp_master_tick();

/**
 * Enable the master module.
 */
void ptp_master_enable();

/**
 * Disable the master module.
 */
void ptp_master_disable();


// -------------

/**
 * Process a message by the master module.
 * 
 * @param pRawMsg Pointer to the raw PTP message.
 * @param pHeader Pointer to the extracted PTP message header.
 */
void ptp_master_process_message(RawPtpMessage *pRawMsg, PtpHeader *pHeader);

/**
 * Render a PTP Sync message based on header data.
 * @param pData pointer to target the PTP message
 * @param pHeader pointer to a filled-out PtpHeader object
 */
void ptp_construct_binary_sync(void * pData, const PtpHeader * pHeader); // create Sync message

#endif /* FLEXPTP_MASTER */
