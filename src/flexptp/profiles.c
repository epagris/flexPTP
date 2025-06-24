#include "profiles.h"

#include "ptp_types.h"
#include "settings_interface.h"
#include <stdint.h>
#include <string.h>

char *PTP_TRANSPEC_HINT[] = { "unknown (default) [0]", "gPTP (802.1AS) [1]" }; ///< Hint on transport specific field
char *PTP_TRANSPORT_TYPE_HINT[] = { "IPv4", "802.3" }; ///< Hint on transport types
char *PTP_DELMECH_HINT[] = { "E2E", "P2P" }; ///< Hint on delay mechanism
char *PTP_FLAGS_HINT[] = { "Sync (and Follow_Up) messages will only be issued if the peer slave proves compliant", "This node is SLAVE-ONLY" }; ///< Hints for 

void ptp_print_profile() {
    MSG(PTP_COLOR_BGREEN "---- PTP PROFILE ----\n"
        PTP_COLOR_BYELLOW " transport specific: " PTP_COLOR_CYAN "%s\n"
        PTP_COLOR_BYELLOW " transport:          " PTP_COLOR_CYAN"%s\n"
        PTP_COLOR_BYELLOW " delay mechanism:    " PTP_COLOR_CYAN"%s\n"
        PTP_COLOR_BYELLOW " domain:             " PTP_COLOR_CYAN "%u\n",
        PTP_TRANSPEC_HINT[ptp_get_transport_specific()],
        PTP_TRANSPORT_TYPE_HINT[ptp_get_transport_type()],
        PTP_DELMECH_HINT[ptp_get_delay_mechanism()],
        ptp_get_domain());

    int8_t drlp = ptp_get_delay_req_log_period();
    if (drlp != PTP_LOGPER_SYNCMATCHED) {
        MSG(PTP_COLOR_BYELLOW " logDelayReqPeriod:  " PTP_COLOR_CYAN "%d\n", drlp);
    } else {
        MSG(PTP_COLOR_BYELLOW " logDelayReqPeriod:  " PTP_COLOR_CYAN "SYNC MATCHED\n");
    }

    MSG(PTP_COLOR_BYELLOW " logSyncPeriod:      " PTP_COLOR_CYAN "%d\n", ptp_get_sync_log_period());
    MSG(PTP_COLOR_BYELLOW " logAnnouncePeriod:  " PTP_COLOR_CYAN "%d\n", ptp_get_announce_log_period());

    const char * tlvSet = ptp_get_loaded_tlv_chain();
    if (!strcmp(tlvSet, "")) {
        tlvSet = "none";
    }
    MSG(PTP_COLOR_BYELLOW " TLV-chain:          " PTP_COLOR_CYAN "%s\n", tlvSet);

    uint8_t flags = ptp_get_profile_flags();
    MSG(PTP_COLOR_BYELLOW " flags:              " PTP_COLOR_CYAN "0x%X\n", flags);
    for (uint8_t i = 0; i < PTP_PF_N - 1; i++) {
        uint8_t mask = 1 << i;
        bool on = (flags & mask) != 0;
        const char * color = on ? (PTP_COLOR_BGREEN) : (PTP_COLOR_BRED);
        char bullet = on ? '+' : '-';
        MSG("   %s%c (0x%X) %s" PTP_COLOR_RESET "\n", color, bullet, mask,  PTP_FLAGS_HINT[i]);
    }
}