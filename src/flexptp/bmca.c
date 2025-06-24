#include "bmca.h"

#include "clock_utils.h"
#include "event.h"
#include "format_utils.h"
#include "ptp_core.h"
#include "ptp_defs.h"
#include "ptp_types.h"
#include "task_ptp.h"

#include <flexptp_options.h>
#include <stdint.h>
#include <string.h>

///\cond 0
// global state
#define S (gPtpCoreState)
///\endcond

// ------------

#define COMPARE_AND_RETURN(pmp1, pmp2, field)   \
    {                                           \
        if (pmp1->field < pmp2->field) {        \
            return 0;                           \
        } else if (pmp1->field > pmp2->field) { \
            return 1;                           \
        }                                       \
    }

int ptp_select_better_master(PtpMasterProperties *pMP1, PtpMasterProperties *pMP2) {
    COMPARE_AND_RETURN(pMP1, pMP2, priority1);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockClass);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockAccuracy);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockVariance);
    COMPARE_AND_RETURN(pMP1, pMP2, priority2);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockIdentity);
    return 1;
}

static char *BMCA_HINTS[] = {
    "INITIALIZING",
    "LISTENING",
    "PRE_MASTER",
    "MASTER",
    "SLAVE",
    "PASSIVE",
    "UNCALIBRATED",
    "FAULTY",
    "DISABLED"};

#define SBMC_PRINT_LOG(p, c) \
    if (S.logging.bmca)      \
        MSG("%s -> %s\n", BMCA_HINTS[(p)], BMCA_HINTS[(c)]);

// handle possible state change
static void ptp_bmca_handle_state_change(PtpBmcaFsmState state) {
    if (S.bmca.state != state) {
        // print log
        SBMC_PRINT_LOG(S.bmca.state, state);

        // store state
        S.bmca.stateDuration = 0;
        S.bmca.state = state;

        // dispatch event
        PtpCoreEvent event = {.code = PTP_CEV_BMCA_STATE_CHANGED, .w = state, .dw = 0};
        ptp_event_enqueue(&event);
    }
}

void ptp_bmca_tick() {
    PtpBmcaFsmState state = S.bmca.state;
    uint32_t sd = S.bmca.stateDuration;
    bool master_mode_enabled = PTP_ENABLE_MASTER_OPERATION && (!(S.profile.flags & PTP_PF_SLAVE_ONLY));

    uint64_t bmCI, ourCI;
    bmCI = S.bmca.masterProps.grandmasterClockIdentity;
    ourCI = S.capabilities.grandmasterClockIdentity;

    switch (state) {
    case PTP_BMCA_INITIALIZING: {
        state = PTP_BMCA_LISTENING; // next state should be LISTENING

        if (master_mode_enabled) {               // if master operation is enabled
            S.bmca.masterProps = S.capabilities; // in the beginning, let's assume we're the best master
        }
    } break;
    case PTP_BMCA_LISTENING: {
        if (sd > (PTP_BMCA_LISTENING_TIMEOUT_MS / PTP_HEARTBEAT_TICKRATE_MS)) { // if the LISTENING state residence time has expired...
            if (master_mode_enabled && (bmCI == ourCI)) {                       // if it's us who takes the MASTER role...
                state = PTP_BMCA_PRE_MASTER;
            } else {                                                              // if we should follow a remote master
                if ((bmCI != ourCI) && (bmCI != ~((uint64_t)0)) && (bmCI != 0)) { // us as a master and extrema cases exluded
                    state = PTP_BMCA_UNCALIBRATED;                                // leave only listening state if some remote master has ennounced itself
                }
            }
        }
    } break;
    case PTP_BMCA_PRE_MASTER: {
        if (sd > (PTP_MASTER_QUALIFICATION_TIMEOUT * ptp_logi2ms(S.profile.logAnnouncePeriod) / PTP_HEARTBEAT_TICKRATE_MS)) {
            state = PTP_BMCA_MASTER;
        }
    } break;
    case PTP_BMCA_UNCALIBRATED: {
        state = PTP_BMCA_SLAVE;
    } break;
    case PTP_BMCA_SLAVE: {
        if (S.bmca.masterTOCntr++ > (PTP_ANNOUNCE_RECEIPT_TIMEOUT * S.bmca.masterAnnPer_ms / PTP_HEARTBEAT_TICKRATE_MS)) { // if a master dropout is detected
            if (master_mode_enabled) {
                S.bmca.masterProps = S.capabilities; // let's assume we are the best MASTER for this time
            } else {
                memset(&S.bmca.masterProps, 0xFF, sizeof(PtpMasterProperties)); // fill the master properties with the worst values regarding the comparison
            }
            state = PTP_BMCA_LISTENING;
        }
    } break;
    default:
        break;
    }

    // increase state duration
    S.bmca.stateDuration++;

    // handle possible state change
    ptp_bmca_handle_state_change(state);
}

