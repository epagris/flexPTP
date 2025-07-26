/* (C) András Wiesner, 2025 */

#ifndef SERVO_SPID_CONTROLLER
#define SERVO_SPID_CONTROLLER

#include <stdint.h>

#include "../ptp_servo_types.h"

#define SPID_FILTER_LEN (128)
#define SPID_MINIMUM_CORRECTION_LOWER_LIMIT_PPB (20)
#define SPID_LIMIT_RANGE_CONSTANT (1.5)
#define SPID_SMOOTHENED_CTRL_LIMIT_NS (1000)
#define SPID_COEFFICIENT_CORRECTION_LIMIT_NS (200)

/**
 * Initialize PID controller.
 */
void spid_ctrl_init();

/**
 * Deinitialize PID controller.
 */
void spid_ctrl_deinit();

/**
 * Reset PID controller.
 */
void spid_ctrl_reset();

/**
 * Run the PID controller.
 * 
 * @param dt time error in nanoseconds
 * @param pAux auxiliary synchronization cycle context data 
 */
float spid_ctrl_run(int32_t dt, PtpServoAuxInput * pAux);

#endif /* SERVO_SPID_CONTROLLER */
