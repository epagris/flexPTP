/**
  ******************************************************************************
  * @file    ptp_servo_types.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module defines the basic servo types.
  ******************************************************************************
  */

#ifndef FLEXPTP_SIM_PTP_SERVO_TYPES_H_
#define FLEXPTP_SIM_PTP_SERVO_TYPES_H_

#include "ptp_sync_cycle_data.h"

/**
 * @brief Data to perform a full synchronization.
 */ 
typedef struct {
    PtpSyncCycleData scd; ///< Sync cycle data

    // information about sync interval
    int8_t logMsgPeriod; ///< Logarithmic message period
    double msgPeriodMs; ///< Message period in ms
    int64_t measSyncPeriodNs; ///< Measured synchronization period (t1->t1)
} PtpServoAuxInput;

#endif /* FLEXPTP_SIM_PTP_SERVO_TYPES_H_ */
