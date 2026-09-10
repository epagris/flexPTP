#include "stats.h"

#include <memory.h>
#include <math.h>

#include "event.h"
#include "ptp_core.h"
#include "ptp_defs.h"
#include "settings_interface.h"


///\cond 0
#define S (gPtpCoreState)
///\endcond

// clear statistics
void ptp_clear_stats() {
	memset(&S.stats, 0, sizeof(PtpStats));
	S.stats.filtTimeErr = 100 * PTP_ACCURACY_LIMIT_NS; // prevent strange LOCKED-UNLOCKED-LOCKED series when starting up
}

// get statistics
const PtpStats* ptp_get_stats() {
	return &S.stats;
}
// filter parameters for statistics calculation
// WARNING: Data calculation won't be totally accurate due to unvertain sampling time!

#define PTP_TE_FILT_Fc_HZ (0.1) ///< Cutoff frequency (Hz)

// collect statistics
void ptp_collect_stats(int64_t d) {
	double a = exp(-PTP_TE_FILT_Fc_HZ * 2 * M_PI * (S.slave.messaging.syncPeriodMs / 1000.0));

	// performing time error filtering
	double y_prev = S.stats.filtTimeErr, y;
	y = a * y_prev + (1 - a) * d; // filtering equation
	S.stats.filtTimeErr = y;

	// set locked state
	bool locked = ((fabs(S.stats.filtTimeErr) < (PTP_ACCURACY_LIMIT_NS)) && (ptp_get_current_master_clock_identity() != 0));
	// The prefix carries the SAME condition as the line it prefixes. Without the state-change
	// term it printed on every call -- and ptp_collect_stats() runs once per Sync -- so the
	// console filled with bare "[LOG-LCKD] " markers and no messages behind them.
	bool lockStateChanged = (locked != S.stats.locked);
	CLILOG(S.logging.logid && S.logging.locked && lockStateChanged, "[LOG-LCKD] ");
	CLILOG(S.logging.locked && lockStateChanged, "PTP %s!\n", locked ? "LOCKED" : "DIVERGED");
	
	// invoke LOCKED/UNLOCKED event
	if (locked != S.stats.locked) {
		PTP_IUEV(locked ? PTP_UEV_LOCKED : PTP_UEV_UNLOCKED);
	}

	S.stats.locked = locked;
}
