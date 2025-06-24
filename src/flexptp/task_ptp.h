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
 * Register PTP task.
 */
void reg_task_ptp();

/**
 * Unreg PTP task.
 */
void unreg_task_ptp();

/**
 * Is the PTP task operating?
 */
bool task_ptp_is_operating();

/**
 * Enqueue PTP message.
 * 
 * @param pPayload message payload
 * @param len message length
 * @param ts_sec reception timestamp seconds part
 * @param ts_ns receptiopn timestamp nanoseconds part
 * @param tp transport portocol (L2/L4)
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
 * Post an event.
 * 
 * @param event pointer to a filled event object
 * @return sucessful posting
 */
bool ptp_event_enqueue(const PtpCoreEvent * event);

#endif // TASK_PTP_H_
