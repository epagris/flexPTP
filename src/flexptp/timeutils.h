/**
 ******************************************************************************
 * @file    timeutils.h
 * @copyright András Wiesner, 2019-\showdate "%Y"
 * @brief   This module defines storage classes for timestamps and operations
 *  on time values.
 ******************************************************************************
 */

#ifndef TIMEUTILS_H_
#define TIMEUTILS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Timestamp (unsigned)
 */
typedef struct
{
    uint64_t sec;     ///< seconds
    uint32_t nanosec; ///< nanoseconds
} TimestampU;

/**
 * @brief Timestamp (signed)
 */
typedef struct
{
    int64_t sec;     ///< seconds
    int32_t nanosec; ///< nanoseconds
} TimestampI;

#define NANO_PREFIX (1000000000)      ///< Integer nano prefix
#define NANO_PREFIX_F (1000000000.0f) ///< Float nanno prefix

#define T_SEC_PER_MINUTE (60)                                        ///< seconds per minute
#define T_SEC_PER_HOUR (60 * T_SEC_PER_MINUTE)                       ///< seconds per hour
#define T_SEC_PER_DAY (24 * T_SEC_PER_HOUR)                          ///< seconds per day
#define T_SEC_PER_YEAR (365 * T_SEC_PER_DAY)                         ///< seconds per year
#define T_SEC_PER_LEAPYEAR (366 * T_SEC_PER_DAY)                     ///< seconds per leapyear
#define T_SEC_PER_FOURYEAR (3 * T_SEC_PER_YEAR + T_SEC_PER_LEAPYEAR) ///< seconds per four years

// TIME OPERATIONS
/**
 * Convert unsigned timestamp to signed.
 *
 * @param ti destination (signed) timestamp
 * @param tu source (unsigned) timestamp
 */
TimestampI *tsUToI(TimestampI *ti, const TimestampU *tu);

/**
 * Convert signed timestamp to unsigned.
 *
 * @param tu destination (signed) timestamp
 * @param ti source (unsigned) timestamp
 */
TimestampU *tsIToU(TimestampU *tu, const TimestampI *ti);

/**
 * Add up timestamps.
 * r = a + b;
 *
 * @param r result
 * @param a first operand
 * @param b second operand
 *
 * @return r
 */
TimestampI *addTime(TimestampI *r, const TimestampI *a, const TimestampI *b);

/**
 * Substract timestamps.
 * r = a - b;
 *
 * @param r result
 * @param a first operand
 * @param b second operand
 *
 * @return r
 */
TimestampI *subTime(TimestampI *r, const TimestampI *a, const TimestampI *b);

/**
 * Divide (inversely scale) a time value by an integer.
 *
 * @param r pointer to the result field
 * @param a dividend
 * @param divisor divisor
 *
 * @return r
 */
TimestampI *divTime(TimestampI *r, const TimestampI *a, int divisor);

/**
 * Convert unsigned time into nanoseconds.
 *
 * @param t time
 *
 * @result time in nanoseconds
 */
uint64_t nsU(const TimestampU *t); //

/**
 * Convert signed time into nanoseconds.
 *
 * @param t time
 *
 * @result time in nanoseconds
 */
int64_t nsI(const TimestampI *t);

/**
 * Normalize time. Move whole seconds from the nanoseconds field to the seconds.
 *
 * @param t Time. It will be overwritten.
 */
void normTime(TimestampI *t); // normalize time

/**
 * Convert duration (~time value) to hardware ticks.
 *
 * @param ts time
 * @param tps ticks per second
 *
 * @return duration in ticks (rounded to floor during division)
 */
int64_t tsToTick(const TimestampI *ts, uint32_t tps);

/**
 * Convert nanoseconds to time.
 *
 * @param r results
 * @param ns time in nanoseconds
 *
 * @return r
 */
TimestampI *nsToTsI(TimestampI *r, int64_t ns);

/**
 * Does the timestamp differ from zero?
 *
 * @param a timestamp
 *
 * @return a != 0
 */
bool nonZeroI(const TimestampI *a);

/**
 * Print datetime to string.
 *
 * @param str destination area, must provide at least 20 consecutive bytes!
 * @param ts pointer to timestamp object holding the time value
 */
void tsPrint(char *str, const TimestampI *ts);

#endif /* TIMEUTILS_H_ */
