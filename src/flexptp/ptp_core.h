/**
  ******************************************************************************
  * @file    ptp_core.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   Core of the PTP implementation. Defines functions for message
  * processing, clock tuning, storing and loading configurations and managing
  * event callbacks.
  ******************************************************************************
  */

#ifndef PTP_CORE_H_
#define PTP_CORE_H_

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "event.h"
#include "task.h"
#include "timers.h"

#include "ptp_types.h"

#include <flexptp_options.h>

/**
 * Initialize flexPTP module.
 * 
 * @param hwa network interface hardware address over which the flexPTP operates
 */
void ptp_init(const uint8_t *hwa);

/**
 * Deinitialize flexPTP module.
 */
void ptp_deinit();

/**
 * Reset PTP subsystem.
 */
void ptp_reset();

/**
 * Process a PTP packet.
 * 
 * @param pRawMsg pointer to raw PTP message
 */
void ptp_process_packet(RawPtpMessage *pRawMsg);

/**
 * Process a core event.
 * 
 * @param event pointer to an event object
 */
void ptp_process_event(const PtpCoreEvent * event);

/**
 * Set callback invoked each synchronization cycle.
 * @param syncCB callback function pointer
 */
void ptp_set_sync_callback(PtpSyncCallback syncCb);

/**
 * Set user event callback.
 * @param userEventCb callback function pointer
 */
void ptp_set_user_event_callback(PtpUserEventCallback userEventCb);

///\cond 0
extern PtpCoreState gPtpCoreState;
extern const TimestampI zeroTs; // a zero timestamp
///\endcond

#endif /* PTP_CORE_H_ */
