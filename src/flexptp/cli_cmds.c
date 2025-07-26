#include "cli_cmds.h"

#include <string.h>

#include "clock_utils.h"
#include "logging.h"
#include "profiles.h"
#include "ptp_core.h"
#include "ptp_profile_presets.h"
#include "ptp_types.h"
#include "settings_interface.h"

#ifdef MIN
#undef MIN
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))


// #define S (gPtpCoreState)

// ---- COMPILE ONLY IF CLI_REG_CMD is provided ----

#ifdef CLI_REG_CMD

#ifndef CMD_FUNCTION
#error "No CMD_FUNCTION macro has been defined, cannot register CLI functions!"
#endif

static CMD_FUNCTION(CB_reset) {
    ptp_reset();
    MSG("> PTP reset!\n");
    return 0;
}

static CMD_FUNCTION(CB_offset) {
    if (argc > 0) {
        ptp_set_clock_offset(atoi(ppArgs[0]));
    }

    MSG("> PTP clock offset: %d ns\n", ptp_get_clock_offset());
    return 0;
}

static CMD_FUNCTION(CB_log) {
    bool logEn = false;

    if (argc > 1) {
        if (!strcmp(ppArgs[1], "on")) {
            logEn = true;
        } else if (!strcmp(ppArgs[1], "off")) {
            logEn = false;
        } else {
            return -1;
        }

        if (!strcmp(ppArgs[0], "def")) {
            ptp_log_enable(PTP_LOG_DEF, logEn);
        } else if (!strcmp(ppArgs[0], "corr")) {
            ptp_log_enable(PTP_LOG_CORR_FIELD, logEn);
        } else if (!strcmp(ppArgs[0], "ts")) {
            ptp_log_enable(PTP_LOG_TIMESTAMPS, logEn);
        } else if (!strcmp(ppArgs[0], "info")) {
            ptp_log_enable(PTP_LOG_INFO, logEn);
        } else if (!strcmp(ppArgs[0], "locked")) {
            ptp_log_enable(PTP_LOG_LOCKED_STATE, logEn);
        } else if (!strcmp(ppArgs[0], "bmca")) {
            ptp_log_enable(PTP_LOG_BMCA, logEn);
        } else {
            return -1;
        }

    } else {
        return -1;
    }

    return 0;
}

static CMD_FUNCTION(CB_time) {
    char datetime[24];
    TimestampU t;
    ptp_time(&t);

    if (argc > 0 && !strcmp(ppArgs[0], "ns")) {
        MSG("%lu %u\n", t.sec, t.nanosec);
    } else {
        tsPrint(datetime, (const TimestampI *)&t);
        MSG("%s\n", datetime);
    }

    return 0;
}

static CMD_FUNCTION(CB_master) {
    if (argc > 0) {
        if (!strcmp(ppArgs[0], "prefer")) {
            uint64_t idM = ptp_get_current_master_clock_identity();
            if (argc > 1) {
                idM = hextoclkid(ppArgs[1]);
            }
            ptp_prefer_master_clock(idM);
        } else if (!strcmp(ppArgs[0], "unprefer")) {
            ptp_unprefer_master_clock();
        }
    }

    MSG("Master clock ID: ");
    ptp_print_clock_identity(ptp_get_current_master_clock_identity());
    MSG("\n");
    return 0;
}

static CMD_FUNCTION(CB_ptpinfo) {
    MSG("Own clock ID: ");
    ptp_print_clock_identity(ptp_get_own_clock_identity());
    MSG("\nMaster clock ID: ");
    ptp_print_clock_identity(ptp_get_current_master_clock_identity());
    MSG("\n");
    return 0;
}

static CMD_FUNCTION(CB_ptpdomain) {
    if (argc > 0) {
        ptp_set_domain(atoi(ppArgs[0]));
    }
    MSG("PTP domain: %d\n", (int)ptp_get_domain());
    return 0;
}

