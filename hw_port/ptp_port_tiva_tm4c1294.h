/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef HW_PORT_PTP_PORT_TIVA_TM4C1294_C_
#define HW_PORT_PTP_PORT_TIVA_TM4C1294_C_

#include <flexptp/timeutils.h>

void ptphw_init(uint32_t increment, uint32_t addend);   // initialize PTP hardware
void ptphw_gettime(TimestampU * pTime); // get time

#endif                          /* HW_PORT_PTP_PORT_TIVA_TM4C1294_C_ */
