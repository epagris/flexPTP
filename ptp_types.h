/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_PTP_TYPES_H_
#define FLEXPTP_PTP_TYPES_H_

#include <flexptp/ptp_sync_cycle_data.h>
#include <stdint.h>
#include <stdbool.h>

#include <flexptp_options.h>

#ifndef SIMULATION
#include "FreeRTOS.h"
#include "timers.h"
#endif

// PTP packet types
enum PTPMessageType {
    PTP_MT_Sync = 0,
    PTP_MT_Delay_Req = 1,

    PTP_MT_Follow_Up = 8,
    PTP_MT_Delay_Resp = 9,

    PTP_MT_Announce = 11
};

// PTP header control field values
enum PTPControl {
    PTP_CON_Sync = 0,
    PTP_CON_Delay_Req = 1,
    PTP_CON_Follow_Up = 2,
    PTP_CON_Delay_Resp = 3,
    PTP_CON_Other = 5
};

// PTP flags structure
typedef struct {
    bool PTP_SECURITY;
    bool PTP_ProfileSpecific_2;
    bool PTP_ProfileSpecific_1;
    bool PTP_UNICAST;
    bool PTP_TWO_STEP;
    bool PTP_ALTERNATE_MASTER;
    bool FREQUENCY_TRACEABLE;
    bool TIME_TRACEABLE;
    bool PTP_TIMESCALE;
    bool PTP_UTC_REASONABLE;
    bool PTP_LI_59;
    bool PTP_LI_61;
} PTPFlags;

// PTP message header structure
typedef struct {
    // 0.
    uint8_t messageType;        // ID
    uint8_t transportSpecific;

    // 1.
    uint8_t versionPTP;         // PTP version
    uint8_t _r1;

    // 2-3.
    uint16_t messageLength;     // length

    // 4.
    uint8_t domainNumber;

    // 5.
    uint8_t _r2;

    // 6-7.
    PTPFlags flags;

    // 8-15.
    uint64_t correction_ns;
    uint32_t correction_subns;

    // 16-19.
    uint32_t _r3;

    // 20-27.
    uint64_t clockIdentity;

    // 28-29
    uint16_t sourcePortID;

    // 30-31.
    uint16_t sequenceID;

    // 32.
    uint8_t control;

    // 33.
    int8_t logMessagePeriod;    // ...
} PtpHeader;

// identification carrying Delay_Resp message
typedef struct {
    uint64_t requestingSourceClockIdentity;
    uint16_t requestingSourcePortIdentity;
} Delay_RespIdentification;

typedef enum {
    PTP_TP_IPv4 = 0,

} PtpTransportType;

typedef enum {
    PTP_DM_E2E = 0,

} PtpDelayMechanism;

typedef enum {
    PTP_TSPEC_UNKNOWN_DEF = 0,

} PtpTransportSpecific;

typedef enum {
    PTP_MC_EVENT = 0,
    PTP_MC_GENERAL = 1
} PtpMessageClass;

#define MAX_PTP_MSG_SIZE (128)
typedef struct RawPtpMessage_ {
    TimestampI ts;              // timestamp
    uint32_t size;              // packet size

    // --- transmit related ---
    TimestampI *pTs;            // pointer to timestamp
    void (*pTxCb)(const struct RawPtpMessage_ * pMsg);  // transmit callback function
    PtpDelayMechanism tx_dm;    // transmit transport type
    PtpMessageClass tx_mc;      // transmit message class

    // --- data ---
    uint8_t data[MAX_PTP_MSG_SIZE];     // raw packet data
} RawPtpMessage;

// contents of an announce message
typedef struct {
    uint16_t originCurrentUTCOffset;
    uint8_t priority1;
    uint8_t grandmasterClockClass;
    uint8_t grandmasterClockAccuracy;
    uint16_t grandmasterClockVariance;
    uint8_t priority2;
    uint64_t grandmasterClockIdentity;
    uint16_t localStepsRemoved;
    uint8_t timeSource;
} PtpAnnounceBody;

