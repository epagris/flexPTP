/* (C) András Wiesner, 2021 */

#ifndef SERVO_PD_CONTROLLER_H_
#define SERVO_PD_CONTROLLER_H_

#include <stdint.h>

void pd_ctrl_init(); // initialize PD controller
void pd_ctrl_reset(); // reset controller
float pd_ctrl_run(int32_t dt); // run the controller (input: time error in nanosec)

#endif /* SERVO_PD_CONTROLLER_H_ */
