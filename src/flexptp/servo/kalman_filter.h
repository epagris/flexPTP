#ifndef SERVO_KALMAN_FILTER
#define SERVO_KALMAN_FILTER

#include <stdint.h>

#include "../ptp_servo_types.h"

/** A Kalman-filter based servo. This module implements the Kalman-filter introduced in the paper: 
 * 'Performance Analysis of Kalman-Filter-Based Clock Synchronization in IEEE 1588 Networks' by 
 * Giada Gorgi and Claudio Narduzzi (https://ieeexplore.ieee.org/document/5934411)
 */

/**
 * Initialize the Kalman-filter.
 */
void kalman_filter_init();

/**
 * Deinitialize the Kalman-filter.
 */
void kalman_filter_deinit();

/**
 * Reset the Kalman-filter.
 */
void kalman_filter_reset();

/**
 * Run the Kalman-filter.
 * 
 * @param dt time error in nanoseconds
 * @param pAux auxiliary synchronization cycle context data 
 */
float kalman_filter_run(int32_t dt, PtpServoAuxInput * pAux);


#endif /* SERVO_KALMAN_FILTER */
