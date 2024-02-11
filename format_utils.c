/* The flexPTP project, (C) András Wiesner, 2024 */

#include <flexptp/format_utils.h>
#include <stdint.h>

// lookup table for log msg. period to ms conversion (ms values are floor-rounded)
#define LOG_INTERVAL_LOOKUP_OFFSET (6)
static uint16_t sLogIntervalMs[] = { 15, 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 0 };    // terminating zero as last element

// log interval to milliseconds
uint16_t ptp_logi2ms(int8_t logi)
{
    return sLogIntervalMs[logi + LOG_INTERVAL_LOOKUP_OFFSET];
}

// milliseconds to log interval
int8_t ptp_ms2logi(uint16_t ms)
{
    uint16_t *pIter = sLogIntervalMs;
    while (*pIter != 0) {
        if (*pIter == ms) {
            break;
        }
    }
    return (int8_t) (pIter - sLogIntervalMs) + LOG_INTERVAL_LOOKUP_OFFSET;
}

// Network->Host byte order conversion for 64-bit values
uint64_t ntohll(uint64_t in)
{
    unsigned char out[8] = { in >> 56, in >> 48, in >> 40, in >> 32, in >> 24, in >> 16, in >> 8, in };
    return *(uint64_t *) out;
}

uint64_t htonll(uint64_t in)
{
    return ntohll(in);
}
