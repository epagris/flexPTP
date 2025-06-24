#include "ptp_raw_msg_circbuf.h"

void ptp_circ_buf_init(PtpCircBuf *pCircBuf, RawPtpMessage *pMsgPool, uint8_t n) {
    pCircBuf->msgs = pMsgPool;
    pCircBuf->totalSize = n;
    pCircBuf->freeBufs = n;
    pCircBuf->lastReceived = 0;
    pCircBuf->allocPending = -1;
}

// allocate packet (CALL ONLY IF THERE IS SPACE AVAILABLE!)
RawPtpMessage *ptp_circ_buf_alloc(PtpCircBuf *pCircBuf) {
    if (pCircBuf->freeBufs > 0 && pCircBuf->allocPending == -1) {
        uint8_t current = (pCircBuf->lastReceived + 1) % pCircBuf->totalSize; // allocate a new packet
        pCircBuf->allocPending = current;
        return &(pCircBuf->msgs[current]);
    } else {
        return NULL;
    }
}

int ptp_circ_buf_commit(PtpCircBuf *pCircBuf) {
    if (pCircBuf->allocPending != -1) {
        pCircBuf->lastReceived = pCircBuf->allocPending; // advance last index
        pCircBuf->freeBufs--;                            // decrease amount of free buffers
        pCircBuf->allocPending = -1;                     // turn off allocation pending flag
        return pCircBuf->lastReceived;
    } else {
        return -1;
    }
}

void ptp_circ_buf_free(PtpCircBuf *pCircBuf) {
    if (pCircBuf->freeBufs < pCircBuf->totalSize) {
        pCircBuf->freeBufs++;
    }
}

RawPtpMessage *ptp_circ_buf_get(PtpCircBuf *pCircBuf, uint8_t idx) {
    if (idx < pCircBuf->totalSize) {
        return &(pCircBuf->msgs[idx]);
    } else {
        return NULL;
    }
}
