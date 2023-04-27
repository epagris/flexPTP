#ifndef HW_PORT_PTP_PORT_SIMULATION_
#define HW_PORT_PTP_PORT_SIMULATION_

#include <flexptp/timeutils.h>

#ifdef __cplusplus
extern "C" {
#endif

    void ptphw_init(uint32_t increment, uint32_t addend);       // initialize PTP hardware
    void ptphw_gettime(TimestampU * pTime);     // get time
    void ptphw_set_addend(uint32_t addend);     // set addend
    void ptphw_update_clock(uint32_t s, uint32_t ns, int dir);  // update clock

#ifdef __cplusplus
}
#endif
#endif                          /* HW_PORT_PTP_PORT_SIMULATION_ */
