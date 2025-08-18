#ifndef SERVO_DEBUG_SERVO
#define SERVO_DEBUG_SERVO

#include <stdint.h>

#include "../ptp_servo_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the debug servo.
 */
void debug_servo_init();

/**
 * Deinitialize the debug servo.
 */
void debug_servo_deinit();

/**
 * Reset the debug servo.
 */
void debug_servo_reset();

/**
 * Run the debug servo.
 * 
 * @param dt time error in nanoseconds
 * @param pAux auxiliary synchronization cycle context data 
 */
float debug_servo_run(int32_t dt, PtpServoAuxInput * pAux);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_DEBUG_SERVO */
