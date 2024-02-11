/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <stdint.h>
#include <stdbool.h>

// timestamp (unsigned)
typedef struct {
    uint64_t sec;
    uint32_t nanosec;
} TimestampU;

// timestamp (signed)
typedef struct {
    int64_t sec;
    int32_t nanosec;
} TimestampI;

#define NANO_PREFIX (1000000000)
#define NANO_PREFIX_F (1000000000.0f)

#define T_SEC_PER_MINUTE (60)
#define T_SEC_PER_HOUR (60 * T_SEC_PER_MINUTE)
#define T_SEC_PER_DAY (24 * T_SEC_PER_HOUR)
#define T_SEC_PER_YEAR (365 * T_SEC_PER_DAY)
#define T_SEC_PER_LEAPYEAR (366 * T_SEC_PER_DAY)
#define T_SEC_PER_FOURYEAR (3 * T_SEC_PER_YEAR + T_SEC_PER_LEAPYEAR)

// TIME OPERATIONS
TimestampI *tsUToI(TimestampI * ti, const TimestampU * tu);     // convert unsigned timestamp to signed
TimestampI *addTime(TimestampI * r, const TimestampI * a, const TimestampI * b);        // sum timestamps (r = a + b)
TimestampI *subTime(TimestampI * r, const TimestampI * a, const TimestampI * b);        // substract timestamps (r = a - b)
TimestampI *divTime(TimestampI * r, const TimestampI * a, int divisor); // divide time value by an integer
uint64_t nsU(const TimestampU * t);     // convert unsigned time into nanoseconds
int64_t nsI(const TimestampI * t);      // convert signed time into nanoseconds
void normTime(TimestampI * t);  // normalize time
int64_t tsToTick(const TimestampI * ts, uint32_t tps);  // convert time to hardware ticks, tps: ticks per second
TimestampI *nsToTsI(TimestampI * r, int64_t ns);        // convert nanoseconds to time
bool nonZeroI(const TimestampI * a);    // does the timestamp differ from zero?

void tsPrint(char *str, const TimestampI * ts); // print datetime to string, string must be 20-long at least!

#endif                          /* TIMEUTILS_H */
