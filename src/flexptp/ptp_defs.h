/**
 ******************************************************************************
 * @file    ptp_defs.h
 * @copyright András Wiesner, 2019-\showdate "%Y"
 * @brief   In here reside a multitude of fundamental PTP-related constants
 * and definitions.
 ******************************************************************************
 */

#ifndef FLEXPTP_PTP_DEFS_H_
#define FLEXPTP_PTP_DEFS_H_

#include <stdint.h>

#include <flexptp_options.h>

// IP address of PTP-IGMP groups
#define PTP_IGMP_DEFAULT_STR ("224.0.1.129")    ///< PTP default IGMP group
#define PTP_IGMP_PEER_DELAY_STR ("224.0.0.107") ///< PTP Peer-Delay IGMP group

#ifndef LWIP
typedef ip4_addr ip_addr_t;
typedef ip4_addr ip4_addr_t;
#endif

extern const ip_addr_t PTP_IGMP_PRIMARY;    ///< Primary IGMP address
extern const ip_addr_t PTP_IGMP_PEER_DELAY; ///< Peer_Delay IGMP address

#define PTP_ETHERTYPE (0x88F7)        ///< PTP EtherType
#define ETHERTYPE_PTP (PTP_ETHERTYPE) ///< (for lwIP conformity)

// Ethernet address of PTP messages
extern const uint8_t PTP_ETHERNET_PRIMARY[6];    ///< PTP's L2 Primary Ethernet address
extern const uint8_t PTP_ETHERNET_PEER_DELAY[6]; ///< PTP's L2 Peer_Delay Ethernet address

// PTP UDP ports
#define PTP_PORT_EVENT (319)   ///< PTP event message port
#define PTP_PORT_GENERAL (320) ///< PTP general message port

#define PTP_HEADER_LENGTH (34)        ///< Length of the PTP header
#define PTP_TIMESTAMP_LENGTH (10)     ///< Length of a single timestamp
#define PTP_PORT_ID_LENGTH (10)       ///< Length of the port identification field
#define PTP_ANNOUNCE_BODY_LENGTH (20) ///< Length of the Announce body

#define PTP_PCKT_SIZE_SYNC (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH)                                       ///< Size of a Sync message
#define PTP_PCKT_SIZE_FOLLOW_UP (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH)                                  ///< Size of a Follow_Up message
#define PTP_PCKT_SIZE_DELAY_REQ (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH)                                  ///< Size of a Delay_Req message
#define PTP_PCKT_SIZE_DELAY_RESP (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH + PTP_PORT_ID_LENGTH)            ///< Size of a Delay_Resp message
#define PTP_PCKT_PDELAY_RESERVED_END (10)                                                                   ///< Reserved area at the end of the message
#define PTP_PCKT_SIZE_PDELAY_REQ (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH + PTP_PCKT_PDELAY_RESERVED_END)  ///< Size of a PDelay_Req message
#define PTP_PCKT_SIZE_PDELAY_RESP (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH + PTP_PORT_ID_LENGTH)           ///< Size of a PDelay_Resp message
#define PTP_PCKT_SIZE_PDELAY_RESP_FOLLOW_UP (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH + PTP_PORT_ID_LENGTH) ///< Size of a PDelay_Resp_Follow_Up message
#define PTP_PCKT_SIZE_ANNOUNCE (PTP_HEADER_LENGTH + PTP_TIMESTAMP_LENGTH + PTP_ANNOUNCE_BODY_LENGTH)        ///< Size of an Announce message

// ---------------------------

#define PTP_MAX_PROFILE_NAME_LENGTH (7)                            ///< Maximum profile name length
#define PTP_MAX_TLV_PRESET_NAME_LENGTH PTP_MAX_PROFILE_NAME_LENGTH ///< Maximum TLV preset name length

// ---- CALCULATED VALUES ----

#define PTP_CLOCK_TICK_FREQ_HZ (1000000000 / PTP_INCREMENT_NSEC)                                                                   ///< Rated clock tick frequency
#define PTP_ADDEND_INIT (((double)(0x100000000)) / (((double)(PTP_MAIN_OSCILLATOR_FREQ_HZ)) / ((double)(PTP_CLOCK_TICK_FREQ_HZ)))) ///< Initial addend value
#define PTP_ADDEND_CORR_PER_PPB_F (((double)0x100000000) / ((double)PTP_INCREMENT_NSEC * PTP_MAIN_OSCILLATOR_FREQ_HZ))             ///< Addend/ppb ratio

// ---- AUTODEFINES ----------

#ifndef PTP_HEARTBEAT_TICKRATE_MS
#define PTP_HEARTBEAT_TICKRATE_MS (125) ///< Heartbeat ticking period
#endif

#ifndef PTP_PORT_ID
#define PTP_PORT_ID (1) ///< PTP port ID on the device
#endif

