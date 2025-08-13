/**
  ******************************************************************************
  * @file    task_ptp.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   The entry point of the whole PTP-implementation. Calling
  * reg_task_ptp() initializes the PTP-engine, invoking unreg_task_ptp()
  * shuts it down.
  ******************************************************************************
  */

#ifndef TASK_PTP_H_
#define TASK_PTP_H_

#include <stdint.h>
#include <stdbool.h>

#include "event.h"
#include "ptp_types.h"

/**
 * Start the heartbeat timer.
 */
void ptp_start_heartbeat_tmr();

/**
 * Stop the heartbeat timer.
 */
void ptp_stop_heartbeat_tmr();

/**
 * Register PTP task.
 * 
 * @return initialization successful
 */
bool reg_task_ptp();

/**
 * Unreg PTP task.
 */
void unreg_task_ptp();

/**
 * Is the PTP task operating?
 */
bool is_flexPTP_operating();

/**
 * Enqueue PTP message.
 * 
 * @param pPayload message payload
 * @param len message length
 * @param ts_sec reception timestamp seconds part
 * @param ts_ns reception timestamp nanoseconds part
 * @param tp transport protocol (L2/L4)
 */
void ptp_receive_enqueue(const void *pPayload, uint32_t len, uint32_t ts_sec, uint32_t ts_ns, int tp);

/**
 * Put a PTP message into the transmit queue.
 * 
 * @param pMsg pointer to raw PTP message. Can be discarded after the function has returned.
 * @return successful insertion
 */
bool ptp_transmit_enqueue(const RawPtpMessage * pMsg);

/**
 * Transmit timestamp callback handler.
 * 
 * @param tag unique message tag
 * @param seconds transmit timestamp seconds part
 * @param nanoseconds transmit timestamp nanoseconds part
 */
void ptp_transmit_timestamp_cb(uint32_t uid, uint32_t seconds, uint32_t nanoseconds);

/**
 * Read transmit timestamp and release message buffer area.
 * 
 * @param tag unique message tag
 * @param pTs pointer to a TimestampI object
 * 
 * @return successful timestamp fetch
 */
bool ptp_read_and_clear_transmit_timestamp(uint32_t tag, TimestampI * pTs);

/**
 * Post an event.
 * 
 * @param event pointer to a filled event object
 * @return successful posting
 */
bool ptp_event_enqueue(const PtpCoreEvent * event);

#ifdef FLEXPTP_OSLESS
/**
 * Provide the flexPTP with periodic ticks of PTP_HEARTBEAT_TICKRATE_MS intervals.
 * This function is only exposed if operating in FLEXPTP_OSLESS mode!
 * 
 * Call this function at PTP_HEARTBEAT_TICKRATE_MS intervals if operating in FLEXPTP_OSLESS
 * mode, otherwise flexPTP manages it.
 */
void ptp_heartbeat_tmr_cb();

/**
 * flexPTP's main loop.
 * This function is only exposed if operating in FLEXPTP_OSLESS mode!
 * 
 * Call this function periodically to advance internal processing if operating in
 * FLEXPTP_OSLESS mode, otherwise the library internally manages it.
 */
void task_ptp();
#endif

#endif // TASK_PTP_H_
