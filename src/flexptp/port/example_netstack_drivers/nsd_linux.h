#ifndef FLEXPTP_LINUX_NSD_LINUX_H
#define FLEXPTP_LINUX_NSD_LINUX_H

#include <stdint.h>

#include "../../timeutils.h"

/**
 * Preinitialize the Linux network stack driver.
 *
 * @param ifn name of the targeted network interface (e.g. eth0)
 *
 * @return initialization successful
 */
bool linux_nsd_preinit(const char * ifn);

/**
 * Clean up the Linux network stack driver.
 */
void linux_nsd_cleanup(void);

/**
 * Set PHC time.
 *
 * @param seconds seconds part
 * @param nanoseconds nanoseconds part
 */
void linux_set_time(uint32_t seconds, uint32_t nanoseconds);

/**
 * Adjust the PHC.
 *
 * @param tuning_ppb clock adjustment
 */
void linux_adjust_clock(double tuning_ppb);

/**
 * Query PHC time.
 *
 * @param pTime pointer to a timestamp object
 */
void linux_get_time(TimestampU * pTime);

#endif // FLEXPTP_LINUX_NSD_LINUX_H
