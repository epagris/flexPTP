/**
  ******************************************************************************
  * @file    cli_cmds.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module handles and registers CLI commands.
  *          Commands:
  @verbatim
  ptp servo params [Kp Kd]                           Set or query K_p and K_d servo parameters
  ptp servo log internals {on|off}                   Enable or disable logging of servo internals
  ptp reset                                          Reset PTP subsystem
  ptp servo offset [offset_ns]                       Set or query clock offset
  ptp log {def|corr|ts|info|locked|bmca} {on|off}    Turn on or off logging
  time [ns]                                          Print time
  ptp master [[un]prefer] [clockid]                  Master clock settings
  ptp info                                           Print PTP info
  ptp domain [domain]                                Print or get PTP domain
  ptp addend [addend]                                Print or set addend
  ptp transport [{ipv4|802.3}]                       Set or get PTP transport layer
  ptp delmech [{e2e|p2p}]                            Set or get PTP delay mechanism
  ptp transpec [{def|gPTP}]                          Set or get PTP transportSpecific field (majorSdoId)
  ptp profile [preset [<name>]]                      Print or set PTP profile, or list available presets
  ptp tlv [preset [name]|unload]                     Print or set TLV-chain, or list available TLV presets
  ptp pflags [<flags>]                               Print or set profile flags
  ptp period <delreq|sync|ann> [<lp>|matched]        Print or set log. periods
  ptp coarse [threshold]                             Print or set coarse correction threshold
  ptp priority [<p1> <p2>]                           Print or set clock priority fields
  @endverbatim
  ******************************************************************************
  */

#ifndef FLEXPTP_CLI_CMDS_H_
#define FLEXPTP_CLI_CMDS_H_

/**
 * Register flexPTP CLI commands. CLI_REG_CMD must be defined and operational to make this work.
 */
void ptp_register_cli_commands();

/**
 * Remove flexPTP CLI commands.
 *
 */
void ptp_remove_cli_commands();


#endif /* FLEXPTP_CLI_CMDS_H_ */
