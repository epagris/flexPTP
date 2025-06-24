/**
  ******************************************************************************
  * @file    ptp_profile_presets.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module manages predefined profile presets. It allows their
  * fetching by name or ID.
  ******************************************************************************
  */

#ifndef PTP_PROFILE_PRESETS_H_
#define PTP_PROFILE_PRESETS_H_

#include "ptp_types.h"

/**
 * Get profile by name.
 * 
 * @param pName name of the desired profile
 * @return const pointer to the profile or NULL if not found
 */
const PtpProfile * ptp_profile_preset_get(const char * pName);

/**
 * Get TLV chain by name.
 * 
 * @param pName name of the desired TLV chain
 * @return const pointer to the chain or NULL if not found
 */
const PtpProfileTlvElement * ptp_tlv_chain_preset_get(const char * pName);

/**
 * Get number of predefined profiles.
 * 
 * @return number of profiles
 */
size_t ptp_profile_preset_cnt();

/**
 * Get number of predefined TLV chains.
 * 
 * @return number of TLV chains
 */
size_t ptp_tlv_chain_preset_cnt();

/**
 * Get profile name by index.
 * 
 * @param i profile index
 * @return const pointer to the profile that's associated the ith index or NULL
 */
const char * ptp_profile_preset_get_name(size_t i);

/**
 * Get TLV chain by index.
 * 
 * @param i TLV chain index
 * @return const pointer to the TLV chain that's associated the ith index or NULL
 */
const char * ptp_tlv_chain_preset_get_name(size_t i);

#endif /* PTP_PROFILE_PRESETS_H_ */
