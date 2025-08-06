/**
 ******************************************************************************
 * @file    ptp_types.h
 * @copyright András Wiesner, 2019-\showdate "%Y"
 * @brief   This module defines the fundamental PTP message and state machine
 * type, flags, bitfields and the PTP engine's internal state types.
 ******************************************************************************
 */

#ifndef FLEXPTP_PTP_TYPES_H_
#define FLEXPTP_PTP_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#include "event.h"
#include "ptp_defs.h"
#include "timeutils.h"

#include "ptp_sync_cycle_data.h"

#include <flexptp_options.h>

#if !defined(FLEXPTP_FREERTOS) && !defined(FLEXPTP_CMSIS_OS2)
#define FLEXPTP_FREERTOS (1)
#endif

#ifdef FLEXPTP_FREERTOS
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
#elif defined(FLEXPTP_CMSIS_OS2)
#include <cmsis_compiler.h>
#include <cmsis_os2.h>
#endif

#ifdef FLEXPTP_FREERTOS
typedef TimerHandle_t TimerType;
#elif defined(FLEXPTP_CMSIS_OS2)
typedef osTimerId_t TimerType;
#endif

/**
 * @brief PTP packet type enumeration.
 */
typedef enum PTPMessageType {
    PTP_MT_Sync = 0,                   ///< Sync
    PTP_MT_Delay_Req = 1,              ///< Delay Request
    PTP_MT_PDelay_Req = 2,             ///< Peer Delay Request
    PTP_MT_PDelay_Resp = 3,            ///< Peer Delay Response
    PTP_MT_Follow_Up = 8,              ///< Follow Up
    PTP_MT_Delay_Resp = 9,             ///< Delay Response
    PTP_MT_PDelay_Resp_Follow_Up = 10, ///< Peer Delay Response Follow Up
    PTP_MT_Announce = 11               ///< Announce
} PtpMessageType;

/**
 * @brief PTP header control field values.
 */
typedef enum PTPControl {
    PTP_CON_Sync = 0,       ///< Sync
    PTP_CON_Delay_Req = 1,  ///< Delay Request
    PTP_CON_Follow_Up = 2,  ///< Follow Up
    PTP_CON_Delay_Resp = 3, ///< Delay Response
    PTP_CON_Other = 5       ///< Other
} PtpControl;

/**
 *  @brief PTP flags structure
 */
typedef struct {
    bool PTP_SECURITY;          ///< Security
    bool PTP_ProfileSpecific_2; ///< Profile Specific 2
    bool PTP_ProfileSpecific_1; ///< Profile Specific 1
    bool PTP_UNICAST;           ///< Unicast
    bool PTP_TWO_STEP;          ///< Two Step
    bool PTP_ALTERNATE_MASTER;  ///< Alternate Master
    bool FREQUENCY_TRACEABLE;   ///< Frequency Traceable
    bool TIME_TRACEABLE;        ///< Time Traceable
    bool PTP_TIMESCALE;         ///< Timescale
    bool PTP_UTC_REASONABLE;    ///< UTC Reasonable
    bool PTP_LI_59;             ///< Leap Second (59)
    bool PTP_LI_61;             ///< Leap Second (61)
} PtpFlags;

/**
 * @brief PTP message header structure.
 */
typedef struct
{
    // 0.
    uint8_t messageType;       ///< ID
    uint8_t transportSpecific; ///< Transport Specific

    // 1.
    uint8_t versionPTP; ///< PTP version
    uint8_t minorVersionPTP;

    // 2-3.
    uint16_t messageLength; ///< Length

    // 4.
    uint8_t domainNumber; ///< Domain

    // 5.
    uint8_t _r2;

    // 6-7.
    PtpFlags flags; ///< Flags

    // 8-15.
    uint64_t correction_ns;    ///< Correction nanoseconds
    uint32_t correction_subns; ///< Correction subnanoseconds

    // 16-19.
    uint32_t _r3;

    // 20-27.
    uint64_t clockIdentity; ///< Clock Identity

    // 28-29
    uint16_t sourcePortID; ///< Source Port ID

    // 30-31.
    uint16_t sequenceID; ///< Sequence ID

    // 32.
    uint8_t control; ///< Control

    // 33.
    int8_t logMessagePeriod; // Logarithmic Message Period
} PtpHeader;

