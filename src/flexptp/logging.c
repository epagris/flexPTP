#include "logging.h"
#include "ptp_core.h"

#include <flexptp_options.h>

///\cond 0
#define S (gPtpCoreState)
///\endcond

// enable/disable general logging
static void ptp_log_def_en(bool en) {
    if (en) { // on turning on
        MSG("\n\nT1 [s] | T1 [ns] | T4 [s] | T4 [ns] | Dt [s] | Dt [ns] |"
#ifdef PTP_ADDEND_INTERFACE
            " Dt [tick] | addend |"
#elif defined(PTP_HLT_INTERFACE)
            " tuning_ppb |"
#endif
            " corr_ppb | mpd_ns | sync_period_ns\n\n");
    }
}

// ------------------------

/**
 * Prototype for callbacks that get called when a particular kind of log is turned on or off.
 *
 * @param en indicates if loggin is enabled of disabled
 */
typedef void (*LogEnFn)(bool en);

/**
 * @brief PTP log pair.
 */
typedef struct {
    int id;          ///< ID of log type
    LogEnFn logEnFn; ///< Callback function on turning on/off logging
    bool *en;        ///< variable storing log state
} PtpLogPair;

static PtpLogPair sLogTable[PTP_LOG_N + 1] = {
    {PTP_LOG_DEF, ptp_log_def_en, &(S.logging.def)},
    {PTP_LOG_CORR_FIELD, NULL, &(S.logging.corr)},
    {PTP_LOG_TIMESTAMPS, NULL, &(S.logging.timestamps)},
    {PTP_LOG_INFO, NULL, &(S.logging.info)},
    {PTP_LOG_LOCKED_STATE, NULL, &(S.logging.locked)},
    {PTP_LOG_BMCA, NULL, &(S.logging.bmca)},
    {-1, NULL, NULL}};

void ptp_log_enable(int logId, bool en) {
    PtpLogPair *pIter = sLogTable;
    while (pIter->id != -1) {
        if (pIter->id == logId && *(pIter->en) != en) { // if callback is found and changing state indeed
            if (pIter->logEnFn != NULL) {               // callback function is not necessary
                pIter->logEnFn(en);
            }
            *(pIter->en) = en;
            break;
        }
        pIter++;
    }
}

void ptp_log_disable_all() {
    PtpLogPair *pIter = sLogTable;
    while (pIter->logEnFn != NULL) {
        if (pIter->logEnFn != NULL) {
            pIter->logEnFn(false);
        }
        *(pIter->en) = false;
        pIter++;
    }
}
