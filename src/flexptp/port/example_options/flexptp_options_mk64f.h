#ifndef FLEXPTP_OPTIONS_MK64F_H_
#define FLEXPTP_OPTIONS_MK64F_H_

#define LWIP
#define PTP_HLT_INTERFACE

#define FLEXPTP_TASK_PRIORITY 24

// -------------------------------------------
// --- DEFINES FOR PORTING IMPLEMENTATION ----
// -------------------------------------------

// Include LwIP/EtherLib headers here

#include <lwip/ip_addr.h>
#include <lwip/netif.h>

// Give a printf-like printing implementation MSG(...)
// Give a maskable printing implementation CLILOG(en,...)
// Provide an SPRINTF-implementation SPRINTF(str,n,fmt,...)

#include "embfmt/embformat.h"
#include "standard_output/standard_output.h"

#define FLEXPTP_SNPRINTF(...) embfmt(__VA_ARGS__)

// Include hardware port files and fill the defines below to port the PTP stack to a physical hardware:

#include <stdlib.h>

#include <eth_custom/ethernetif.h>
#include "flexPTP_port/ptp_port_mk64f_lwip.h"

#define PTP_CONFIG_LEADING_PARAMS netif_default

#define PTP_HW_INIT() ethernetif_ptp_init(PTP_CONFIG_LEADING_PARAMS)
#define PTP_SET_CLOCK(s, ns) ethernetif_ptp_set_clock(PTP_CONFIG_LEADING_PARAMS, labs(s), abs(ns))
#define PTP_SET_TUNING(tuning) ethernetif_ptp_set_tuning(PTP_CONFIG_LEADING_PARAMS, tuning)
#define PTP_HW_GET_TIME(pt) ptphw_gettime(pt)

// Include the clock servo (controller) and define the following:
// - PTP_SERVO_INIT(): function initializing clock servo
// - PTP_SERVO_DEINIT(): function deinitializing clock servo
// - PTP_SERVO_RESET(): function reseting clock servo
// - PTP_SERVO_RUN(d): function running the servo, input: master-slave time difference (error), return: clock tuning value in PPB
//

#include <flexptp/servo/pid_controller.h>

#define PTP_SERVO_INIT() pid_ctrl_init()
#define PTP_SERVO_DEINIT() pid_ctrl_deinit()
#define PTP_SERVO_RESET() pid_ctrl_reset()
#define PTP_SERVO_RUN(d, pscd) pid_ctrl_run(d, pscd)

// Optionally add interactive, tokenizing CLI-support
// - CLI_REG_CMD(cmd_hintline,n_cmd,n_min_arg,cb): function for registering CLI-commands
//      cmd_hintline: text line printed in the help beginning with the actual command, separated from help text by \t charaters
//      n_cmd: number of tokens (words) the command consists of
//      n_arg: minimal number of arguments must be passed with the command
//      cb: callback function cb(const CliToken_Type *ppArgs, uint8_t argc)
//  return: cmd id (can be null, if discarded)

#include "cli.h"
#define CLI_REG_CMD(cmd_hintline, n_cmd, n_min_arg, cb) cli_register_command(cmd_hintline, n_cmd, n_min_arg, cb)
//#define CLI_REG_CMD(cmd_hintline, n_cmd, n_min_arg, cb)

// -------------------------------------------

#define CLILOG(en, ...)       \
    {                         \
        if (en)               \
            MSG(__VA_ARGS__); \
    }

// -------------------------------------------

#define PTP_ENABLE_MASTER_OPERATION (1) // Enable or disable master operations

// Static fields of the Announce message
#define PTP_CLOCK_PRIORITY1 (128)                      // priority1 (0-255)
#define PTP_CLOCK_PRIORITY2 (128)                      // priority2 (0-255)
#define PTP_BEST_CLOCK_CLASS (PTP_CC_DEFAULT)          // best clock class of this device
#define PTP_WORST_ACCURACY (PTP_CA_UNKNOWN)            // worst accucary of the clock
#define PTP_TIME_SOURCE (PTP_TSRC_INTERNAL_OSCILLATOR) // source of the distributed time

// -------------------------------------------

// #include "flexptp/event.h"
// extern void flexptp_user_event_cb(PtpUserEventCode uev);
// #define PTP_USER_EVENT_CALLBACK flexptp_user_event_cb

#endif /* MK64F_H_ */
