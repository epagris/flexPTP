/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_SBMC_H_
#define FLEXPTP_SBMC_H_

#include <flexptp/ptp_types.h>

// select better master
// return 0: if pMP1 is better than pMP2, 1: if reversed
int ptp_select_better_master(PtpMasterProperties * pMP1, PtpMasterProperties * pMP2);

#endif                          /* FLEXPTP_SBMC_H_ */
