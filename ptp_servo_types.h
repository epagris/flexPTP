#ifndef FLEXPTP_SIM_PTP_SERVO_TYPES_H
#define FLEXPTP_SIM_PTP_SERVO_TYPES_H

#include <flexptp/ptp_sync_cycle_data.h>

// Data to perform a full synchronization
typedef struct {
    PtpSyncCycleData scd;       // sync cycle data

    // information about sync interval
    int8_t logMsgPeriod;        // ...
    double msgPeriodMs;         // message period in ms
    double measSyncPeriod;      // measured synchronization period (t1->t1) TODO rename! (suffix: _ns)
} PtpServoAuxInput;

#endif                          //FLEXPTP_SIM_PTP_SERVO_TYPES_H
