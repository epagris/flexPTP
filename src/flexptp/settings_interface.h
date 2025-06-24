/**
  ******************************************************************************
  * @file    settings_interface.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module features functions to tweak around the PTP engine's
  * almost every property.
  ******************************************************************************
  */

#ifndef FLEXPTP_SETTINGS_INTERFACE_H_
#define FLEXPTP_SETTINGS_INTERFACE_H_

#include <stdint.h>

#include "ptp_types.h"

/**
 * Set clock offset in nanoseconds.
 * 
 * @param offset offset in nanoseconds
 */
void ptp_set_clock_offset(int32_t offset); // set clock offset in nanoseconds

/**
 * Get clock offset in nanoseconds.
 * 
 * @return clock offset in nanoseconds
 */
int32_t ptp_get_clock_offset();

/**
 * Make the PTP engine to expect messages from a particular master.
 * 
 * @param clockId clock ID of the master we want to follow
 */
void ptp_prefer_master_clock(uint64_t clockId);

/**
 * Allow the PTP engine to follow the best master on the network.
 */
void ptp_unprefer_master_clock(); // allow slave to synchronize to the BMCA-elected master

/**
 * Get clock ID of the master that the PTP engine currently follows.
 * 
 * @return clock ID of the followed master
 */
uint64_t ptp_get_current_master_clock_identity(); 

/**
 * Get our own clock identity.
 * 
 * @return our own clock identity
 */
uint64_t ptp_get_own_clock_identity();

/**
 * Set PTP domain which the PTP engine listens to.
 * 
 * @param domain PTP domain
 */
void ptp_set_domain(uint8_t domain);

/**
 * Get the PTP domain the engine operates in.
 * 
 * @return PTP domain
 */
uint8_t ptp_get_domain();

/**
 * Set addend, the frequency tuning word.
 * 
 * @param addend addend
 */
void ptp_set_addend(uint32_t addend);

/**
 * Get addend.
 * 
 * @return addend
 */
uint32_t ptp_get_addend();

/**
 * Get PTP transport type.
 * 
 * @return PTP transport type (L2/L4).
 */
PtpTransportType ptp_get_transport_type();

/**
 * Set PTP transport type.
 * 
 * @param tp transport type (L2/L4)
 */
void ptp_set_transport_type(PtpTransportType tp);

/**
 * Set the PTP delay mechanism (E2E/P2P).
 * 
 * @param dm delay mechanism
 */
void ptp_set_delay_mechanism(PtpDelayMechanism dm);

/**
 * Get the PTP delay mechanism.
 * 
 * @return delay mechanism (E2E/P2P)
 */
PtpDelayMechanism ptp_get_delay_mechanism();

/**
 * Set transport specific field.
 * 
 * @param tspec value of transport specific field
 */
void ptp_set_transport_specific(PtpTransportSpecific tspec);

/**
 * Get the transport specific field.
 * 
 * @return value of transport specific field
 */
PtpTransportSpecific ptp_get_transport_specific();

/**
 * Load TLV-chain preset fetched by name.
 * 
 * @param tlvSet name of the TLV-chain preset, pass an empty string ("") to unload everything
 */
void ptp_set_tlv_chain_by_name(const char * tlvSet);

/**
 * Get the name of the loaded TLV-chain preset.
 * 
 * @return name of the current TLV-chain preset OR an empty ("") string if nothing was loaded before
 */
const char * ptp_get_loaded_tlv_chain();

/**
 * Set profile flags.
 * 
 * @param flags combination of PtpProfileFlags
 */
void ptp_set_profile_flags(uint8_t flags);

/**
 * Get profile flags.
 * 
 * @return combination of PtpProfileFlags
 */
uint8_t ptp_get_profile_flags();

/**
 * Load profile preset.
 * 
 * @param pProfile pointer an existing profile preset.
 */
void ptp_load_profile(const PtpProfile * pProfile); // load profile preset

/**
 * Get logarithmic PTP message period designator code of Delay_Request queries.
 * 
 * @return logarithmic period code 
 */
int8_t ptp_get_delay_req_log_period();

/**
 * Set logarithmic PTP message period designator code of Delay_Requests.
 * 
 * @param drlp logarithmic period code
 */
void ptp_set_delay_req_log_period(int8_t drlp);

/**
 * Get the logarithmic Sync period code.
 * 
 * @return logarithmic period code
 */
int8_t ptp_get_sync_log_period();

/**
 * Set the logarithmic Sync period code.
 * 
 * @param slp logarithmic period code
 */
void ptp_set_sync_log_period(int8_t slp);

/**
 * Get the logarithmic Announce period code.
 * 
 * @return logarithmic period code
 */
int8_t ptp_get_announce_log_period();

/**
 * Set the logarithmic Announce period code.
 * 
 * @param alp logarithmic period code
 */
void ptp_set_announce_log_period(int8_t alp);

/**
 * Set PTP coarse correction time error tigger value.
 * 
 * @param ns coarse correction kick-in threshold in nanoseconds
 */
void ptp_set_coarse_threshold(uint64_t ns);

/**
 * Get PTP coarse correction time error tigger value.
 * 
 * @return coarse correction kick-in theshold in nanosconds
 */
uint64_t ptp_get_coarse_threshold(); 

/**
 * Set master dataset Priority1 field.
 */
void ptp_set_priority1(uint8_t p1);

/**
 * Get master dataset Priority1 field.
 * 
 * @return Priority1 field
 */
uint8_t ptp_get_priority1();

/**
 * Set master dataset Priority2 field.
 */
void ptp_set_priority2(uint8_t p2);

/**
 * Get master dataset Priority2 field.
 * 
 * @return Priority2 field
 */
uint8_t ptp_get_priority2();

/**
 * Set device clock class.
 */
void ptp_set_clock_class(PtpClockClass cc);

/**
 * Get device clock class.
 * 
 * @return device clock class
 */
PtpClockClass ptp_get_clock_class();

/**
 * Set clock accuracy.
 */
void ptp_set_clock_accuracy(PtpClockAccuracy ca);

/**
 * Get clock accuracy.
 * 
 * @return accuracy class of the clock
 */
PtpClockAccuracy ptp_get_clock_accuracy();

/**
 * Set clock variance.
 */
void ptp_set_clock_variance(uint16_t var);

/**
 * Get clock variance.
 * 
 * @return clock variance OR 0xFFFF indicating no calculation took place
 */
uint16_t ptp_get_clock_variance();

/**
 * Set number of local steps removed.
 */
void ptp_set_local_steps_removed(uint16_t lsr);

/**
 * Get number of local steps removed.
 * 
 * @return number of local steps removed
 */
uint16_t ptp_get_local_steps_removed();

/**
 * Get PTP time right from the hardware clock.
 * 
 * @param pT pointer to an existing TimestampU object, that is going to be overwritten with the current time
 */
void ptp_time(TimestampU * pT); 

#endif /* FLEXPTP_SETTINGS_INTERFACE_H_ */
