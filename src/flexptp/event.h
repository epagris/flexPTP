/**
  ******************************************************************************
  * @file    event.h
  * @copyright András Wiesner, 2025-\showdate "%Y"
  * @brief   In this module are the core and user events defined.
  ******************************************************************************
  */

#ifndef FLEXPTP_EVENT
#define FLEXPTP_EVENT

// ----------- CORE EVENTS -----------

typedef enum {
    PTP_CEV_HEARTBEAT = 0x00,
    PTP_CEV_BMCA_STATE_CHANGED,
} PtpCoreEventCode;

#include <stdint.h>
typedef struct {
    uint16_t code; ///< Event code
    union {
        uint16_t w;
        uint8_t b[2];
    } w; /// 2-byte data
    union {
        uint32_t dw;
        uint16_t w[2];
        uint8_t b[4];
    } dw; ///< 4-byte data
} PtpCoreEvent;

// ------------ USER EVENTS -------------

/**
 * PTP user event codes.
 * 
 * Commented-out ones are not yet implemented.
 */
typedef enum {
    PTP_UEV_INIT_DONE = 0x01, ///< The flexPTP core has been initialized
    PTP_UEV_RESET_DONE,       ///< The flexPTP module has been reset

    PTP_UEV_SYNC_RECVED,                  ///< A Sync message has been received (slave)
    PTP_UEV_SYNC_SENT,                    ///< A Sync message has eebn sent (master)
    PTP_UEV_FOLLOW_UP_RECVED,             ///< A Follow_Up message has been received (slave)
    //PTP_UEV_FOLLOW_UP_SENT,               ///< A Follow_Up message has been sent (master) 
    PTP_UEV_DELAY_REQ_RECVED,             ///< A Delay_Req had been received (master)
    PTP_UEV_DELAY_REQ_SENT,               ///< A Delay_Req had been sent (slave)
    PTP_UEV_DELAY_RESP_RECVED,            ///< A Delay_Resp had been received (slave)
    PTP_UEV_DELAY_RESP_SENT,              ///< A Delay_Resp had been sent (master)
    PTP_UEV_PDELAY_REQ_RECVED,            ///< A PDelay_Req had been received (master/slave)
    PTP_UEV_PDELAY_REQ_SENT,              ///< A PDelay_Req had been sent (master/slave)
    PTP_UEV_PDELAY_RESP_RECVED,           ///< A PDelay_Resp had been received (master/slave)
    PTP_UEV_PDELAY_RESP_SENT,             ///< A PDelay_Resp had been sent (master/slave)
    PTP_UEV_PDELAY_RESP_FOLLOW_UP_RECVED, ///< A PDelay_Resp_Follow_Up had been received (master/slave)
    //PTP_UEV_PDELAY_RESP_FOLLOW_UP_SENT,   ///< A PDelay_Resp_Follow_Up had been sent (master/slave)
    PTP_UEV_ANNOUNCE_SENT,                ///< An Announce message has been sent (master)
    PTP_UEV_ANNOUNCE_RECVED,              ///< An Announce message has been received (master/slave)

    PTP_UEV_LOCKED,   ///< The average clock accuracy is sufficient
    PTP_UEV_UNLOCKED, ///< Our clock has deviated from the master in average

    PTP_UEV_BMCA_STATE_CHANGED, ///< The BMCA state has changed

    PTP_UEV_NETWORK_ERROR, ///< Indication of lost messages or the absence of expected responses
    PTP_UEV_QUEUE_ERROR,   ///< This event signals that the flexPTP's internal transmission output queue is full and blocked
} PtpUserEventCode;

/**
 * Invoke the user event callback.
 * 
 * @param uev user event code
 */
void ptp_invoke_user_event_cb(PtpUserEventCode uev);

#define PTP_IUEV(uev) ptp_invoke_user_event_cb(uev);

#endif /* FLEXPTP_EVENT */
