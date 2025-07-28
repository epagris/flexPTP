#include "debug_servo.h"
#include "cliutils/cli.h"
#include "flexptp/ptp_defs.h"
#include "standard_output/standard_output.h"

#include <flexptp_options.h>
#include <stdlib.h>
#include <string.h>

// --------------

static double skew0;        // clock skew offset (ppb)
static double skew_prev;    // clock skew in the previous cycle (ppb)
static int32_t dt0;         // time error offset (ns)
static int32_t dt_prev;     // time error in the previous cycle (ns)
static int32_t offset_prev; // time error offset in the previous cycle (ns)

static uint64_t cycle; // cycle counter

static double tuning_next_ppb;       // tuning effective in the next cycle (ppb)
static bool tuning_next_cycle_valid; // indicates that the above tuning will be implemented

// ---------------

#ifdef CLI_REG_CMD

#ifndef CMD_FUNCTION
#error "No CMD_FUNCTION macro has been defined, cannot register CLI functions!"
#endif

typedef enum {
    DS_CMDH_TUNE,
    DS_CMDH_SET_SKEW0,
    DS_CMDH_SET_DT0,
    DS_CMDH_N
} DebugServoCmdHandle;

static int cmd_handles[DS_CMDH_N];

static CMD_FUNCTION(tune) {
    tuning_next_ppb = atof(ppArgs[0]);
    tuning_next_cycle_valid = true;
    MSG("Tuning in next cycle: " PTP_COLOR_BGREEN "%.4f" PTP_COLOR_RESET " ppb\n", tuning_next_ppb);
    return 0;
}

static CMD_FUNCTION(skew_offset) {
    if (argc > 0) {
        if (!strcmp("last", ppArgs[0])) {
            skew0 = skew_prev;
        } else {
            skew0 = atof(ppArgs[0]);
        }
    }
    MSG("Skew offset: " PTP_COLOR_BGREEN "%.4f" PTP_COLOR_RESET "ppb\n", skew0);
    return 0;
}

static CMD_FUNCTION(time_offset) {
    if (argc > 0) {
        if (!strcmp("last", ppArgs[0])) {
            dt0 = dt_prev;
        } else {
            dt0 = atof(ppArgs[0]);
        }
    }
    MSG("Time offset: " PTP_COLOR_BYELLOW "%i" PTP_COLOR_RESET "ns", dt0);
    return 0;
}

static void register_cli_cmds() {
    cmd_handles[DS_CMDH_TUNE] = CLI_REG_CMD("ptp servo tune <tuning>\t\t\tSet relative tuning effective in next cycle", 3, 1, tune);
    cmd_handles[DS_CMDH_SET_SKEW0] = CLI_REG_CMD("ptp servo skew0 [skew|last]\t\t\tSet or get skew offset (ppb)", 3, 0, skew_offset);
    cmd_handles[DS_CMDH_SET_DT0] = CLI_REG_CMD("ptp servo dt0 [dt|last]\t\t\tSet or get time offset (ns)", 3, 0, time_offset);
}

#ifdef CLI_REMOVE_CMD
static void remove_cli_cmds() {
    for (uint8_t i = 0; i < DS_CMDH_N; i++) {
        CLI_REMOVE_CMD(i);
    }
}
#endif

#endif

void debug_servo_init() {
    debug_servo_reset();

#ifdef CLI_REG_CMD
    register_cli_cmds();
#endif
}

void debug_servo_deinit() {
#if defined(CLI_REG_CMD) && defined(CLI_REMOVE_CMD)
    remove_cli_cmds();
#endif
}

void debug_servo_reset() {
    skew0 = 0.0;
    dt0 = 0.0;
    dt_prev = 0.0;
    cycle = 0.0;
    tuning_next_ppb = 0.0;
}

float debug_servo_run(int32_t dt, PtpServoAuxInput *pAux) {
    double tuning_ppb = 0.0;

    // in the very first cycle skip running the filter
    if (cycle == 0) {
        goto retain_cycle_data;
    }

    // calculate input data
    double skew = ((double)(dt - dt_prev)) / ((double)pAux->measSyncPeriodNs) * 1E+09; // skew
    double skew_rel = skew - skew0;                                                    // substract skew offset
    int32_t offset = dt - dt0 - skew_rel * pAux->measSyncPeriodNs * 1E-09;             // time offset
    int32_t offset_delta = offset - offset_prev;

    MSG(PTP_COLOR_BGREEN "% 8.4f" PTP_COLOR_BYELLOW " % 12i" PTP_COLOR_BRED " % 12i" PTP_COLOR_RESET "\n", skew_rel, offset, offset_delta);

    offset_prev = offset;
    skew_prev = skew;

retain_cycle_data:
    dt_prev = dt;

    cycle++;

    if (tuning_next_cycle_valid) {
        tuning_ppb = tuning_next_ppb;
        tuning_next_cycle_valid = false;

        MSG("Now tuning " PTP_COLOR_BGREEN "%.4f" PTP_COLOR_RESET "ppb!\n", tuning_ppb);
    }

    return tuning_ppb;
}