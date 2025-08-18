/**
  ******************************************************************************
  * @file    slave.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   This module implements the slave clock functionality.
  ******************************************************************************
  */

#ifndef FLEXPTP_SLAVE
#define FLEXPTP_SLAVE

#include "ptp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process PTP packet addressed to a slave clock.
 * @param pRawMsg pointer to the raw PTP message
 * @param pHeader pointer to the extracted PTP header
 */
void ptp_slave_process_message(RawPtpMessage *pRawMsg, PtpHeader *pHeader);

/**
 * Initialize the PTP slave.
 */
void ptp_slave_init();

/**
 * Reset the PTP slave.
 */
void ptp_slave_reset();

/**
 * Destroy the PTP slave.
 */
void ptp_slave_destroy();

/**
 * Start PTP slave (autonomous) operation.
 */
void ptp_slave_enable();

/**
 * Stop PTP slave (autonomous) operation.
 */
void ptp_slave_disable();

/**
 * Tick the slave module.
 */
void ptp_slave_tick();

#ifdef __cplusplus
}
#endif

#endif /* FLEXPTP_SLAVE */
