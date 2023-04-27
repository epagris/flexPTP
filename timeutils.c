/* (C) András Wiesner, 2020 */

#include <flexptp/timeutils.h>
#include <flexptp_options.h>

#include <stdio.h>

// TimestampU -> TimestampI
TimestampI *tsUToI(TimestampI * ti, const TimestampU * tu)
{
    ti->sec = (int64_t) tu->sec;
    ti->nanosec = (int32_t) tu->nanosec;
    return ti;
}

// r = a + b;
TimestampI *addTime(TimestampI * r, const TimestampI * a, const TimestampI * b)
{
    int64_t ns = nsI(a) + nsI(b);
    nsToTsI(r, ns);
    return r;
}

// r = a - b;
TimestampI *subTime(TimestampI * r, const TimestampI * a, const TimestampI * b)
{
    int64_t ns = nsI(a) - nsI(b);
    nsToTsI(r, ns);
    return r;
}

TimestampI *divTime(TimestampI * r, const TimestampI * a, int divisor)
{
    int64_t ns = a->sec * NANO_PREFIX + a->nanosec;
    ns = ns / divisor;          // így a pontosság +-0.5ns
    r->sec = ns / NANO_PREFIX;
    r->nanosec = ns - r->sec * NANO_PREFIX;
    return r;
}

uint64_t nsU(const TimestampU * t)
{
    return t->sec * NANO_PREFIX + t->nanosec;
}

int64_t nsI(const TimestampI * t)
{
    return t->sec * NANO_PREFIX + t->nanosec;
}

void normTime(TimestampI * t)
{
    uint32_t s = t->nanosec / NANO_PREFIX;
    t->sec += s;
    t->nanosec -= s * NANO_PREFIX;
}

int64_t tsToTick(const TimestampI * ts, uint32_t tps)
{
    int64_t ns = ts->sec * NANO_PREFIX + ts->nanosec;
    int64_t ticks = (ns * tps) / NANO_PREFIX;
    return ticks;
}

TimestampI *nsToTsI(TimestampI * r, int64_t ns)
{
    r->sec = ns / NANO_PREFIX;
    r->nanosec = ns % NANO_PREFIX;
    return r;
}

bool nonZeroI(const TimestampI * a)
{
    return a->sec != 0 || a->nanosec != 0;
}

static unsigned FIRST_DAY_OF_MONTH[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

void tsPrint(char *str, const TimestampI * ts)
{
    unsigned fouryear, year, month, day, hour, minute, sec, rem;

    fouryear = ts->sec / T_SEC_PER_FOURYEAR;    // determine elapsed four year blocks from 1970 (YYLY)
    rem = ts->sec - fouryear * T_SEC_PER_FOURYEAR;      // compute remaining seconds
    year = fouryear * 4;        // calculate years fouryear-block part

    // split up four-year blocks into distinct years
    if (rem > T_SEC_PER_YEAR) {
        year++;
        rem -= T_SEC_PER_YEAR;
    }

    if (rem > T_SEC_PER_YEAR) {
        year++;
        rem -= T_SEC_PER_YEAR;
    }

    if (rem > T_SEC_PER_LEAPYEAR) {
        year++;
        rem -= T_SEC_PER_LEAPYEAR;
    }
    // convert remaining seconds to days
    day = rem / T_SEC_PER_DAY;
    rem -= day * T_SEC_PER_DAY;
    day++;

    year += 1970;
    bool leapyear = year % 4 == 0;

    // get month from days
    unsigned i = 0;
    for (i = 0; i < 12; i++) {
        unsigned first1 = FIRST_DAY_OF_MONTH[i] + (((i >= 2) && leapyear) ? 1 : 0);
        unsigned first2 = FIRST_DAY_OF_MONTH[i + 1] + (((i + 1 >= 2) && leapyear) ? 1 : 0);

        if (day > first1 && day <= first2) {
            month = i + 1;
            day = day - first1;
            break;
        }
    }

    // get time
    hour = rem / T_SEC_PER_HOUR;
    rem -= hour * T_SEC_PER_HOUR;
    minute = rem / T_SEC_PER_MINUTE;
    rem -= minute * T_SEC_PER_MINUTE;
    sec = rem;

    // -------------------------------

    // print datetime
    SNPRINTF(str, 20, "%02d-%02d-%04d %02d:%02d:%02d", day, month, year, hour, minute, sec);
}
