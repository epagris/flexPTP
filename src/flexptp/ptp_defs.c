#include "ptp_defs.h"

const uint8_t PTP_ETHERNET_PRIMARY[6] = { 0x01, 0x1B, 0x19, 0x00, 0x00, 0x00 };
const uint8_t PTP_ETHERNET_PEER_DELAY[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };

#if defined(LWIP) || defined(__linux__)
const ip_addr_t PTP_IGMP_PRIMARY = { 0x810100E0 }; // 224.0.1.129
const ip_addr_t PTP_IGMP_PEER_DELAY = { 0x6B0000E0 }; // 224.0.0.107
#elif defined(ETHLIB)
const ip_addr_t PTP_IGMP_PRIMARY = IPv4(224, 0, 1, 129); // 224.0.1.129
const ip_addr_t PTP_IGMP_PEER_DELAY = IPv4(224, 0, 0, 107); // 224.0.0.107
#elif defined(PTP_IGMP_PRIMARY_INIT) && defined(PTP_IGMP_PEER_DELAY_INIT)
// Network stack supplied by the port. The #error below already asks the integrator to
// "define PTP_IGMP_PRIMARY and PTP_IGMP_PEER_DELAY", but offered no mechanism to do so:
// both are const objects defined here, so a port could not provide them without editing
// this file. These two macros are that mechanism -- give them a brace initialiser matching
// whatever ip_addr_t the port's flexptp_options.h declares.
const ip_addr_t PTP_IGMP_PRIMARY = PTP_IGMP_PRIMARY_INIT;       // 224.0.1.129
const ip_addr_t PTP_IGMP_PEER_DELAY = PTP_IGMP_PEER_DELAY_INIT; // 224.0.0.107
#else // unkown network stack
#error "Hence the network stack is unknown, please define PTP_IGMP_PRIMARY_INIT and PTP_IGMP_PEER_DELAY_INIT!"
#endif
