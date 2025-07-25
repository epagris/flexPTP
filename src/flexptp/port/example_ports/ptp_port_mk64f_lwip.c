#include "ptp_port_mk64f_lwip.h"

#include "eth_custom/ethernetif.h"
#include "fsl_enet.h"
#include "lwip/netif.h"

void ptphw_gettime(TimestampU * pTime) {
    enet_ptp_time_t time;
    ethernetif_ptp_get_time(netif_default, &time);

    pTime->sec = time.second;
    pTime->nanosec = time.nanosecond;
}

