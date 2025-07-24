/* (C) András Wiesner, 2020-2022 */

#include "ptp_core.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"

#include "common.h"
#include "event.h"
#include "logging.h"
#include "master.h"
#include "portmacro.h"
#include "slave.h"
#include "timers.h"

#include "bmca.h"
#include "cli_cmds.h"
#include "clock_utils.h"
#include "format_utils.h"
#include "msg_utils.h"
#include "network_stack_driver.h"
#include "ptp_defs.h"
#include "ptp_types.h"
#include "settings_interface.h"
#include "stats.h"
#include "task_ptp.h"
#include "timeutils.h"

#include <flexptp_options.h>

// --------------------------------------

// provide our own MIN implementation
#ifdef MIN
#undef MIN
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

///\cond 0
// global state
PtpCoreState gPtpCoreState;
#define S (gPtpCoreState)

const TimestampI zeroTs = {0, 0}; // a zero timestamp
///\endcond

// --------------------------------------

/**
 * Heartbeat callback.
 *
 * @param timer timer handle
 */
static void ptp_heartbeat_tmr_cb(TimerHandle_t timer) {
    PtpCoreEvent event = {.code = PTP_CEV_HEARTBEAT, .w = 0, .dw = 0};
    ptp_event_enqueue(&event);
}

/**
 * Construct the heartbeat timer.
 */
static void ptp_create_heartbeat_tmr() {
    // create smbc timer
    S.timers.heartbeat = xTimerCreate("ptp_heartbeat", pdMS_TO_TICKS(PTP_HEARTBEAT_TICKRATE_MS), // timeout
                                      true,                                                      // timer operates in repeat mode
                                      NULL,                                                      // ID
                                      ptp_heartbeat_tmr_cb);                                     // callback-function
}

/**
 * Remove the heartbeat timer.
 */
static void ptp_remove_heartbeat_tmr() {
    xTimerStop(S.timers.heartbeat, 0);
    xTimerDelete(S.timers.heartbeat, 0);
}

static void ptp_start_heartbeat_tmr() {
    xTimerStart(S.timers.heartbeat, 0);
}

static void ptp_stop_heartbeat_tmr() {
    xTimerStop(S.timers.heartbeat, 0);
}

// --------------------------------------

static void ptp_common_init(const uint8_t *hwa) {
    // create clock identity
    ptp_create_clock_identity(hwa);

    // seed the randomizer
    srand(S.hwoptions.clockIdentity);

    // reset options
    nsToTsI(&S.hwoptions.offset, PTP_DEFAULT_SERVO_OFFSET);

    // initialize hardware
#ifdef PTP_ADDEND_INTERFACE
    PTP_HW_INIT(PTP_INCREMENT_NSEC, (uint32_t)PTP_ADDEND_INIT);
#elif defined(PTP_HLT_INTERFACE)
    PTP_HW_INIT();
#endif

    // initialize controller
    PTP_SERVO_INIT();

    // initialize the heartbeat timer
    ptp_create_heartbeat_tmr();
}

// initialize PTP module
void ptp_init(const uint8_t *hwa) {
    /* ---- COMMON ----- */
    ptp_common_init(hwa);

    /* ----- SBMC ------ */
    ptp_bmca_init();

    /* ----- SLAVE ----- */
    ptp_slave_init();

    /* ---- MASTER --- */
    ptp_master_init();

    // ---------------------

    ptp_reset(); // reset all PTP systems

#ifdef CLI_REG_CMD
    // register cli commands
    ptp_register_cli_commands();
#endif // CLI_REG_CMD

    // dispatch INIT_DONE event
    PTP_IUEV(PTP_UEV_INIT_DONE);
}

// deinit PTP module
void ptp_deinit() {
    /* ---- COMMON ----- */

    // remove the heartbeat timer
    ptp_remove_heartbeat_tmr();

#ifdef CLI_REG_CMD
    // remove cli commands
    ptp_remove_cli_commands();
#endif // CLI_REG_CMD

    // deinitialize controller
    PTP_SERVO_DEINIT();

    /* ----- SBMC ------ */
    ptp_bmca_destroy();

    /* ----- SLAVE ----- */
    ptp_slave_destroy();

    /* ---- MASTER --- */
    ptp_master_destroy();
}

