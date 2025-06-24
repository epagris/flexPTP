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
	CLILOG(S.logging.locked && (locked != S.stats.locked), "PTP %s!\n", locked ? "LOCKED" : "DIVERGED");
	
	// invoke LOCKED/UNLOCKED event
	if (locked != S.stats.locked) {
		PTP_IUEV(locked ? PTP_UEV_LOCKED : PTP_UEV_UNLOCKED);
	}

	S.stats.locked = locked;
}
