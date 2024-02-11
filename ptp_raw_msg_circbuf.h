/* The flexPTP project, (C) András Wiesner, 2024 */

#ifndef FLEXPTP_PTP_RAW_MSG_CIRCBUF_H_
#define FLEXPTP_PTP_RAW_MSG_CIRCBUF_H_

#include <flexptp/ptp_types.h>
#include <stdbool.h>

// "ring" buffer for PTP-messages
#define PTP_MSG_BUF_SIZE (32)
typedef struct {
    RawPtpMessage *msgs;        // messages
    uint8_t totalSize;          // total buffer size (element count)
    uint8_t lastReceived;       // pointer to last received and last processed messages
    uint8_t freeBufs;           // number of free buffers
    int allocPending;           // allocation pending (by index)
} PtpCircBuf;

void ptp_circ_buf_init(PtpCircBuf * pCircBuf, RawPtpMessage * pMsgPool, uint8_t n);     // initialize circular buffer
RawPtpMessage *ptp_circ_buf_alloc(PtpCircBuf * pCircBuf);       // allocate next available circular buffer
int ptp_circ_buf_commit(PtpCircBuf * pCircBuf); // commit last allocation
void ptp_circ_buf_free(PtpCircBuf * pCircBuf);  // free oldest allocation
RawPtpMessage *ptp_circ_buf_get(PtpCircBuf * pCircBuf, uint8_t idx);    // get message by index

#endif                          /* FLEXPTP_PTP_RAW_MSG_CIRCBUF_H_ */
