/**
  ******************************************************************************
  * @file    ptp_raw_msg_circbuf.h
  * @copyright András Wiesner, 2019-\showdate "%Y"
  * @brief   This module implements a circular buffer that is used for accepting
  * and omitting received and to be transmissed packets in a controlled way. The
  * implementation does not rely on any OS features, will not block.
  ******************************************************************************
  */

#ifndef FLEXPTP_PTP_RAW_MSG_CIRCBUF_H_
#define FLEXPTP_PTP_RAW_MSG_CIRCBUF_H_

#include <stdbool.h>

#include "ptp_types.h"

/**
 * @brief "Ring" buffer for PTP-messages.
 */ 
typedef struct {
    RawPtpMessage * msgs; ///< messages
    uint8_t totalSize; ///< element count
    uint8_t lastReceived; ///< pointer to last received and last processed messages
    uint8_t freeBufs; ///< number of free buffers
    int allocPending; ///< allocation pending (by index)
} PtpCircBuf;

/**
 * Initialize circular buffer.
 * 
 * @param pCircBuf pointer to an empty (non-initialized) PtpCircBuf object
 * @param pMsgPool pointer to an allocated pool that supports holding at least n full PTP messages
 * @param n number of supported elements
 */
void ptp_circ_buf_init(PtpCircBuf * pCircBuf, RawPtpMessage * pMsgPool, uint8_t n); 

/**
 * Allocate an available buffer area from the circular buffer.
 * 
 * @param pCircBuf pointer to an existing PtpCircBuf object
 * 
 * @return pointer to the allocated area or NULL on failure
 */
RawPtpMessage * ptp_circ_buf_alloc(PtpCircBuf * pCircBuf);

/**
 * Commit last allocation, claim the first available area.
 * The first available area can be obtained through ptp_circ_buf_alloc().
 * 
 * @param pCircBuf pointer to an existing PtpCircBuf object
 */
int ptp_circ_buf_commit(PtpCircBuf *pCircBuf); // commit last allocation

/**
 * Release the oldest allocation.
 * 
 * @param pCircBuf pointer to an existing PtpCircBuf object
 */
void ptp_circ_buf_free(PtpCircBuf *pCircBuf);

/**
 * Peek an allocated area by index. Read the circular buffer like it was an array.
 * 
 * @param pCircBuf pointer to an existing PtpCircBuf object
 * @param idx index of the allocation
 * @return pointer to the area or NULL if is idx is out of bounds
 */
RawPtpMessage * ptp_circ_buf_get(PtpCircBuf *pCircBuf, uint8_t idx); 

#endif /* FLEXPTP_PTP_RAW_MSG_CIRCBUF_H_ */
