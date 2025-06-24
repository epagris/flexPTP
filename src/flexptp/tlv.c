#include "tlv.h"
#include "flexptp/ptp_types.h"
#include <string.h>

uint16_t ptp_tlv_insert(void * dst, const PtpProfileTlvElement * pad, PtpMessageType mt, uint16_t maxLen) {
    const PtpProfileTlvElement * iter = pad;
    uint8_t * p = (uint8_t *)dst;
    uint16_t lenLeft = maxLen;
    uint16_t size = 0;

    while (iter != NULL) {
        if (iter->msgType == mt) {
            if (iter->size > lenLeft) {
                break;
            }
            memcpy(p, iter->data, iter->size);
            p += iter->size;
            lenLeft -= iter->size;
            size += iter->size;
        }
        iter = iter->next;
    }

    return size;
}