/**
 * @brief Identification carrying Delay_Resp message.
 */
typedef struct {
    uint64_t requestingSourceClockIdentity; ///< Requesting Source Clock Identity
    uint16_t requestingSourcePortIdentity;  ///< Requesting Source Port Identity
} PtpDelay_RespIdentification;

/**
 * @brief PTP transport type enumeration.
 */
typedef enum {
    PTP_TP_IPv4 = 0, ///< IPv4 Transport Type
    PTP_TP_802_3 = 1 ///< Ethernet Transport Type
} PtpTransportType;

/**
 * @brief PTP Delay mechanism enumeration.
 */
typedef enum {
    PTP_DM_E2E = 0, ///< End-to-End Delay Mechanism
    PTP_DM_P2P = 1  ///< Peer-to-Peer Delay Mechanism
} PtpDelayMechanism;

/**
 * @brief PTP transport specific enumeration.
 */
typedef enum {
    PTP_TSPEC_UNKNOWN_DEF = 0, ///< Unkown Transport Specific Flag (default)
    PTP_TSPEC_GPTP_8021AS = 1, ///< 802.1AS Transport Specific Flag
} PtpTransportSpecific;

/**
 * @brief Enumeration for different PTP message classes.
 */
typedef enum {
    PTP_MC_EVENT = 0,  ///< Event Message Class
    PTP_MC_GENERAL = 1 ///< General Message Class
} PtpMessageClass;

#define MAX_PTP_MSG_SIZE (128) ///< Maximum PTP message size

/**
 * @brief Raw PTP message structure.
 */
struct RawPtpMessage_;

/**
 * @brief * Transmit callback function prototype.
 */
typedef void(TxCb)(const struct RawPtpMessage_ *pMsg);

/**
 * Tagging of transmitted PTP messages.
 */
typedef enum {
    RPMT_RANDOM = 0,            ///< Create a random, unique tag
    RPMT_SYNC,                  ///< Sync tag
    RPMT_DELAY_REQ,             ///< Delay_Req tag
} RawPtpMsgTag;

typedef struct RawPtpMessage_ {
    TimestampI ts; ///< Timestamp
    uint32_t size; ///< Packet size

    // --- transmit related ---
    uint32_t tag;            ///< unique transmit tag
    TxCb *pTxCb;             ///< transmit callback function
    PtpDelayMechanism tx_dm; ///< transmit transport type
    PtpMessageClass tx_mc;   ///< transmit message class

    // --- data ---
    uint8_t data[MAX_PTP_MSG_SIZE]; ///< raw packet data
} RawPtpMessage;

/**
 * @brief Standard PTP clock classes.
 */
typedef enum {
    PTP_CC_PRIMARY_REFERENCE = 6,               ///< Primary reference clock, cannot be a slave
    PTP_CC_PRIMARY_REFERENCE_HOLDOVER = 7,      ///< Normally a Primary reference clock, but now working in holdover mode
    PTP_CC_APPLICATION_SPECIFIC = 13,           ///< Application specific reference clock, ARB timescale, cannot be a slave
    PTP_CC_APPLICAION_SPECIFIC_HOLDOVER = 14,   ///< An Application specific class clock that operates in holdover
    PTP_CC_PRIMARY_REFERENCE_DEGRAD_A = 52,     ///< A Primary reference operating off the holdover specification
    PTP_CC_APPLICATION_SPECIFIC_DEGRAD_A = 58,  ///< An Application specific class clock operating off the holdover specification
    PTP_CC_PRIMARY_REFERENCE_DEGRAD_B = 187,    ///< A Primary reference operating off the holdover specification, can be a slave
    PTP_CC_APPLICATION_SPECIFIC_DEGRAD_B = 193, ///< An Application specific class clock operating off the holdover specification, can be a slave
    PTP_CC_DEFAULT = 248,                       ///< Default clock class
    PTP_CC_SLAVE_ONLY = 255                     ///< The clock is slave ony
} PtpClockClass;

