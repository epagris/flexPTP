//
// Created by epagris on 2022.10.22..
//

#include "PtpMasterClock.h"

#include <memory>

void PtpMasterClock::setup(const PtpMasterClock::PtpMasterClockSettings &settings) {
    mSettings = settings;
}

void PtpMasterClock::start() {
    mBeaconThread = std::make_shared<std::thread>(&PtpMasterClock::mBeaconThread, this);
}

void PtpMasterClock::mBeaconThread_CB() {
    while (mRunning) {

    }
}
