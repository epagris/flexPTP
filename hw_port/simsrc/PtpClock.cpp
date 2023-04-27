#include "PtpClock.h"

PtpClock::PtpClock(const PtpClock::ClockProperties &ckProps) : mCkProps(ckProps) {}

void PtpClock::setTime(uint32_t s, uint32_t ns) {
    mTime = s * C_NANO_PREFIX + ns * C_NANO_PREFIX;
}

void PtpClock::updateTime(uint32_t s, uint32_t ns, bool dir) {
    int64_t diff = s * C_NANO_PREFIX + ns;
    diff *= dir ? +1 : -1;
    mTime += diff;
}

PtpClock::Timestamp PtpClock::getTime() const {
    return { mTime / C_NANO_PREFIX, (uint32_t)(mTime % C_NANO_PREFIX) };
}

void PtpClock::setAddend(uint64_t addend) {
    mAddend = addend;
}

uint64_t PtpClock::getAddend() const {
    return mAddend;
}

void PtpClock::advanceClock(uint32_t s, uint32_t ns) {
    mTime += (s * C_NANO_PREFIX + ns) * (mAddend / mCkProps.ckNomAddend);
}