#ifdef PTP_ADDEND_INTERFACE
static CMD_FUNCTION(CB_addend) {
    if (argc > 0) {
        ptp_set_addend((uint32_t)atoll(ppArgs[0]));
    }
    MSG("Addend: %u\n", ptp_get_addend());
    return 0;
}
#elif defined(PTP_HLT_INTERFACE)
static CMD_FUNCTION(CB_tuning) {
    if (argc > 0) {
        ptp_set_tuning((uint32_t)atof(ppArgs[0]));
    }
    MSG("Tuning: %.4f\n", ptp_get_tuning());
    return 0;
}
#endif

static CMD_FUNCTION(CB_transportType) {
    if (argc > 0) {
        if (!strcmp(ppArgs[0], "ipv4")) {
            ptp_set_transport_type(PTP_TP_IPv4);
        } else if (!strcmp(ppArgs[0], "802.3")) {
            ptp_set_transport_type(PTP_TP_802_3);
        }
    }

    MSG("Transport layer: %s\n", PTP_TRANSPORT_TYPE_HINT[ptp_get_transport_type()]);
    return 0;
}

static CMD_FUNCTION(CB_delayMechanism) {
    if (argc > 0) { // If changing delay mechanism, PTP will be inherently reset!
        if (!strcmp(ppArgs[0], "e2e")) {
            ptp_set_delay_mechanism(PTP_DM_E2E);
        } else if (!strcmp(ppArgs[0], "p2p")) {
            ptp_set_delay_mechanism(PTP_DM_P2P);
        }
    }

    MSG("Delay mechanism: %s\n", PTP_DELMECH_HINT[(int)ptp_get_delay_mechanism()]);
    return 0;
}

static CMD_FUNCTION(CB_transpec) {
    if (argc > 0) { // ...inherent reset!
        if (!strcmp(ppArgs[0], "def") || !strcmp(ppArgs[0], "unknown")) {
            ptp_set_transport_specific(PTP_TSPEC_UNKNOWN_DEF);
        } else if (!strcmp(ppArgs[0], "gPTP")) {
            ptp_set_transport_specific(PTP_TSPEC_GPTP_8021AS);
        }
    }

    MSG("PTP transport specific: %s\n", PTP_TRANSPEC_HINT[(int)ptp_get_transport_specific()]);
    return 0;
}

static CMD_FUNCTION(CB_profile) {
    bool printProfileSummary = true;
    if (argc > 0 && !strcmp(ppArgs[0], "preset")) {
        if (argc > 1) {
            const PtpProfile *pProf = ptp_profile_preset_get(ppArgs[1]);
            if (pProf) {
                MSG("Loading preset '%s'...\n\n", ppArgs[1]);
                ptp_load_profile(pProf); // ...inherent reset!
            } else {
                MSG("Profile preset '%s' is unknown, settings untouched!\n", ppArgs[1]);
            }
        } else {
            MSG("PTP profile presets: ");
            size_t N = ptp_profile_preset_cnt();
            size_t i;
            for (i = 0; i < N; i++) {
                MSG("%s", ptp_profile_preset_get_name(i));
                if ((i + 1) != N) {
                    MSG(", ");
                }
            }
            MSG("\n");
            printProfileSummary = false;
        }
    }

    if (printProfileSummary) {
        ptp_print_profile();
    }
    return 0;
}

static CMD_FUNCTION(CB_tlv) {
    bool printLoadedTlvPresetChain = true;
    if (argc > 0) {
        if (!strcmp(ppArgs[0], "preset")) {
            if (argc > 1) {
                const PtpProfileTlvElement *pTlv = ptp_tlv_chain_preset_get(ppArgs[1]);
                if (pTlv) {
                    MSG("Loading TLV-chain '%s'...\n\n", ppArgs[1]);
                    ptp_set_tlv_chain_by_name(ppArgs[1]); // ...inherent reset!
                } else {
                    MSG("TLV-chain preset '%s' is unknown, settings are not tampered with!\n", ppArgs[1]);
                }
                printLoadedTlvPresetChain = false;

            } else {
                MSG("TLV-chain presets: ");
                size_t N = ptp_tlv_chain_preset_cnt();
                size_t i;
                for (i = 0; i < N; i++) {
                    MSG("%s", ptp_tlv_chain_preset_get_name(i));
                    if ((i + 1) != N) {
                        MSG(", ");
                    }
                }
                MSG("\n");

                printLoadedTlvPresetChain = false;
            }
        } else if (!strcmp(ppArgs[0], "unload")) {
            MSG("Unloading any TLV-chain...\n\n");
            ptp_set_tlv_chain_by_name("");
            printLoadedTlvPresetChain = false;
        }
    }

    if (printLoadedTlvPresetChain) {
        MSG("Loaded TLV-chain: ");
        const char *tlvSet = ptp_get_loaded_tlv_chain();
        if (!strcmp(tlvSet, "")) {
            MSG("none\n\n");
        } else {
            MSG("'%s'\n\n", tlvSet);
        }
    }

    return 0;
}

