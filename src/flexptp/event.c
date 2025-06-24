#include "event.h"

#include "ptp_core.h"

///\cond 0
#define S (gPtpCoreState)
///\endcond

void ptp_invoke_user_event_cb(PtpUserEventCode uev) {
    if (S.userEventCb != NULL) {
        S.userEventCb(uev);
    }
}