/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef PTP_H
#define PTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef SIMULATION
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#endif

#include <flexptp/timeutils.h>
#include <flexptp/ptp_types.h>
#include <flexptp/stats.h>
#include <flexptp/settings_interface.h>

// -------------------------------------------
// ----- FLEXPTP_OPTIONS.H INCLUDE AREA ------
// -------------------------------------------

#include <flexptp_options.h>

// -------------------------------------------
// (End of customizable area)
// -------------------------------------------

// -------------------------------------------

    void ptp_init();            // initialize PTP subsystem
    void ptp_deinit();          // deinitialize PTP subsystem

    void ptp_reset();           // reset PTP subsystem
    void ptp_process_packet(RawPtpMessage * pRawMsg);   // process PTP packet

    typedef void (*SyncCallback)(int64_t time_error, const PtpSyncCycleData * pSCD, uint32_t freqCodeWord);

    void ptp_set_sync_callback(SyncCallback syncCB);

#define PTP_IS_LOCKED(th) (ptp_get_stats()->filtTimeErr < (th) && !(ptp_get_current_master_clock_identity() != 0))      // is ptp locked considering threshold passed?

    extern PtpCoreState gPtpCoreState;

    void ptp_store_config(PtpConfig * pConfig); // store PTP-engine configuration (param: output)
    void ptp_load_config(const PtpConfig * pConfig);    // load PTP-engine configuration
    void ptp_load_config_from_dump(const void *pDump);  // load PTP-engine configuration from binary dump (i.e. from unaligned address)

#ifdef __cplusplus
}
#endif
#endif                          /* PTP */
