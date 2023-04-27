#ifndef FLEXPTP_SIM_PTPMASTERCLOCK_H
#define FLEXPTP_SIM_PTPMASTERCLOCK_H

#include <memory>
#include <list>
#include <thread>
#include "PtpSlaveClock.h"

class PtpMasterClock {
 public:
    using PtpMasterClockSettings = struct {
        uint16_t originCurrentUTCOffset;
        uint8_t priority1;
        uint8_t grandmasterClockClass;
        uint8_t grandmasterClockAccuracy;
        uint16_t grandmasterClockVariance;
        uint8_t priority2;
        uint64_t grandmasterClockIdentity;
        uint16_t localStepsRemoved;
        uint8_t timeSource;
    };
 private:
     std::list < std::shared_ptr < PtpSlaveClock >> mSlvs;      // slave clocks connected to this master clock
    PtpMasterClockSettings mSettings;   // master clock settings
     std::shared_ptr < std::thread > mBeaconThread;     // thread handling multicasted message transmission
    bool mRunning {
    };
    void mBeaconThread_CB();    // function running in the thread
 public:
     PtpMasterClock() = default;
    void setup(const PtpMasterClockSettings & settings);        // setup master clock
    void start();               // start master clock
};

#endif                          //FLEXPTP_SIM_PTPMASTERCLOCK_H
