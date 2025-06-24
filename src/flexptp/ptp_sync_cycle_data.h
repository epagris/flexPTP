/**
 ******************************************************************************
 * @file    ptp_sync_cycle_data.h
 * @brief   This module defines the context object of a full synchronization
 * cycle. In SLAVE mode four timestamps are used when operating in E2E and six when in P2P
 * mode. Timestamps explained:
 *     + **T1**: Sync transmission by the master clock
 *     + **T2**: Sync reception by the slave clock
 *     + **T3**: (P)Delay_Req transmission time by slave clock
 *     + **T4**: (P)Delay_Req reception time by master clock
 *     + **T5**: (P)Delay_Resp transmission time by master clock
 *     + **T6**: (P)Delay_Resp reception time by slave clock
 * 
 * In MASTER P2P mode the fields take on the following meaning:
 *     + **T1**: PDelay_Req transmission time by the master clock
 *     + **T2**: PDelay_Req reception time by the slave clock
 *     + **T3**: PDelay_Resp transmission time by the slave clock
 *     + **T4**: PDelay_Resp reception time by the master clock
 * 
 * @copyright András Wiesner, 2019-\showdate "%Y"
 ******************************************************************************
 */

#ifndef FLEXPTP_PTP_SYNC_CYCLE_DATA_H_
#define FLEXPTP_PTP_SYNC_CYCLE_DATA_H_

#include "timeutils.h"

// timestamp indices
#define T1 (0)
#define T2 (1)
#define T3 (2)
#define T4 (3)
#define T5 (4)
#define T6 (5)

/**
 * @brief PTP synchronization cycle data.
 */
typedef struct {
    /* ---- SLAVE -----
     *
     * T1: Sync transmission time by master clock
     * T2: Sync reception time by slave clock
     * T3: (P)Delay_Req transmission time by slave clock
     * T4: (P)Delay_Req reception time by master clock
     * T5: (P)Delay_Resp transmission time by master clock
     * T6: (P)Delay_Resp reception time by slave clock
     *
     *   2 for M2S (Sync-FollowUp), 2 for S2M (DelReq-DelResp) if E2E OR
     *   4 for S2M (PDelReq-PDelResp-PDelResp_Follow_Up) if P2P
     *
     * ---- MASTER P2P ------
     * 
     * T1: PDelay_Req transmission time by the master clock
     * T2: PDelay_Req reception time by the slave clock
     * T3: PDelay_Resp transmission time by the slave clock
     * T4: PDelay_Resp reception time by the master clock
     */

    TimestampI t[6]; ///< T1-T6 timestamps
    uint64_t cf[6]; ///< T1-T6 correction fields
} PtpSyncCycleData;

#endif /* FLEXPTP_PTP_SYNC_CYCLE_DATA_H_ */
