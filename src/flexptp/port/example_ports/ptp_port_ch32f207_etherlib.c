#include "ptp_port_ch32f207_etherlib.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <cmsis_os2.h>

#include "ch32f20x_gpio.h"
#include "ch32f20x_rcc.h"
#include "flexptp_options.h"

void ptphw_init(uint32_t increment, uint32_t addend) {
    //MSG("Turning PTP on!");

    // enable PTP timestamping
    ETHHW_EnablePTPTimeStamping(ETH);

    osDelay(10);

    // initialize PTP time
    ETHHW_InitPTPTime(ETH, 0, 0);

    // enable fine correction
    ETHHW_EnablePTPFineCorr(ETH, true);

    // set addend register value
    ETHHW_SetPTPAddend(ETH, addend);

    // set increment register value
    ETHHW_SetPTPSubsecondIncrement(ETH, increment);

    // setup PPS-pin
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_5;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinRemapConfig(GPIO_Remap_PTP_PPS, ENABLE);
}

void ptphw_gettime(TimestampU *pTime) {
	uint32_t sec;
	uint32_t nsec;
    ETHHW_GetPTPTime(ETH, &sec, &nsec);
	pTime->sec = sec;
	pTime->nanosec = nsec;
}
