#ifndef HW_PORT_PTP_PORT_STM32F407_ETHERLIB_C_
#define HW_PORT_PTP_PORT_STM32F407_ETHERLIB_C_

#include "../../timeutils.h"

/**
 * Initialize PTP hardware.
 * 
 * @param increment PTP hardware clock tick increment in nanoseconds
 * @param addend initialize frequency tuning code word
 */
void ptphw_init(uint32_t increment, uint32_t addend);

/**
 * Get time right from the hardware clock.
 * 
 * @param pTime pointer to a TimestampU object.
 */
void ptphw_gettime(TimestampU * pTime);

#endif /* HW_PORT_PTP_PORT_STM32F407_ETHERLIB_C_ */