#include "ptp_port_simulation.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory>

#include "simsrc/PtpClock.h"

std::shared_ptr<PtpClock> pSlClock;

extern "C" {

void ptphw_init(uint32_t increment, uint32_t addend) {
    // create clock
    PtpClock::ClockProperties ckProps = {
            0xFFFFFFFF,
            0xFFFF5DCC,
            0.0
    };
    pSlClock = std::make_shared<PtpClock>(ckProps);

    // set default addend
    pSlClock->setAddend(addend);
}

void ptphw_gettime(TimestampU *pTime) {
    PtpClock::Timestamp ts = pSlClock->getTime();
    pTime->sec = ts.sec;
    pTime->nanosec = ts.ns;
}

void ptphw_set_addend(uint32_t addend) {
    pSlClock->setAddend(addend);
}

void ptphw_update_clock(uint32_t s, uint32_t ns, int dir) {
    pSlClock->updateTime(s, ns, dir);
}

}

