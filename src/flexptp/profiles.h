/**
  ******************************************************************************
  * @file    profiles.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module implements defines a single method that prints a 
  * verbose summary of the operating PTP profile.
  ******************************************************************************
  */

#ifndef FLEXPTP_PROFILES_H_
#define FLEXPTP_PROFILES_H_

#ifdef __cplusplus
extern "C" {
#endif

extern char *PTP_TRANSPEC_HINT[];
extern char *PTP_TRANSPORT_TYPE_HINT[];
extern char *PTP_DELMECH_HINT[];

/**
 * Print current PTP profile settings summary.
 */
void ptp_print_profile();

#ifdef __cplusplus
}
#endif

#endif /* FLEXPTP_PROFILES_H_ */
