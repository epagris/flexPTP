#ifndef FLEXPTP_SETTINGS_INTERFACE_H_
#define FLEXPTP_SETTINGS_INTERFACE_H_

#include <flexptp/ptp_types.h>
#include <stdint.h>

void ptp_set_clock_offset(int32_t offset);      // set clock offset in nanoseconds
int32_t ptp_get_clock_offset(); // get clock offset in nanoseconds
void ptp_prefer_master_clock(uint64_t clockId); // lock slave to a particular master
void ptp_unprefer_master_clock();       // allow slave to synchronize to the BMCA-elected master
uint64_t ptp_get_current_master_clock_identity();       // get current master clock ID
uint64_t ptp_get_own_clock_identity();  // get out clock identity
void ptp_set_domain(uint8_t domain);    // set PTP domain
uint8_t ptp_get_domain();       // get PTP domain
void ptp_set_addend(uint32_t addend);   // set hardware clock addend (frequency tuning!)
uint32_t ptp_get_addend();      // get hardware clock addend
PtpTransportType ptp_get_transport_type();      // get PTP transport type

void ptp_time(TimestampU * pT); // get time

#endif                          /* FLEXPTP_SETTINGS_INTERFACE_H_ */
