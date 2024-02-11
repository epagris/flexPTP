/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_PTP_DEFS_H_
#define FLEXPTP_PTP_DEFS_H_

#include "flexptp_options.h"

#include <stdint.h>

// IP address of PTP-IGMP groups
#define PTP_IGMP_DEFAULT_STR ("224.0.1.129")

#ifndef LWIP
typedef ip4_addr ip_addr_t;
typedef ip4_addr ip4_addr_t;
#endif

extern const ip_addr_t PTP_IGMP_PRIMARY;

// Ethernet address of PTP messages
extern const uint8_t PTP_ETHERNET_PRIMARY[6];

// PTP UDP ports
#define PTP_PORT_EVENT (319)
#define PTP_PORT_GENERAL (320)

#define PTP_HEADER_LENGTH (34)
#define PTP_TIMESTAMP_LENGTH (10)
#define PTP_PORT_ID_LENGTH (10)

#define PTP_PCKT_SIZE_SYNC (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH)
#define PTP_PCKT_SIZE_FOLLOW_UP (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH)
#define PTP_PCKT_SIZE_DELAY_REQ (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH)
#define PTP_PCKT_SIZE_DELAY_RESP (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH + PTP_PORT_ID_LENGTH)

// ---- AUTODEFINES ----------

#ifndef PTP_ACCURACY_LIMIT_NS
#define PTP_ACCURACY_LIMIT_NS (100)
#endif

// ---- CALCULATED VALUES ----

#define PTP_CLOCK_TICK_FREQ_HZ (1000000000 / PTP_INCREMENT_NSEC)        // clock tick frequency
#define PTP_ADDEND_INIT ((uint32_t)(0x100000000 / (PTP_MAIN_OSCILLATOR_FREQ_HZ / (float)PTP_CLOCK_TICK_FREQ_HZ)))       // addend value
#define PTP_ADDEND_CORR_PER_PPB_F ((float)0x100000000 / ((float)PTP_INCREMENT_NSEC * PTP_MAIN_OSCILLATOR_FREQ_HZ))      // addend/ppb

#endif                          /* FLEXPTP_PTP_DEFS_H_ */
