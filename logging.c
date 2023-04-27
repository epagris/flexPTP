#include <flexptp/logging.h>
#include <flexptp/ptp_core.h>
#include <flexptp_options.h>

#define S (gPtpCoreState)

// enable/disable general logging
static void ptp_log_def_en(bool en)
{
    if (en) {                   // on turning on
        MSG("\n\nT1 [s] | T1 [ns] | T4 [s] | T4 [ns] | Dt [s] | Dt [ns] | Dt [tick] | Addend\n\n");
    }
}

// ------------------------

// PTP log pair
typedef struct {
    int id;                     // ID of log type
    void (*logEnFn)(bool);      // callback function on turning on/off logging
    bool *en;                   // variable storing log state
} PtpLogPair;

static PtpLogPair sLogTable[PTP_LOG_N + 1] = {
    {PTP_LOG_DEF, ptp_log_def_en, &(S.logging.def)},
    {PTP_LOG_CORR_FIELD, NULL, &(S.logging.corr)},
    {PTP_LOG_TIMESTAMPS, NULL, &(S.logging.timestamps)},
    {PTP_LOG_INFO, NULL, &(S.logging.info)},
    {PTP_LOG_LOCKED_STATE, NULL, &(S.logging.locked)},
    {-1, NULL, NULL}
};

void ptp_enable_logging(int logId, bool en)
{
    PtpLogPair *pIter = sLogTable;
    while (pIter->id != -1) {
        if (pIter->id == logId && *(pIter->en) != en) { // if callback is found and changing state indeed
            if (pIter->logEnFn != NULL) {       // callback function is not necessary
                pIter->logEnFn(en);
            }
            *(pIter->en) = en;
            break;
        }
        pIter++;
    }
}

void ptp_disable_all_logging()
{
    PtpLogPair *pIter = sLogTable;
    while (pIter->logEnFn != NULL) {
        if (pIter->logEnFn != NULL) {
            pIter->logEnFn(false);
        }
        *(pIter->en) = false;
        pIter++;
    }
}
