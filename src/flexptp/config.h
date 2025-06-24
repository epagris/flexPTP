/**
  ******************************************************************************
  * @file    config.h
  * @copyright András Wiesner, 2024-\showdate "%Y"
  * @brief   This module defines functions for storing and loading flexPTP
  * configurations.
  ******************************************************************************
  */

#ifndef FLEXPTP_CONFIG
#define FLEXPTP_CONFIG

#include "ptp_types.h"

/**
 * @brief Global storable-loadable configuration.
 */
typedef struct {
    PtpProfile profile;           ///< PTP-profile
    TimestampI offset;            ///< PPS signal offset
    uint32_t logging;             ///< logging compressed into a single bitfield
    uint8_t priority1, priority2; ///< Clock priority fields
} PtpConfig;

/**
 * Store flexPTP configuration to an existing location.
 * @param pConfig pointer to an existing PtpConfig object to where the configuration will be saved
 */
void ptp_store_config(PtpConfig *pConfig); // store PTP-engine configuration (param: output)

/**
 * Load flexPTP configuration from a PTP config object.
 * @param pConfig pointer to an existing PtpConfig object, providing the retained configuration
 */
void ptp_load_config(const PtpConfig *pConfig); // load PTP-engine configuration

/**
 * Load PTP-engine configuration from binary dump (i.e. from unaligned address)
 * @param pDump pointer to config dump
 */
void ptp_load_config_from_dump(const void *pDump);

#endif /* FLEXPTP_CONFIG */