/**
 * @brief Standard clock accuracy definitions.
 */
typedef enum {
    PTP_CA_25NS = 0x20,    ///< Accurate to within 25ns
    PTP_CA_100NS,          ///< Accurate to within 100ns
    PTP_CA_250NS,          ///< Accurate to within 250ns
    PTP_CA_1US,            ///< Accurate to within 1us
    PTP_CA_2_5US,          ///< Accurate to within 2.5us
    PTP_CA_10US,           ///< Accurate to within 10us
    PTP_CA_25US,           ///< Accurate to within 25us
    PTP_CA_100US,          ///< Accurate to within 100us
    PTP_CA_250US,          ///< Accurate to within 250us
    PTP_CA_1MS,            ///< Accurate to within 1ms
    PTP_CA_2_5MS,          ///< Accurate to within 2.5ms
    PTP_CA_10MS,           ///< Accurate to within 10ms
    PTP_CA_25MS,           ///< Accurate to within 25ms
    PTP_CA_100MS,          ///< Accurate to within 100ms
    PTP_CA_250MS,          ///< Accurate to within 250ms
    PTP_CA_1S,             ///< Accurate to within 1s
    PTP_CA_10S,            ///< Accurate to within 10s
    PTP_CA_GT10S,          ///< Accurate to > 10s
    PTP_CA_UNKNOWN = 0xFE, ///< Accuracy is unknown
} PtpClockAccuracy;

/**
 * @brief Standard PTP time source definitions.
 */
typedef enum {
    PTP_TSRC_ATOMIC_CLOCK = 0x10,       ///< The clock is directly connected to such a device and using the PTP timescale
    PTP_TSRC_GPS = 0x20,                ///< The clock is synchronized to a satellite system that distribtues time and frequency tied to international standards
    PTP_TSRC_TERRESTRIAL_RADIO = 0x30,  ///< The clock is synchronized via any type of radio distribution system that is tied to international standards
    PTP_TSRC_PTP = 0x40,                ///< The clock is synchronized to a different PTP domain
    PTP_TSRC_NTP = 0x50,                ///< The clock is synchronized via NTP or SNTP
    PTP_TSRC_HAND_SET = 0x60,           ///< The clock has been set by means of a human interface based on observation of an international standard source of time within the claimed clock accuracy.
    PTP_TSRC_OTHER = 0x90,              ///< Any of source not covered by other values
    PTP_TSRC_INTERNAL_OSCILLATOR = 0xA0 ///< Undisciplined, free running oscillator
} PtpTimeSource;

#define PTP_VARIANCE_HAS_NOT_BEEN_COMPUTED (0xFFFF)

/**
 * @brief Contents of a PTP Announce message without the common PTP header.
 */
typedef struct {
    uint16_t currentUTCOffset;         ///< Current UTC Offset
    uint8_t priority1;                 ///< Priority 1
    uint8_t grandmasterClockClass;     ///< Grandmaster Clock Class
    uint8_t grandmasterClockAccuracy;  ///< Grandmaster Clock Accuracy
    uint16_t grandmasterClockVariance; ///< Grandmaster Clock Variance
    uint8_t priority2;                 ///< Priority 2
    uint64_t grandmasterClockIdentity; ///< Grandmaster Clock Identity
    uint16_t localStepsRemoved;        ///< Local Steps Removed
    uint8_t timeSource;                ///< Time Source
} PtpAnnounceBody;

typedef PtpAnnounceBody PtpMasterProperties;

/**
 * @brief Core state machine states.
 */
typedef enum PtpM2SState {
    SIdle,        ///< Idle
    SWaitFollowUp ///< Waiting for a Follow Up message
} PtpM2SState;

