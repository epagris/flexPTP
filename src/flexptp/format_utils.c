/* (C) András Wiesner, 2020-2022 */

#include "format_utils.h"
#include <stdint.h>

#define LOG_INTERVAL_LOOKUP_OFFSET (6) ///< Index associated with the 1s logarithmic interval.

/**
 * Lookup table for log msg. period to ms conversion (ms values are floor-rounded).
 */ 
static uint16_t sLogIntervalMs[] = { 15, 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 0 }; // terminating zero as last element

// log interval to milliseconds
uint16_t ptp_logi2ms(int8_t logi) {
	return sLogIntervalMs[logi + LOG_INTERVAL_LOOKUP_OFFSET];
}

// milliseconds to log interval
int8_t ptp_ms2logi(uint16_t ms) {
	uint16_t * pIter = sLogIntervalMs;
	while (*pIter != 0) {
		if (*pIter == ms) {
			break;
		}
	}
	return (int8_t)(pIter - sLogIntervalMs) + LOG_INTERVAL_LOOKUP_OFFSET;
}

// Network->Host byte order conversion for 64-bit values
uint64_t FLEXPTP_ntohll(uint64_t in) {
	unsigned char out[8] = { in >> 56, in >> 48, in >> 40, in >> 32, in >> 24, in >> 16, in >> 8, in };
	return *(uint64_t*) out;
}

uint64_t FLEXPTP_htonll(uint64_t in) {
	return FLEXPTP_ntohll(in);
}