static CMD_FUNCTION(CB_profile_flags) {
    if (argc > 0) { // set profile flags
        uint8_t pf = atoi(ppArgs[0]);
        ptp_set_profile_flags(pf);
    }

    MSG("Profile flags: %X\n", ptp_get_profile_flags());

    return 0;
}

static CMD_FUNCTION(CB_logPeriod) {
    if (argc > 1) {
        if (!strcmp(ppArgs[0], "delreq")) {
            if (!strcmp(ppArgs[1], "match")) {
                ptp_set_delay_req_log_period(PTP_LOGPER_SYNCMATCHED);
            } else {
                int8_t lp = atoi(ppArgs[1]);
                lp = MIN(MAX(PTP_LOGPER_MIN, lp), PTP_LOGPER_MAX);
                ptp_set_delay_req_log_period(lp);
            }
        } else if (!strcmp(ppArgs[0], "sync")) {
            int8_t lp = atoi(ppArgs[1]);
            lp = MIN(MAX(PTP_LOGPER_MIN, lp), PTP_LOGPER_MAX);
            ptp_set_sync_log_period(lp);
        } else if (!strcmp(ppArgs[0], "ann")) {
            int8_t lp = atoi(ppArgs[1]);
            lp = MIN(MAX(PTP_LOGPER_MIN, lp), PTP_LOGPER_MAX);
            ptp_set_announce_log_period(lp);
        }
    }

    MSG("(P)Delay_Req log. period: ");
    int8_t drlp = ptp_get_delay_req_log_period();
    if (drlp != PTP_LOGPER_SYNCMATCHED) {
        MSG("%d\n", drlp);
    } else {
        MSG("MATCHED\n");
    }

    MSG("Sync log. period: %d\n", ptp_get_sync_log_period());
    MSG("Announce log. period: %d\n", ptp_get_announce_log_period());

    return 0;
}

static CMD_FUNCTION(CB_coarseThreshold) {
    if (argc > 0) {
        ptp_set_coarse_threshold(atoi(ppArgs[0]));
    }

    MSG("PTP coarse correction threshold: %lu ns\n", ptp_get_coarse_threshold());
    return 0;
}

static CMD_FUNCTION(CB_priority) {
    if (argc >= 2) {
        ptp_set_priority1(atoi(ppArgs[0]));
        ptp_set_priority2(atoi(ppArgs[1]));
    }

    MSG("PTP priorities: priority1: %u, priority2: %u\n", ptp_get_priority1(), ptp_get_priority2());
    return 0;
}

// command assignments
enum PTP_CMD_IDS {
    CMD_RESET,
    CMD_OFFSET,
    CMD_LOG,
    CMD_TIME,
    CMD_MASTER,
    CMD_PTPINFO,
    CMD_PTPDOMAIN,
    CMD_ADDEND_TUNING,
    CMD_TRANSPORTTYPE,
    CMD_DELAYMECH,
    CMD_TRANSPEC,
    CMD_PROFILE,
    CMD_TLV,
    CMD_PROFILE_FLAGS,
    CMD_LOGPERIOD,
    CMD_COARSE_THRESHOLD,
    CMD_PRIORITY,
    CMD_N
};

