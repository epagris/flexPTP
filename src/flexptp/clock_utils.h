/**
  ******************************************************************************
  * @file    clock_utils.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module defines clock identity related operations.
  ******************************************************************************
  */

#ifndef FLEXPTP_CLOCK_UTILS_H_
#define FLEXPTP_CLOCK_UTILS_H_

#include <stdint.h>

/**
 * Print PTP hardware clock identity.
 * @param clockID clock ID in 64-bit LE representation
 */
void ptp_print_clock_identity(uint64_t clockID);

/**
 * Convert a string containing only hexadecimal digits to a 64-bit LE clock ID representation.
 * @param str string repesentation of the clock ID
 */
uint64_t hextoclkid(const char *str);

/** 
 * Generate clock ID based on the MAC address.
 * It works the following way: the first 3 bytes come from the first three bytes of the MAC address, then a string of 0xFF-0xFE is appended, 
 * finally the second three bytes of the MAC-address is inserted.
 * 
 * @param hwa hardware address of the network interface
 */
void ptp_create_clock_identity(const uint8_t * hwa);

#endif /* FLEXPTP_CLOCK_UTILS_H_ */
