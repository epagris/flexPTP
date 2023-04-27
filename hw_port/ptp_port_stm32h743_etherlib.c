#include <flexptp/hw_port/flexptp_options_stm32h743_etherlib.h>
#include <flexptp/hw_port/ptp_port_stm32h743.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32h7xx_hal.h"
#include "eth_hw_drv.h"

#include "utils.h"
#include "cli.h"

extern ETH_HandleTypeDef EthHandle;

static unsigned sFreq = 1;

#ifdef CLI_REG_CMD

static int CB_pps(const CliToken_Type * ppArgs, uint8_t argc)
{
    if (argc >= 1) {
        float freq = atof(ppArgs[0]);

        int fc_exp;

        if (freq > 0) {
            fc_exp = round(log2f(freq)) + 1;

            // lower limit
            if (fc_exp < ETH_PTP_PPS_1Hz) {
                fc_exp = ETH_PTP_PPS_1Hz;
            }
            // upper limit
            if (fc_exp > ETH_PTP_PPS_16384Hz) {
                fc_exp = ETH_PTP_PPS_16384Hz;
            }

            sFreq = exp2(fc_exp - 1);
        } else {
            sFreq = 0;
            fc_exp = 0;
        }

        ETHHW_SetPTPPPSFreq(ETH, fc_exp);

//          // parse frequency [Hz] (integer only!)
//          uint32_t freq_Hz = atoi(ppArgs[0]);
//
//          if (freq_Hz == 0) {
//              // stop pulse train generation
//              ETH_StopPTPPPSPulseTrain(&EthHandle);
//          } else {
//            // compute period [ns]
//            uint32_t period_ns = NANO_PREFIX / freq_Hz;
//
//            // display warning if frequency is not integer divisor of 1E+09
//            if ((NANO_PREFIX % freq_Hz) != 0) {
//                MSG("Warning! PPS frequency is not totally accurate, "
//                    "choose frequency values corresponding to periods "
//                    "being integer divisors of 1E+09!\n");
//            }
//
//            // set duty cycle (try 50%) by specifying positive pulse length
//            uint32_t high_ns = period_ns / 2;
//
//            // start pulse train generation
//            ETH_StartPTPPPSPulseTrain(&EthHandle, high_ns, period_ns);
//          }
//
//          // store frequency setting
//          sFreq = freq_Hz;
    }

    if (sFreq > 0) {
        MSG("PPS frequency: %u Hz\n", sFreq);
    } else {
        MSG("PPS output is turned off.\n");
    }

    return 0;
}

static void ptphw_register_cli_commands()
{                               // TODO....
    cli_register_command("ptp pps {freq} \t\t\tSet or query PPS signal frequency [Hz]", 2, 0, CB_pps);
}

#endif                          // CLI_REG_CMD

void ptphw_init(uint32_t increment, uint32_t addend)
{
    MSG("Turning PTP on!");

    // enable PTP timestamping
    ETHHW_EnablePTPTimeStamping(ETH);

    vTaskDelay(pdMS_TO_TICKS(10));

    //ETH_EnablePTPTimeStamping(&EthHandle);

    // initialize PTP time
    ETHHW_InitPTPTime(ETH, 0, 0);

    // enable fine correction
    ETHHW_EnablePTPFineCorr(ETH, true);

    // set addend register value
    ETHHW_SetPTPAddend(ETH, addend);

    // set increment register value
    ETHHW_SetPTPSubsecondIncrement(ETH, increment);

    //ETH_StartPTPPPSPulseTrain(&EthHandle, 500E+06, 1E+09);
    ETHHW_SetPTPPPSFreq(ETH, ETH_PTP_PPS_1Hz);
    sFreq = 1;

    __HAL_RCC_GPIOG_CLK_ENABLE();

    // setup PPS-pin
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin = GPIO_PIN_8;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

#ifdef CLI_REG_CMD
    // register cli commands
    ptphw_register_cli_commands();
#endif                          // CLI_REG_CMD
}

void ptphw_gettime(TimestampU * pTime)
{
    pTime->sec = ETH->MACSTSR;
    pTime->nanosec = ETH->MACSTNR & ETH_MACSTNR_TSSS;
}