// command descriptors
static int sCmds[CMD_N + 1];

#endif // CLI_REG_CMD

// register cli commands
void ptp_register_cli_commands() {
#ifdef CLI_REG_CMD
    sCmds[CMD_RESET] = CLI_REG_CMD("ptp reset \t\t\tReset PTP subsystem", 2, 0, CB_reset);
    sCmds[CMD_OFFSET] = CLI_REG_CMD("ptp servo offset [offset_ns] \t\t\tSet or query clock offset", 3, 0, CB_offset);
    sCmds[CMD_LOG] = CLI_REG_CMD("ptp log {def|corr|ts|info|locked|bmca} {on|off} \t\t\tTurn on or off logging", 2, 2, CB_log);
    sCmds[CMD_TIME] = CLI_REG_CMD("time [ns] \t\t\tPrint time", 1, 0, CB_time);
    sCmds[CMD_MASTER] = CLI_REG_CMD("ptp master [[un]prefer] [clockid] \t\t\tMaster clock settings", 2, 0, CB_master);
    sCmds[CMD_PTPINFO] = CLI_REG_CMD("ptp info \t\t\tPrint PTP info", 2, 0, CB_ptpinfo);
    sCmds[CMD_PTPDOMAIN] = CLI_REG_CMD("ptp domain [domain]\t\t\tPrint or set PTP domain", 2, 0, CB_ptpdomain);
#ifdef PTP_ADDEND_INTERFACE
    sCmds[CMD_ADDEND_TUNING] = CLI_REG_CMD("ptp addend [addend]\t\t\tPrint or set addend", 2, 0, CB_addend);
#elif defined(PTP_HLT_INTERFACE)
    sCmds[CMD_ADDEND_TUNING] = CLI_REG_CMD("ptp tuning [tuning]\t\t\tPrint or set tuning", 2, 0, CB_tuning);
#endif
    sCmds[CMD_TRANSPORTTYPE] = CLI_REG_CMD("ptp transport [{ipv4|802.3}]\t\t\tSet or get PTP transport layer", 2, 0, CB_transportType);
    sCmds[CMD_DELAYMECH] = CLI_REG_CMD("ptp delmech [{e2e|p2p}]\t\t\tSet or get PTP delay mechanism", 2, 0, CB_delayMechanism);
    sCmds[CMD_TRANSPEC] = CLI_REG_CMD("ptp transpec [{def|gPTP}]\t\t\tSet or get PTP transportSpecific field (majorSdoId)", 2, 0, CB_transpec);
    sCmds[CMD_PROFILE] = CLI_REG_CMD("ptp profile [preset [<name>]]\t\t\tPrint or set PTP profile, or list available presets", 2, 0, CB_profile);
    sCmds[CMD_TLV] = CLI_REG_CMD("ptp tlv [preset [name]|unload]\t\t\tPrint or set TLV-chain, or list available TLV presets", 2, 0, CB_tlv);
    sCmds[CMD_PROFILE_FLAGS] = CLI_REG_CMD("ptp pflags [<flags>]\t\t\tPrint or set profile flags", 2, 0, CB_profile_flags);
    sCmds[CMD_LOGPERIOD] = CLI_REG_CMD("ptp period <delreq|sync|ann> [<lp>|matched]\t\t\tPrint or set log. periods", 2, 0, CB_logPeriod);
    sCmds[CMD_COARSE_THRESHOLD] = CLI_REG_CMD("ptp coarse [threshold]\t\t\tPrint or set coarse correction threshold", 2, 0, CB_coarseThreshold);
    sCmds[CMD_PRIORITY] = CLI_REG_CMD("ptp priority [<p1> <p2>]\t\t\tPrint or set clock priority fields", 2, 0, CB_priority);
    sCmds[CMD_N] = -1;
#endif // CLI_REG_CMD
}

void ptp_remove_cli_commands() {
#ifdef CLI_REMOVE_CMD
    for (uint8_t i = 0; i < CMD_N; i++) {
        CLI_REMOVE_CMD(i);
    }
#endif // CLI_REMOVE_CMD
}
