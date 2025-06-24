/**
  ******************************************************************************
  * @file    stats.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This is the statistics module that gathers data of the operating
  * PTP-engine.
  ******************************************************************************
  */

#ifndef FLEXPTP_STATS_H_
#define FLEXPTP_STATS_H_

#include <stdint.h>
#include <stdbool.h>

#include "ptp_types.h"

/**
 * Clear statitics.
 */
void ptp_clear_stats();

/**
 * Get statistics.
 * 
 * @return const pointer to the filled PTP stats object
 */
const PtpStats* ptp_get_stats(); 

/**
 * Collect statistics.
 * 
 * @param d time error in nanoseconds.
 */
void ptp_collect_stats(int64_t d);

/**
 * Expression to test if the clock could be considered locked measuring agains a specific filtered PTP time error
 * @param th threshold in nanoseconds
 */
#define PTP_IS_LOCKED(th) ((ptp_get_stats()->filtTimeErr < (th)) && (ptp_get_current_master_clock_identity() != 0)) ///< Is the PTP clock seemingly locked considering threshold passed?

#endif /* FLEXPTP_STATS_H_ */