/**
 * Handle announce messages.
 *
 * @param pAnn pointer to PtpAnnounceBody object
 * @param pHeader pointer to Announce message header
 */
void ptp_handle_announce_msg(PtpAnnounceBody *pAnn, PtpHeader *pHeader) {
    PtpBmcaState *s = &(S.bmca);
    bool masterChanged = false;
    PtpBmcaFsmState state = s->state;
    bool master_mode_enabled = PTP_ENABLE_MASTER_OPERATION && (!(S.profile.flags & PTP_PF_SLAVE_ONLY));

    switch (state) {
    case PTP_BMCA_LISTENING:
        if (master_mode_enabled) {                                        // if master operation is enabled...
            if (ptp_select_better_master(pAnn, &(s->masterProps)) == 0) { // compare the new master to the best one we've discovered so far
                s->masterProps = *pAnn;                                   // store remote master's capabilities if it's better then the former one
                masterChanged = true;                                     // signal that master has changed
            }
        } else {                           // slave only operation
            s->masterProps = *pAnn;        // retain remote master capabilities
            state = PTP_BMCA_UNCALIBRATED; // change to uncalibrated state
            masterChanged = true;          // ...
        }
        break;
    case PTP_BMCA_PRE_MASTER:
    case PTP_BMCA_MASTER:
    case PTP_BMCA_SLAVE: {
        // if a better master is found, then switch over
        if (ptp_select_better_master(pAnn, &(s->masterProps)) == 0) {
            s->masterProps = *pAnn;
            state = PTP_BMCA_UNCALIBRATED;
            masterChanged = true;
        }

        // update UTC offset if necessary
        if (pAnn->currentUTCOffset > S.capabilities.currentUTCOffset) {
            S.capabilities.currentUTCOffset = pAnn->currentUTCOffset;
        }
    } break;
    default:
        break;
    }

    // if master has changed, then calculate Announce period
    if (masterChanged) {
        S.bmca.masterTOCntr = 0;
        s->masterAnnPer_ms = ptp_logi2ms(pHeader->logMessagePeriod);
    }

    // clear Master timeout if relevant Announce has arrived
    if (pAnn->grandmasterClockIdentity == s->masterProps.grandmasterClockIdentity) {
        if (s->masterProps.currentUTCOffset != pAnn->currentUTCOffset) { // update current UTC offset
            s->masterProps.currentUTCOffset = pAnn->currentUTCOffset;
        }
        S.bmca.masterTOCntr = 0;
    }

    // handle possible state change
    ptp_bmca_handle_state_change(state);
}

void ptp_bmca_init() {
    // initialize device capabilities
    PtpMasterProperties *caps = &S.capabilities;
    caps->currentUTCOffset = PTP_FALLBACK_UTC_OFFSET;
    caps->priority1 = PTP_CLOCK_PRIORITY1;
    caps->grandmasterClockClass = PTP_BEST_CLOCK_CLASS;
    caps->grandmasterClockAccuracy = PTP_WORST_ACCURACY;
    caps->grandmasterClockVariance = PTP_VARIANCE_HAS_NOT_BEEN_COMPUTED;
    caps->priority2 = PTP_CLOCK_PRIORITY2;
    caps->grandmasterClockIdentity = S.hwoptions.clockIdentity;
    caps->localStepsRemoved = 0;
    caps->timeSource = PTP_TIME_SOURCE;
}

void ptp_bmca_destroy() {
    return;
}

void ptp_bmca_reset() {
    memset(&S.bmca, 0, sizeof(PtpBmcaState)); // SBMC state
    S.bmca.state = PTP_BMCA_INITIALIZING;
}