/**
 * @brief BMCA master states.
 */
typedef enum {
    BMCA_NO_MASTER = 0, ///< No master
    BMCA_MASTER_REMOTE, ///< A remote master is found to be the best
    BMCA_MASTER_ME,     ///< I am the best master, yeey!
} BmcaMasterState;

/**
 * @brief BMCA candidate states.
 */
typedef enum {
    BMCA_NO_CANDIDATE = 0,    ///< No candidate on the network
    BMCA_CANDIDATE_COLLECTION ///< Collecting candidates, we are in the candidate collection time window
} BmcaCandidateState;

/**
 * @brief Enumeration for logarithmic message period boundaries.
 */
typedef enum {
    PTP_LOGPER_MIN = -3,         ///< Minimal logarithmic messaging period
    PTP_LOGPER_MAX = 4,          ///< Maximal logarithmic messaging period
    PTP_LOGPER_SYNCMATCHED = 127 ///< Messaging occurs whenever a Sync arrives
} PtpLogMsgPeriods;

typedef enum {
    PTP_BMCA_INITIALIZING = 0x00,
    PTP_BMCA_LISTENING,
    PTP_BMCA_PRE_MASTER,
    PTP_BMCA_MASTER,
    PTP_BMCA_SLAVE,
    PTP_BMCA_PASSIVE,
    PTP_BMCA_UNCALIBRATED,
    PTP_BMCA_FAULTY,
    PTP_BMCA_DISABLED
} PtpBmcaFsmState;

/**
 * @brief BMCA state.
 */
typedef struct {
    PtpBmcaFsmState state;           ///< BMCA state
    PtpMasterProperties masterProps; ///< Master clock properties
    uint16_t masterAnnPer_ms;        ///< Message period of current master
    uint16_t masterTOCntr;           ///< Current master announce dropout counter
    uint32_t stateDuration;          ///< Heartbeat cycles since last state transition
    bool preventMasterSwitchOver;    ///< Set if master switchover is prohibited
} PtpBmcaState;

/**
 * @brief PTP profile additional data list element.
 */
typedef struct _PtpProfileAdditionalData {
    const void *data;                       ///< Pointer to the data block
    uint16_t size;                          ///< Size of the data block
    PtpMessageType msgType;                 ///< Message type into which this extension should be inserted
    struct _PtpProfileAdditionalData *next; ///< Pointer to the next element of the list
} PtpProfileTlvElement;

/**
 * @brief PTP profile flags
 */
typedef enum {
    PTP_PF_NONE = 0x00,                                       ///< Empty profile flags
    PTP_PF_ISSUE_SYNC_FOR_COMPLIANT_SLAVE_ONLY_IN_P2P = 0x01, ///< Send Sync messages only for a compliant peer in P2P mode
    PTP_PF_SLAVE_ONLY = 0x02,                                 ///< Operating only in SLAVE mode
    PTP_PF_N                                                  ///< Number of available PTP profile flags
} PtpProfileFlags;

/**
 * @brief PTP profile structure.
 */
typedef struct {
    PtpTransportType transportType;                  ///< transport layer
    PtpTransportSpecific transportSpecific;          ///< majorSdoId ('transportSpecific')
    PtpDelayMechanism delayMechanism;                ///< delay mechanism
    int8_t logDelayReqPeriod;                        ///< logarithm of (P)DelayReq period (SLAVE and MASTER)
    int8_t logSyncPeriod;                            ///< logarithm of the Sync transmission period (MASTER only)
    int8_t logAnnouncePeriod;                        ///< logarithm of the Announce period (MASTER only)
    uint8_t domainNumber;                            ///< PTP domain number
    uint8_t flags;                                   ///< Flags associated with this profile
    char tlvSet[PTP_MAX_TLV_PRESET_NAME_LENGTH + 1]; ///< Name of the corresponding TLV set
} PtpProfile;

/**
 * @brief PTP TLV types.
 */
