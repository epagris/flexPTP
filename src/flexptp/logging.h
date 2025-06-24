/**
  ******************************************************************************
  * @file    logging.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module handles various logging capabilities.
  ******************************************************************************
  */

#ifndef FLEXPTP_LOGGING_H_
#define FLEXPTP_LOGGING_H_

#include <stdbool.h>

/**
 * @brief Collection type of possible predefined PTP log types.
 */
enum {
	PTP_LOG_DEF, ///< Default PTP log, prints sync-cycle related data (e.g. time error, tuning, code word etc.).
	PTP_LOG_CORR_FIELD, ///< The PTP engine will print the correction fields of particular PTP messages.
	PTP_LOG_TIMESTAMPS, ///< The PTP engine will print the T1-T4/T6 timestamps (in E2E/P2P modes).
	PTP_LOG_INFO, ///< If enabled, the user will be notified of unexpected events occurred and exceptions.
	PTP_LOG_LOCKED_STATE, ///< Signals the user if the PTP engine consideres the clock have gotten locked.
  PTP_LOG_BMCA, ///< Notifies the user about BMCA state changes
	PTP_LOG_N
};

/**
 * Enable or disable a specific kind of logging.
 *
 * @param logId log ID, one of the PTP_LOG_* values
 * @param en enabled/disabled logId kind of logging feature
 */
void ptp_log_enable(int logId, bool en);

/**
 * Disable all logging features.
 */
void ptp_log_disable_all();

#endif /* FLEXPTP_LOGGING_H_ */