typedef PtpAnnounceBody PtpMasterProperties;

// core state machine states
typedef enum PtpM2SState {
    SIdle, SWaitFollowUp
} PtpM2SState;

typedef enum {
    SBMC_NO_MASTER = 0,
    SBMC_MASTER_OK
} SBmcMasterState;

typedef enum {
    SBMC_NO_CANDIDATE = 0,
    SBMC_CANDIDATE_COLLECTION
} SBmcCandidateState;

typedef enum {
    PTP_LOGPER_MIN = -4,
    PTP_LOGPER_MAX = 4,
    PTP_LOGPER_SYNCMATCHED = 127
} PtpLogMsgPeriods;

typedef struct {
    PtpMasterProperties masterProps;    // master clock properties
    uint16_t masterAnnPer_ms;   // message period of current master
    PtpMasterProperties candProps;      // new master candidate properties
    uint16_t candAnnPer_ms;     // message period for master candidate
    bool preventMasterSwitchOver;       // set if master switchover is prohibited
    SBmcMasterState mstState;   // master clock state machine
    SBmcCandidateState candState;       // candidate state machine
    uint8_t candCntr;           // counter before switching master
    uint16_t masterTOCntr;      // current master announce dropout counter
    uint16_t candTOCntr;        // candidate master announce cntr
} PtpSBmcState;

typedef struct {
    PtpTransportType transportType;     // transport layer
    PtpTransportSpecific transportSpecific;     // majorSdoId ('transportSpecific')
    PtpDelayMechanism delayMechanism;   // delay mechanism
    int8_t logDelayReqPeriod;   // logarithm of (P)DelayReq period
    uint8_t domainNumber;       // PTP domain number
} PtpProfile;

typedef struct {
    uint16_t sequenceID, delay_reqSequenceID;   // last sequence IDs
    uint16_t lastRespondedDelReqId;     // ID of the last (P)Delay_Req got responded
    PtpM2SState m2sState;       // Sync-FollowUp state
    int8_t logSyncPeriod;       // logarithm of Sync interval
    uint16_t syncPeriodMs;      // ...
} PtpMessagingState;

typedef struct {
    uint32_t addend;            // hardware clock addend value
} PtpHWClockState;

typedef struct {
    TimestampI meanPathDelay;   // mean path delay
} PtpNetworkState;

typedef struct {
    double filtTimeErr;         // 0.1Hz lowpass-filtered time error
    bool locked;                // is the PTP locked to defined limit?
} PtpStats;

typedef struct {
    // PTP profile
    PtpProfile profile;
    struct {
        uint16_t delReqPeriodMs;
    } auxProfile;               // auxiliary, auto-calculated values for the selected profile

    // Messaging state
    PtpMessagingState messaging;

    // Sync cycle data
    PtpSyncCycleData scd;

    // Hardware clock state
    PtpHWClockState hwclock;
    struct {
        TimestampI offset;      // PPS signal offset
        uint64_t clockIdentity; // clockIdentity calculated from MAC address
    } hwoptions;

    // Slave BMC state
    PtpSBmcState sbmc;

    // Network state
    PtpNetworkState network;

    // Logging
    struct {
        bool def;               // default
        bool corr;              // correction fields
        bool timestamps;        // timestamps
        bool info;              // informative messages
        bool locked;            // clock lock state change
    } logging;

    // Statistics
    PtpStats stats;

    // Timers...
    struct {
        TimerHandle_t sbmc;     // timer for managing SBMC operations
        TimerHandle_t delreq;   // timer for (P)Del_Req
    } timers;
} PtpCoreState;

// global storable-loadable configuration
typedef struct {
    PtpProfile profile;         // PTP-profile
    TimestampI offset;          // PPS signal offset
    uint32_t logging;           // logging compressed into a single bitfield
} PtpConfig;

#endif                          /* FLEXPTP_PTP_TYPES_H_ */
