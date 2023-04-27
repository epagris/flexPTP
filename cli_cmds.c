#include "ptp_core.h"
#include <ctype.h>
#include <flexptp/cli_cmds.h>
#include <flexptp/clock_utils.h>
#include <flexptp/format_utils.h>
#include <flexptp/logging.h>
#include <flexptp/msg_utils.h>
#include <flexptp/ptp_core.h>
#include <flexptp/ptp_profile_presets.h>
#include <flexptp/settings_interface.h>

//#define S (gPtpCoreState)

// ---- COMPILE ONLY IF CLI_REG_CMD is provided ----

#ifdef CLI_REG_CMD

static int CB_reset(const CliToken_Type * ppArgs, uint8_t argc)
{
    ptp_reset();
    MSG("> PTP reset!\n");
    return 0;
}

static int CB_offset(const CliToken_Type * ppArgs, uint8_t argc)
{
    if (argc > 0) {
        ptp_set_clock_offset(atoi(ppArgs[0]));
    }

    MSG("> PTP clock offset: %d ns\n", ptp_get_clock_offset());
    return 0;
}

static int CB_log(const CliToken_Type * ppArgs, uint8_t argc)
{
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
            ptp_enable_logging(PTP_LOG_DEF, logEn);
        } else if (!strcmp(ppArgs[0], "corr")) {
            ptp_enable_logging(PTP_LOG_CORR_FIELD, logEn);
        } else if (!strcmp(ppArgs[0], "ts")) {
            ptp_enable_logging(PTP_LOG_TIMESTAMPS, logEn);
        } else if (!strcmp(ppArgs[0], "info")) {
            ptp_enable_logging(PTP_LOG_INFO, logEn);
        } else if (!strcmp(ppArgs[0], "locked")) {
            ptp_enable_logging(PTP_LOG_LOCKED_STATE, logEn);
        } else {
            return -1;
        }

    } else {
        return -1;
    }

    return 0;
}

static int CB_time(const CliToken_Type * ppArgs, uint8_t argc)
{
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

static int CB_master(const CliToken_Type * ppArgs, uint8_t argc)
{
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

static int CB_ptpinfo(const CliToken_Type * ppArgs, uint8_t argc)
{
    MSG("Own clock ID: ");
    ptp_print_clock_identity(ptp_get_own_clock_identity());
    MSG("\nMaster clock ID: ");
    ptp_print_clock_identity(ptp_get_current_master_clock_identity());
    MSG("\n");
    return 0;
}

static int CB_ptpdomain(const CliToken_Type * ppArgs, uint8_t argc)
{
    if (argc > 0) {
        ptp_set_domain(atoi(ppArgs[0]));
    }
    MSG("PTP domain: %d\n", (int)ptp_get_domain());
    return 0;
}

static int CB_addend(const CliToken_Type * ppArgs, uint8_t argc)
{
    if (argc > 0) {
        ptp_set_addend((uint32_t) atol(ppArgs[0]));
    }
    MSG("Addend: %u\n", ptp_get_addend());
    return 0;
}

static char *sTransportTypeHint[] = { "IPv4", "802.3" };

// command assignments
enum PTP_CMD_IDS {
    CMD_RESET, CMD_OFFSET, CMD_LOG, CMD_TIME, CMD_MASTER, CMD_PTPINFO, CMD_PTPDOMAIN, CMD_ADDEND,

    CMD_N
};

// command descriptors
static int sCmds[CMD_N + 1];

#endif                          // CLI_REG_CMD

// register cli commands
void ptp_register_cli_commands()
{
#ifdef CLI_REG_CMD
    sCmds[CMD_RESET] = cli_register_command("ptp reset \t\t\tReset PTP subsystem", 2, 0, CB_reset);
    sCmds[CMD_OFFSET] = cli_register_command("ptp servo offset [offset_ns] \t\t\tSet or query clock offset", 3, 0, CB_offset);
    sCmds[CMD_LOG] = cli_register_command("ptp log {def|corr|ts|info|locked} {on|off} \t\t\tTurn on or off logging", 2, 2, CB_log);
    sCmds[CMD_TIME] = cli_register_command("time [ns] \t\t\tPrint time", 1, 0, CB_time);
    sCmds[CMD_MASTER] = cli_register_command("ptp master [[un]prefer] [clockid] \t\t\tMaster clock settings", 2, 0, CB_master);
    sCmds[CMD_PTPINFO] = cli_register_command("ptp info \t\t\tPrint PTP info", 2, 0, CB_ptpinfo);
    sCmds[CMD_PTPDOMAIN] = cli_register_command("ptp domain [domain]\t\t\tPrint or get PTP domain", 2, 0, CB_ptpdomain);
    sCmds[CMD_ADDEND] = cli_register_command("ptp addend [addend]\t\t\tPrint or set addend", 2, 0, CB_addend);

    sCmds[CMD_N] = -1;
#endif                          // CLI_REG_CMD
}

void ptp_remove_cli_commands()
{
#ifdef CLI_REG_CMD
    cli_remove_command_array(sCmds);
#endif                          // CLI_REG_CMD
}
