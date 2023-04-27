#ifndef FLEXPTP_LOGGING_H_
#define FLEXPTP_LOGGING_H_

#include <flexptp/ptp_core.h>

enum {
    PTP_LOG_DEF,
    PTP_LOG_CORR_FIELD,
    PTP_LOG_TIMESTAMPS,
    PTP_LOG_INFO,
    PTP_LOG_LOCKED_STATE,
    PTP_LOG_N
};

void ptp_enable_logging(int logId, bool en);
void ptp_disable_all_logging();

#endif                          /* FLEXPTP_LOGGING_H_ */
