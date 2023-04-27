#ifndef FLEXPTP_SIM_FREERTOS_SIMULATION_H
#define FLEXPTP_SIM_FREERTOS_SIMULATION_H

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef timer_t *TimerHandle_t;
    typedef void (*tmrcb)(TimerHandle_t tmr);

    typedef uint32_t TickType_t;
    typedef uint32_t BaseType_t;
#define pdMS_TO_TICKS(x) (x)

    TimerHandle_t xTimerCreate(const char *name, TickType_t timeout, bool singleShot, void *timerID, tmrcb cb);
    BaseType_t xTimerDelete(TimerHandle_t xTimer, TickType_t xBlockTime);
    BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xBlockTime);
    BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xBlockTime);

//typedef Timer * TimerHandle_t;

//void FreeRTOS_simulation_init();

#ifdef __cplusplus
}
#endif
#endif                          //FLEXPTP_SIM_FREERTOS_SIMULATION_H
