/* The flexPTP project, (C) András Wiesner, 2024 */

#include <flexptp/ptp_core.h>
#include <flexptp/ptp_defs.h>
#include <flexptp/stats.h>
#include "ptp_core.h"
#include <memory.h>

#include <math.h>

#define S (gPtpCoreState)

// clear statistics
void ptp_clear_stats()
{
    memset(&S.stats, 0, sizeof(PtpStats));
}

// get statistics
const PtpStats *ptp_get_stats()
{
    return &S.stats;
}

// filter parameters for statistics calculation
// WARNING: Data calculation won't be totally accurate due to changing sampling time!

#define PTP_TE_FILT_Fc_HZ (0.1) // cutoff frequency (Hz)
#define PTP_TE_FILT_Ts_S (1)    // sampling time (s)

// collect statistics
void ptp_collect_stats(int64_t d)
{
    double a = exp(-PTP_TE_FILT_Fc_HZ * 2 * M_PI * (S.messaging.syncPeriodMs / 1000.0));

    // performing time error filtering
    double y_prev = S.stats.filtTimeErr, y;
    y = a * y_prev + (1 - a) * d;       // filtering equation
    S.stats.filtTimeErr = y;

    // set locked state
    bool locked = ((fabs(S.stats.filtTimeErr) < (PTP_ACCURACY_LIMIT_NS)) && (ptp_get_current_master_clock_identity() != 0));
    CLILOG(S.logging.locked && (locked != S.stats.locked), "PTP %s!", locked ? "LOCKED" : "DIVERGED");
    S.stats.locked = locked;
}
