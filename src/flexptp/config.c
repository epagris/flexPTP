#include "config.h"

#include <stdbool.h>

#include "ptp_core.h"

///\cond 0
#define S (gPtpCoreState)
///\endcond

// -------------

// config logging flags
#define CONFIG_LOG_DEF (0x01)        ///< Default logging
#define CONFIG_LOG_INFO (0x02)       ///< General info logging
#define CONFIG_LOG_CORR (0x04)       ///< Correction field logging
#define CONFIG_LOG_TIMESTAMPS (0x08) ///< Timestamp logging
#define CONFIG_LOG_LOCKED (0x10)     ///< Inform the user if the clock diverges or locks
#define CONFIG_LOG_BMCA (0x20)       ///< Peek BMCA state changes
#define CONFIG_LOG_ALL (0x3F)        ///< All logging options packed

///\cond 0
#define CONFIG_ADD_LOGGING(c, f) (((c) ? (f) : 0))
///\endcond

// -----------

void ptp_store_config(PtpConfig *pConfig) {
    pConfig->profile = S.profile;
    pConfig->offset = S.hwoptions.offset;
    pConfig->logging = CONFIG_ADD_LOGGING(S.logging.def, CONFIG_LOG_DEF) |
                       CONFIG_ADD_LOGGING(S.logging.info, CONFIG_LOG_INFO) |
                       CONFIG_ADD_LOGGING(S.logging.corr, CONFIG_LOG_CORR) |
                       CONFIG_ADD_LOGGING(S.logging.timestamps, CONFIG_LOG_TIMESTAMPS) |
                       CONFIG_ADD_LOGGING(S.logging.locked, CONFIG_LOG_LOCKED) |
                       CONFIG_ADD_LOGGING(S.logging.bmca, CONFIG_LOG_BMCA);
    pConfig->priority1 = S.capabilities.priority1;
    pConfig->priority2 = S.capabilities.priority2;
}

void ptp_load_config(const PtpConfig *pConfig) {
    // validate fields
    bool invalid = false;
    invalid |= pConfig->logging & (~CONFIG_LOG_ALL); // if at least a flag not corresponding to any kind of logging is set

    PtpDelayMechanism dm = pConfig->profile.delayMechanism;
    invalid |= (dm != PTP_DM_E2E) && (dm != PTP_DM_P2P);

    PtpLogMsgPeriods logmp = pConfig->profile.logDelayReqPeriod;
    invalid |= ((logmp > PTP_LOGPER_MAX) && (logmp != PTP_LOGPER_SYNCMATCHED)) || (logmp < PTP_LOGPER_MIN);

    PtpTransportSpecific tps = pConfig->profile.transportSpecific;
    invalid |= (tps != PTP_TSPEC_UNKNOWN_DEF) && (tps != PTP_TSPEC_GPTP_8021AS);

    PtpTransportType tpt = pConfig->profile.transportType;
    invalid |= (tpt != PTP_TP_IPv4) && (tpt != PTP_TP_802_3);

    // check validity
    if (invalid) {
        MSG("The retained flexPTP configuration got corrupted, loading aborted!\n");
        return;
    }

    // load config if all fields are valid
    S.profile = pConfig->profile;
    S.hwoptions.offset = pConfig->offset;
    S.capabilities.priority1 = pConfig->priority1;
    S.capabilities.priority2 = pConfig->priority2;

    S.logging.def = (pConfig->logging & CONFIG_LOG_DEF) != 0;
    S.logging.info = (pConfig->logging & CONFIG_LOG_INFO) != 0;
    S.logging.corr = (pConfig->logging & CONFIG_LOG_CORR) != 0;
    S.logging.timestamps = (pConfig->logging & CONFIG_LOG_TIMESTAMPS) != 0;
    S.logging.locked = (pConfig->logging & CONFIG_LOG_LOCKED) != 0;
    S.logging.bmca = (pConfig->logging & CONFIG_LOG_BMCA) != 0;
}

void ptp_load_config_from_dump(const void *pDump) {
    PtpConfig config;
    memcpy(&config, pDump, sizeof(PtpConfig));
    ptp_load_config(&config);
    ptp_reset();
}