#ifndef PTP_ACCURACY_LIMIT_NS
#define PTP_ACCURACY_LIMIT_NS (100) ///< Limit of the LOCKED state
#endif

#ifndef PTP_DEFAULT_SERVO_OFFSET
#define PTP_DEFAULT_SERVO_OFFSET (0) ///< Default servo offset in nanoseconds
#endif

#ifndef PTP_DEFAULT_COARSE_TRIGGER_NS
#define PTP_DEFAULT_COARSE_TRIGGER_NS (20000000) ///< Coarse correction kick-in threshold
#endif

#ifndef PTP_BMCA_LISTENING_TIMEOUT_MS
#define PTP_BMCA_LISTENING_TIMEOUT_MS (3000) ///< BMCA LISTENING state timeout
#endif

#ifndef PTP_MASTER_QUALIFICATION_TIMEOUT
#define PTP_MASTER_QUALIFICATION_TIMEOUT (4) ///< Number of Announce intervals
#endif

#ifndef PTP_ANNOUNCE_RECEIPT_TIMEOUT
#define PTP_ANNOUNCE_RECEIPT_TIMEOUT (3) ///< Number of tolerated consecutive lost Announce messages
#endif

#ifndef PTP_ENABLE_MASTER_OPERATION
#define PTP_ENABLE_MASTER_OPERATION (0) ///< By default, disable Master operation mode
#endif

#ifndef PTP_CLOCK_PRIORITY1
#define PTP_CLOCK_PRIORITY1 (128) ///< Clock priority1
#endif

#ifndef PTP_CLOCK_PRIORITY2
#define PTP_CLOCK_PRIORITY2 (128) ///< Clock priority2
#endif

// ---- CAPABILITIES AND ANNOUNCE DATASET -------

#ifndef PTP_BEST_CLOCK_CLASS
#define PTP_BEST_CLOCK_CLASS (PTP_CC_DEFAULT) ///< Best clock class of this device
#endif

#ifndef PTP_WORST_ACCURACY
#define PTP_WORST_ACCURACY (PTP_CA_UNKNOWN) ///< Worst accuracy of this device
#endif

#ifndef PTP_TIME_SOURCE
#define PTP_TIME_SOURCE (PTP_TSRC_INTERNAL_OSCILLATOR) ///< Time source of this device
#endif

#ifndef PTP_FALLBACK_UTC_OFFSET
#define PTP_FALLBACK_UTC_OFFSET (37) ///< UTC offset caused by the accumulated leap seconds
#endif

// ---- MASTER P2P SLAVE HANDLING -----

#ifndef PTP_PDELAY_SLAVE_QUALIFICATION
#define PTP_PDELAY_SLAVE_QUALIFICATION (3) ///< Number of consecutive PDelReq-PDelResp iterations after the SLAVE is considered stable
#endif

#ifndef PTP_PDELAY_DROPOUT
#define PTP_PDELAY_DROPOUT PTP_PDELAY_SLAVE_QUALIFICATION ///< Maximum number of failed PDelReq-PDelResp cycles before the MASTER drops the SLAVE
#endif

// ---- TERMINAL COLORS -----

// clang-format off

#ifndef PTP_CUSTOM_COLORS ///< Define this to override PTP_COLOR_* macros
#define PTP_COLOR_RED      "\x1b[31m" ///< Red
#define PTP_COLOR_BRED     "\x1b[91m" ///< Bright red
#define PTP_COLOR_GREEN    "\x1b[32m" ///< Green
#define PTP_COLOR_BGREEN   "\x1b[92m" ///< Bright green
#define PTP_COLOR_YELLOW   "\x1b[33m" ///< Yellow
#define PTP_COLOR_BYELLOW  "\x1b[93m" ///< Bright yellow
#define PTP_COLOR_BLUE     "\x1b[34m" ///< Blue
#define PTP_COLOR_MAGENTA  "\x1b[35m" ///< Magenta
#define PTP_COLOR_BMAGENTA "\x1b[95m" ///< Bright magenta
#define PTP_COLOR_CYAN     "\x1b[36m" ///< Cyan
#define PTP_COLOR_RESET    "\x1b[0m"  ///< Reset colors
#endif

// clang-format on

// ---- NETWORK-HOST CONVERSIONS ----
// -- These are STATICALLY COMPUTABLE macros! ---

#define FLEXPTP_htonl(a)                    \
        ((((a) >> 24) & 0x000000ff) |   \
         (((a) >>  8) & 0x0000ff00) |   \
         (((a) <<  8) & 0x00ff0000) |   \
         (((a) << 24) & 0xff000000))

#define FLEXPTP_ntohl(a)    FLEXPTP_htonl((a))

#define FLEXPTP_htons(a)                \
        ((((a) >> 8) & 0x00ff) |    \
         (((a) << 8) & 0xff00))

#define FLEXPTP_ntohs(a)    FLEXPTP_htons((a))

#endif /* FLEXPTP_PTP_DEFS_H_ */
