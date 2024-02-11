/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_STATS_H_
#define FLEXPTP_STATS_H_

#include <flexptp/ptp_types.h>
#include <stdint.h>
#include <stdbool.h>

void ptp_clear_stats();         // clear statistics
const PtpStats *ptp_get_stats();        // get statistics
void ptp_collect_stats(int64_t d);      // collect statistics

#endif                          /* FLEXPTP_STATS_H_ */