// reset PTP subsystem
void ptp_reset() {
    // pause the heartbeat timer
    ptp_stop_heartbeat_tmr();

    /* ---- COMMON ---- */
    memset(&S.network, 0, sizeof(PtpNetworkState)); // network state

    // reinitialize the Network Stack Driver
    ptp_nsd_init(ptp_get_transport_type(), ptp_get_delay_mechanism());

    // reset statistics
    ptp_clear_stats();

    // reset common functionality
    ptp_common_reset();

    /* ---- SBMC ----- */
    ptp_bmca_reset();

    /* ---- SLAVE ---- */
    ptp_slave_reset();

    /* ---- MASTER --- */
    ptp_master_reset();

    // ------------------------

    // resume the heartbeat timer
    ptp_start_heartbeat_tmr();

    // dispatch RESET event
    PTP_IUEV(PTP_UEV_RESET_DONE);
}

// packet processing
void ptp_process_packet(RawPtpMessage *pRawMsg) {
    PtpHeader header;

    // header readout
    ptp_extract_header(&header, pRawMsg->data);

    // consider only messages in our domain
    if (header.domainNumber != S.profile.domainNumber ||
        header.transportSpecific != S.profile.transportSpecific) {
        return;
    }

    // process Announce messages and halt further processing
    PtpMessageType mt = header.messageType;
    if (mt == PTP_MT_Announce) {
        PtpMasterProperties newMstProp;
        ptp_extract_announce_message(&newMstProp, pRawMsg->data);
        ptp_handle_announce_msg(&newMstProp, &header);
        PTP_IUEV(PTP_UEV_ANNOUNCE_RECVED); // dispatch ANNOUNCE_RECVED event
        return;
    }

    // PDelay_Req messages should always be processed
    if ((header.messageType == PTP_MT_PDelay_Req) && (S.profile.delayMechanism == PTP_DM_P2P)) {
        PTP_IUEV(PTP_UEV_PDELAY_REQ_RECVED); // dispatch PDELAY_REQ_RECVED event
        ptp_send_pdelay_resp(pRawMsg);       // sent the PDelay_Resp message
        PTP_IUEV(PTP_UEV_PDELAY_RESP_SENT);  // dispatch PDELAY_RESP_SENT event
        return;
    }

    // if operating in slave mode
    PtpBmcaFsmState bmcaState = S.bmca.state;
    if (bmcaState == PTP_BMCA_SLAVE) {
        ptp_slave_process_message(pRawMsg, &header);
        return;
    } else if (bmcaState == PTP_BMCA_MASTER) { // if operating in Master mode
        ptp_master_process_message(pRawMsg, &header);
        return;
    }
}

void ptp_process_event(const PtpCoreEvent *event) {
    switch (event->code) {
    case PTP_CEV_HEARTBEAT: { // heartbeat event
        ptp_bmca_tick();
        ptp_slave_tick();
        ptp_master_tick();
    } break;
    case PTP_CEV_BMCA_STATE_CHANGED: {
        PtpBmcaFsmState bmcaState = event->w.w;
        if (bmcaState == PTP_BMCA_SLAVE) {
            ptp_slave_enable();
        } else if (bmcaState == PTP_BMCA_MASTER) {
            ptp_master_enable();
        } else {
            ptp_slave_disable();
            ptp_master_disable();
        }

        // dispatch BMCA_STATE_CHANGED event
        PTP_IUEV(PTP_UEV_BMCA_STATE_CHANGED);
    }
    default:
        break;
    }
}

// -----------------------------------------------

void ptp_set_sync_callback(PtpSyncCallback syncCb) {
    S.slave.syncCb = syncCb;
}

void ptp_set_user_event_callback(PtpUserEventCallback userEventCb) {
    S.userEventCb = userEventCb;
}
