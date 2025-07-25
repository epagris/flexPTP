#ifndef FLEXPTP_OPTIONS_CH32F207_H_
#define FLEXPTP_OPTIONS_CH32F207_H_

#define ETHLIB
#define PTP_ADDEND_INTERFACE

// -------------------------------------------
// ------ DEFINES FOR FLEXPTP SETTINGS -------
// -------------------------------------------

#define ANNOUNCE_COLLECTION_WINDOW (2)

// -------------------------------------------
// --- DEFINES FOR PORTING IMPLEMENTATION ----
// -------------------------------------------

// Include LwIP/EtherLib headers here

#include <etherlib/etherlib.h>

// Give a printf-like printing implementation MSG(...)
// Give a maskable printing implementation CLILOG(en,...)
// Provide an SPRINTF-implementation SPRINTF(str,n,fmt,...)

#include "standard_output/standard_output.h"
#include "cliutils/cli.h"

// Include hardware port files and fill the defines below to port the PTP stack to a physical hardware:
// - PTP_HW_INIT(increment, addend): function initializing timestamping hardware
// - PTP_MAIN_OSCILLATOR_FREQ_HZ: clock frequency fed into the timestamp unit [Hz]
// - PTP_INCREMENT_NSEC: hardware clock increment [ns]
// - PTP_UPDATE_CLOCK(s,ns): function jumping clock by defined value (negative time value means jumping backward)
// - PTP_SET_ADDEND(addend): function writing hardware clock addend register

#include "EthDrv/mac_drv.h"

#define PTP_MAIN_OSCILLATOR_FREQ_HZ (144000000)
#define PTP_INCREMENT_NSEC (7)

#include <stdlib.h>

#define PTP_HW_INIT(increment, addend) ptphw_init(increment, addend)
#define PTP_SET_CLOCK(s,ns) ETHHW_InitPTPTime(ETH, labs(s), abs(ns))
#define PTP_SET_ADDEND(addend) ETHHW_SetPTPAddend(ETH, addend)
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
#define PTP_SERVO_RUN(d,pscd) pid_ctrl_run(d,pscd)

// Optionally add interactive, tokenizing CLI-support
// - CLI_REG_CMD(cmd_hintline,n_cmd,n_min_arg,cb): function for registering CLI-commands
//      cmd_hintline: text line printed in the help beginning with the actual command, separated from help text by \t charaters
//      n_cmd: number of tokens (words) the command consists of
//      n_arg: minimal number of arguments must be passed with the command
//      cb: callback function cb(const CliToken_Type *ppArgs, uint8_t argc)
//  return: cmd id (can be null, if discarded)

#define CLI_REG_CMD(cmd_hintline,n_cmd,n_min_arg,cb) CLI_REG_CMD(cmd_hintline, n_cmd, n_min_arg, cb)

// -------------------------------------------

#define CLILOG(en, ...) { if (en) MSG(__VA_ARGS__); }

// -------------------------------------------

//extern const void * config_get_flexPTP_section();
//#define PTP_CONFIG_PTR() config_get_flexPTP_section()


#endif /* FLEXPTP_OPTIONS_CH32F207_H_ */

