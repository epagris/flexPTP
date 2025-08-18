#ifndef FLEXPTP_PORT_PTP_PORT_MK64F_LWIP
#define FLEXPTP_PORT_PTP_PORT_MK64F_LWIP

#include "flexptp/timeutils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The hardware clock initialization is implemented as part of
 * the lwIP ethernetif using the standard ENET_* methods. */

/**
 * Get time right from the hardware clock.
 * 
 * @param pTime pointer to a TimestampU object.
 */
void ptphw_gettime(TimestampU * pTime);

#ifdef __cplusplus
}
#endif

#endif /* FLEXPTP_PORT_PTP_PORT_MK64F_LWIP */
