/* (C) András Wiesner, 2021 */

#ifndef SERVO_PD_CONTROLLER_H_
#define SERVO_PD_CONTROLLER_H_

#include <stdint.h>

#include "../ptp_servo_types.h"

/**
 * Initalize PD controller.
 */
void pid_ctrl_init();

/**
 * Deinitialize PD controller.
 */
void pid_ctrl_deinit();

/**
 * Reset PD controller.
 */
void pid_ctrl_reset();

/**
 * Run the PD controller.
 * 
 * @param dt time error in nanoseconds
 * @param pAux auxiliary synchronization cycle context data 
 */
float pid_ctrl_run(int32_t dt, PtpServoAuxInput * pAux);

#endif /* SERVO_PD_CONTROLLER_H_ */
