/**
  ******************************************************************************
  * @file    format_utils.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module defines format conversion functions between network and
  * host byte order and conversion functions between PTP logarithmic interval
  * designator and time duration value.
  ******************************************************************************
  */

#ifndef FLEXPTP_FORMAT_UTILS_H_
#define FLEXPTP_FORMAT_UTILS_H_

#include <stdint.h>

/**
 * Convert logarithmic interval designator code to a milliseconds value.
 * Input must be in the inclusive range of [-6, 3]. 
 *
 * The function emulates the execution of the following computation: 2^(n) * 1000 
 *
 * @param logi PTP-defined logarithmic interval code
 * @return closest defined period in milliseconds
 */
uint16_t ptp_logi2ms(int8_t logi);

/**
 * Convert millisecond period value to PTP-defined logarithmic interval designator code.
 * Input must be a value from the following set: (15, 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000).
 *
 * This function mimics the following computation: round(log2(T/1000))
 *
 * @param ms period in milliseconds. Values of above valid input set must be respected.
 * @return PTP-defined logarithmic interval code
*/
int8_t ptp_ms2logi(uint16_t ms);

/**
 * Function to swap byte order, convert BE representation to LE.
 * The naming complies with other flexPTP network->host conversion function namings.
 *
 * @param in value in BE representation
 * @return value in LE representation
 */
uint64_t FLEXPTP_ntohll(uint64_t in);

/**
 * Function to swap byte order, convert LE representation to BE.
 * The naming complies with other flexPTP host->network conversion function namings.
 *
 * @param in value in BE representation
 * @return value in LE representation
 */
uint64_t FLEXPTP_htonll(uint64_t in);

#endif /* FLEXPTP_FORMAT_UTILS_H_ */
