/* The flexPTP project, (C) András Wiesner, 2024 */

#include "ptp_port_tiva_tm4c1294.h"

#include <stdint.h>
#include <stdbool.h>

#include "driverlib/emac.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"

void ptphw_init(uint32_t increment, uint32_t addend)
{
    // init clock
    EMACTimestampConfigSet(EMAC0_BASE, (EMAC_TS_ALL_RX_FRAMES | EMAC_TS_DIGITAL_ROLLOVER | EMAC_TS_PROCESS_IPV4_UDP | EMAC_TS_ALL | EMAC_TS_PTP_VERSION_2 | EMAC_TS_UPDATE_FINE),       // PTPv2 processing
                           increment);
    EMACTimestampAddendSet(EMAC0_BASE, addend);
    EMACTimestampEnable(EMAC0_BASE);

    // init PPS output
    GPIOPinTypePWM(GPIO_PORTG_AHB_BASE, GPIO_PIN_0);
    GPIOPinConfigure(GPIO_PG0_EN0PPS);
    EMACTimestampPPSSimpleModeSet(EMAC0_BASE, EMAC_PPS_1HZ);
}

void ptphw_gettime(TimestampU * pTime)
{
    pTime->sec = 0;
    pTime->nanosec = 0;
    EMACTimestampSysTimeGet(EMAC0_BASE, &(pTime->sec), &(pTime->nanosec));
}
