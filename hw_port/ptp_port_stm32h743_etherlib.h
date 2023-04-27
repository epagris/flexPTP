/*
 * ptp_port_tiva_tm4c1294.c
 *
 *  Created on: 2021. szept. 17.
 *      Author: epagris
 */

#ifndef HW_PORT_PTP_PORT_STM32H743_ETHERLIB_C_
#define HW_PORT_PTP_PORT_STM32H743_ETHERLIB_C_

#include <flexptp/timeutils.h>

void ptphw_init(uint32_t increment, uint32_t addend);   // initialize PTP hardware
void ptphw_gettime(TimestampU * pTime); // get time

#endif                          /* HW_PORT_PTP_PORT_STM32H743_ETHERLIB_C_ */
