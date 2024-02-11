/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef PTP_PROFILE_PRESETS_H_
#define PTP_PROFILE_PRESETS_H_

#include <flexptp/ptp_types.h>

const PtpProfile *ptp_profile_preset_get(const char *pName);    // get profile by name
size_t ptp_profile_preset_cnt();        // get number of preset profiles
const char *ptp_profile_preset_get_name(size_t i);      // get profile name by index

#endif                          /* PTP_PROFILE_PRESETS_H_ */
