/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_FORMAT_UTILS_H_
#define FLEXPTP_FORMAT_UTILS_H_

#include <stdint.h>

uint16_t ptp_logi2ms(int8_t logi);      // log interval to milliseconds
int8_t ptp_ms2logi(uint16_t ms);        // milliseconds to log interval
uint64_t ntohll(uint64_t in);   // network->host byte order change
uint64_t htonll(uint64_t in);   // host->network byte order change

#endif                          /* FLEXPTP_FORMAT_UTILS_H_ */
