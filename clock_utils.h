/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_CLOCK_UTILS_H_
#define FLEXPTP_CLOCK_UTILS_H_

#include <stdint.h>

void ptp_print_clock_identity(uint64_t clockID);        // print clock identity
uint64_t hextoclkid(const char *str);   // convert hexadecimal string to 64-bit clock id

void ptp_create_clock_identity();       // create clock identity based on MAC address

void ptp_set_clock_offset(int32_t offset);      // set PPS offset
int32_t ptp_get_clock_offset(); // get PPS offset

#endif                          /* FLEXPTP_CLOCK_UTILS_H_ */
