/* The flexPTP project, (C) András Wiesner, 2024 */

#include <flexptp/ptp_defs.h>

#ifdef LWIP
const ip_addr_t PTP_IGMP_PRIMARY = { 0x810100E0 };      // 224.0.1.129

#elif defined(ETHLIB)
const ip_addr_t PTP_IGMP_PRIMARY = IPv4(224, 0, 1, 129);        // 224.0.1.129

#endif
