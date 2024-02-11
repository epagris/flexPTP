/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_PTP_SYNC_CYCLE_DATA_H_
#define FLEXPTP_PTP_SYNC_CYCLE_DATA_H_

#include <flexptp/timeutils.h>

typedef struct {

    TimestampI t[6];
} PtpSyncCycleData;

#endif                          /* FLEXPTP_PTP_SYNC_CYCLE_DATA_H_ */