typedef enum {
    /* Standard TLVs */
    PTP_TLV_MANAGEMENT = 0x01,       ///< Management extension
    PTP_TLV_MANAGEMENT_ERROR_STATUS, ///< Management error status
    PTP_TLV_ORGANIZATION_EXTENSION,  ///< Organization extension

    /* Optional unicast message negotitation TLVs */
    PTP_TLV_REQUEST_UNICAST_TRANSMISSION = 0x04,
    PTP_TLV_GRANT_UNICAST_TRANSMISSION,
    PTP_TLV_CANCEL_UNICAST_TRANSMISSION,
    PTP_TLV_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION,

    /* Optional path trace mechanism TLV */
    PTP_TLV_PATH_TRACE = 0x08,

    /* Optional alternate timescale TLV */
    PTP_TLV_ALTERNATE_TIME_OFFSET_INDICATOR = 0x09,

    /* Security TLVs */
    PTP_TLV_AUTHENTICATION = 0x2000,
    PTP_TLV_AUTHENTICATION_CHALLENGE = 0x2001,
    PTP_TLV_SECURITY_ASSOCIATION_UPDATE = 0x2002,

    /* Cumulative frequency scale factor offset */
    PTP_TLV_CUM_FREQ_SCALE_FACTOR_OFFSET = 0x2003
} PtpTlvType;

#define PTP_TLV_HEADER                       \
    uint16_t type;   /**< Type of the TLV */ \
    uint16_t length; /**< Length of this TLV*/

/**
 * @brief PTP TLV
 */
typedef struct {
    PTP_TLV_HEADER
} PtpTlvHeader;

/**
 * @brief PTP slave messaging state structure.
 */
typedef struct {
    uint16_t sequenceID, delay_reqSequenceID; ///< last sequence IDs
    uint16_t lastRespondedDelReqId;           ///< ID of the last (P)Delay_Req got responded
    PtpM2SState m2sState;                     ///< Sync-FollowUp state
    int8_t logSyncPeriod;                     ///< logarithm of Sync interval
    uint16_t syncPeriodMs;                    ///< Sync interval in milliseconds
} PtpSlaveMessagingState;

/**
 * @brief PTP master messaging state structure.
 */
typedef struct {
    uint16_t syncSequenceID;     ///< Sequence ID of the coming Sync
    uint16_t announceSequenceID; ///< Sequence ID of the next Announce message
} PtpMasterMessagingState;

/**
 * @brief PTP P2P slave state viewed from the MASTER.
 */
typedef enum {
    PTP_P2PSS_NONE = 0,   ///< No slave is detected
    PTP_P2PSS_CANDIDATE,  ///< A slave has reported in at least once, now being checked on
    PTP_P2PSS_ESTABLISHED ///< The slave is considered stable and ready
} PtpP2PSlaveState;

/**
 * @brief PTP P2P slave info structure;
 */
typedef struct {
    PtpP2PSlaveState state; ///< Indicates that a slave is responding to our PDELAY_REQ messages
    uint64_t identity;      ///< The clock identity of the connected, operating P2P slave
    uint16_t reportCount;   ///< Number of times the slave had reported in
    uint16_t dropoutCntr;   ///< Dropout watchdog counter for resetting the state machine if the slave went silent
} PtpP2PSlaveInfo;

/**
 * @brief Hardware clock state.
 */
typedef struct {
#ifdef PTP_ADDEND_INTERFACE
    uint32_t addend; ///< hardware clock addend value
#elif defined(PTP_HLT_INTERFACE)
    float tuning_ppb; ///< normalized hardware tuning
#endif
} PtpHWClockState;

/**
 * @brief Network state.
 */
typedef struct {
    TimestampI meanPathDelay; ///< mean path delay
} PtpNetworkState;

/**
 * @brief Structure for statistics.
 */
typedef struct {
    double filtTimeErr; ///< 0.1Hz lowpass-filtered time error
    bool locked;        ///< is the PTP locked to defined limit?
} PtpStats;

/**
 * Sync callback type prototype.
 */
