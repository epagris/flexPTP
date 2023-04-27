#ifndef FLEXPTP_SIM_PTPCLOCK_H
#define FLEXPTP_SIM_PTPCLOCK_H

#include <chrono>

class PtpClock {
 public:
    using ClockProperties = struct {
        uint64_t ckDivOF;       // clock division overflow
        uint64_t ckNomAddend;   // nominal addend value
        double ckVariance_nsns; // clock variance in ns square
    };
    using Timestamp = struct {
        uint64_t sec;
        uint32_t ns;
    };
 public:
    static constexpr uint32_t C_NANO_PREFIX = 1000000000;
 private:
     uint64_t mTime {
    };                          // slave clock time
    uint64_t mAddend {
    };                          // addend value

    const ClockProperties mCkProps;     // slave clock overflow
 public:
    explicit PtpClock(const ClockProperties & ckProps); // slave clock overflow
    void setTime(uint32_t s, uint32_t ns);      // set time
    void updateTime(uint32_t s, uint32_t ns, bool dir); // update time by value
    Timestamp getTime() const;  // get time
    void setAddend(uint64_t addend);    // set addend
    uint64_t getAddend() const; // get addend
    void advanceClock(uint32_t s, uint32_t ns); // advance clock with given time difference
};

#endif                          //FLEXPTP_SIM_PTPCLOCK_H
