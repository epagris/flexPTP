#ifndef FLEXPTP_CRITICAL
#define FLEXPTP_CRITICAL

#include "ptp_types.h"

#ifdef FLEXPTP_FREERTOS
    #define FLEXPTP_ENTER_CRITICAL() taskENTER_CRITICAL();
#elif defined(FLEXPTP_CMSIS_OS2)
    #define FLEXPTP_ENTER_CRITICAL() __disable_interrupts();
#endif

#ifdef FLEXPTP_FREERTOS
    #define FLEXPTP_LEAVE_CRITICAL() taskEXIT_CRITICAL();
#elif defined(FLEXPTP_CMSIS_OS2)
    #define FLEXPTP_LEAVE_CRITICAL() __enable_interrupts();
#endif 

#endif /* FLEXPTP_CRITICAL */
