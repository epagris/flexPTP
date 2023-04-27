#ifndef FLEXPTP_OPTIONS_STM32H743_H_
#define FLEXPTP_OPTIONS_STM32H743_H_

// -------------------------------------------
// --- DEFINES FOR PORTING IMPLEMENTATION ----
// -------------------------------------------

// Include LwIP headers here

//#include "lwip/netif.h"
//#include "lwip/tcpip.h"
//#include "lwip/udp.h"
//#include "lwip/igmp.h"

#include <arpa/inet.h>

#include <env/utils.h>

// Include hardware port files and fill the defines below to port the PTP stack to a physical hardware:
// - PTP_HW_INIT(increment, addend): function initializing timestamping hardware
// - PTP_MAIN_OSCILLATOR_FREQ_HZ: clock frequency fed into the timestamp unit [Hz]
// - PTP_INCREMENT_NSEC: hardware clock increment [ns]
// - PTP_UPDATE_CLOCK(s,ns): function jumping clock by defined value (negative time value means jumping backward)
// - PTP_SET_ADDEND(addend): function writing hardware clock addend register

#include <flexptp/hw_port/ptp_port_simulation.h>
#include <flexptp/hw_port/simsrc/FreeRTOS_simulation.h>
#include <flexptp/hw_port/simsrc/lwip_simulation.h>

#define PTP_MAIN_OSCILLATOR_FREQ_HZ (200000000)
#define PTP_INCREMENT_NSEC (5)

#define PTP_HW_INIT(increment, addend) ptphw_init(increment, addend)
#define PTP_UPDATE_CLOCK(s,ns) 	ptphw_update_clock(labs(s), abs(ns), (s * NANO_PREFIX + ns) < 0)
#define PTP_SET_ADDEND(addend) ptphw_set_addend(addend)
#define PTP_HW_GET_TIME(pt) ptphw_gettime(pt)

// Include the clock servo (controller) and define the following:
// - PTP_SERVO_INIT(): function initializing clock servo
// - PTP_SERVO_DEINIT(): function deinitializing clock servo
// - PTP_SERVO_RESET(): function reseting clock servo
// - PTP_SERVO_RUN(d): function running the servo, input: master-slave time difference (error), return: clock tuning value in PPB
//

#include <flexptp/servo/pd_controller.h>

#define PTP_SERVO_INIT() pd_ctrl_init()
#define PTP_SERVO_DEINIT() pd_ctrl_deinit()
#define PTP_SERVO_RESET() pd_ctrl_reset()
#define PTP_SERVO_RUN(d,pscd) pd_ctrl_run(d,pscd)

// Optionally add interactive, tokenizing CLI-support
// - CLI_REG_CMD(cmd_hintline,n_cmd,n_min_arg,cb): function for registering CLI-commands
//      cmd_hintline: text line printed in the help beginning with the actual command, separated from help text by \t charaters
//      n_cmd: number of tokens (words) the command consists of
//      n_arg: minimal number of arguments must be passed with the command
//      cb: callback function cb(const CliToken_Type *ppArgs, uint8_t argc)
//  return: cmd id (can be null, if discarded)

#include <cli/cli.h>

#define CLI_REG_CMD(cmd_hintline,n_cmd,n_min_arg,cb) cli_register_command(cmd_hintline, n_cmd, n_min_arg, cb)

// -------------------------------------------

#endif                          // FLEXPTP_OPTIONS_STM32H743_H_
