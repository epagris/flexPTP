/**
  ******************************************************************************
  * @file    network_stack_driver.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   This file is a header for the employed Network Stack Driver (NSD).
  * A NSD must define ALL four functions listed below.
  ******************************************************************************
  */

#ifndef FLEXPTP_NETWORK_STACK_DRIVER
#define FLEXPTP_NETWORK_STACK_DRIVER

#include <stdbool.h>

#include "ptp_raw_msg_circbuf.h"

extern PtpCircBuf gRawRxMsgBuf; ///< Input circular buffer
extern PtpCircBuf gRawTxMsgBuf; ///< Output circular buffers

/**
 * Initialize or reinitialize the Network Stack Driver.
 * 
 * @param tp PTP transport type
 * @param dm PTP delay mechanism
 */
void ptp_nsd_init(PtpTransportType tp, PtpDelayMechanism dm);

/**
 * Fetch the Ethernet network interface hardware address.
 * 
 * @param hwa pointer to a 6-byte array where the hardware address is going to be stored to
 */
void ptp_nsd_get_interface_address(uint8_t * hwa);

/**
 * Transmit a PTP message.
 * 
 * @param pMsg pointer to a RawPtpMessage
 */
void ptp_nsd_transmit_msg(RawPtpMessage *pMsg);

/**
 * Join or leave IGMP groups associated with the current PTP profile.
 */
void ptp_nsd_igmp_join_leave(bool join);

#endif /* FLEXPTP_NETWORK_STACK_DRIVER */
