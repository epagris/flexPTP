/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef TASK_PTP_H_
#define TASK_PTP_H_

#include <stdint.h>

void reg_task_ptp();            // register PTP task
void unreg_task_ptp();          // unregister PTP task
bool task_ptp_is_operating();   // does PTP task operate?
void ptp_enqueue_msg(void *pPayload, uint32_t len, uint32_t ts_sec, uint32_t ts_ns, int tp);    // put PTP-message on queue

#endif                          // TASK_PTP_H_