typedef void (*PtpSyncCallback)(int64_t time_error, const PtpSyncCycleData *pSCD,
#ifdef PTP_ADDEND_INTERFACE
                                uint32_t freqCodeWord
#elif defined(PTP_HLT_INTERFACE)
                                float tuning_ppb
#endif
);

/**
 * User event callback prototype.
 */
typedef void (*PtpUserEventCallback)(PtpUserEventCode uev);

/**
 * Fast compensation states.
 */
typedef enum { PTP_FC_IDLE = 0,                    ///< Fast correction algorithm is IDLE
               PTP_FC_SKEW_CORRECTION,             ///< Skew correction is running
               PTP_FC_TIME_CORRECTION,             ///< Time correction is running
               PTP_FC_TIME_CORRECTION_PROPAGATION, ///< Waiting for the effects of time correction to propagate
} PtpFastCompState;

/**
 * @brief Giant PTP core state object.
 */
typedef struct {
    PtpProfile profile;               ///< PTP profile
    PtpMasterProperties capabilities; ///< The capabilities of this device

    PtpHWClockState hwclock; ///< Hardware clock state

    struct {
        TimestampI offset;      ///< PPS signal offset
        uint64_t clockIdentity; ///< clockIdentity calculated from MAC address
    } hwoptions;                ///< Hardware options

    PtpBmcaState bmca;       ///< BMCA state
    PtpNetworkState network; ///< Network state

    // Logging
    struct {
        bool def;        ///< default
        bool corr;       ///< correction fields
        bool timestamps; ///< timestamps
        bool info;       ///< informative messages
        bool locked;     ///< clock lock state change
        bool bmca;       ///< BMCA state change
    } logging;           ///< Logging

    PtpStats stats;                   ///< Statistics
    PtpUserEventCallback userEventCb; ///< User event callback pointer

    /* ---- SLAVE ----- */

    struct {
        bool enabled; ///< Slave module is enabled

        PtpSlaveMessagingState messaging; ///< Messaging state
        PtpSyncCycleData scd;             ///< Sync cycle data
        bool expectPDelRespFollowUp;      ///< Expect a PDelay_Resp_Follow_Up message
        PtpFastCompState fastCompState;   ///< State of fast compensation
        uint8_t fastCompCntr;             ///< Cycle counter for fast compensation
        TimestampI prevSyncMa;            ///< T1 from the previous cycle
        TimestampI prevSyncSl;            ///< T2 from the previous cycle
        TimestampI prevTimeError;         ///< Time error in the previous cycle
        uint64_t coarseLimit;             ///< time error limit above coarse correction is engaged

        uint32_t delReqTickPeriod; ///< ticks between Delay_Req transmissions
        uint32_t delReqTmr;        ///< Timer counting ticks for Delay_Req transmissions

        PtpSyncCallback syncCb; ///< Sync callback invoked in every synchronization cycle
    } slave;

    /* ---- MASTER ----- */

    struct {
        bool enabled; ///< Master module is enabled

        PtpP2PSlaveInfo p2pSlave;      ///< Information on the connected P2P slave (only used in P2P modes)
        uint32_t pdelay_reqSequenceID; ///< Sequence number of the last PDelay_Request sent
        PtpSyncCycleData scd;          ///< Sync cycle data
        bool expectPDelRespFollowUp;   ///< Expect a PDelay_Resp_Follow_Up message

        PtpMasterMessagingState messaging; ///< Messaging state

        uint32_t syncTickPeriod;      ///< Sync transmission period in ticks
        uint32_t syncTmr;             ///< Counter for scheduling Sync transmission
        uint32_t announceTickPeriod;  ///< Announce transmission in ticks
        uint32_t announceTmr;         ///< Counter to schedule Announce transmission
        uint32_t pdelayReqTickPeriod; ///< PDelayReq transmission period in ticks
        uint32_t pdelayReqTmr;        ///< Counter for PDelayReq transmission scheduling
    } master;
} PtpCoreState;

#endif /* FLEXPTP_PTP_TYPES_H_ */
