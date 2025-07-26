/* (C) András Wiesner, 2025 */

#include "spid_controller.h"
#include "standard_output/term_colors.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <flexptp_options.h>

// ----------------------------------

#define LIMIT(x, u, l) (((x) < (l)) ? (l) : (((x) > (u)) ? (u) : (x)))
#define SQR(x) ((x) * (x))

// ----------------------------------

static bool logInternals = false; ///< Decides if servo's internal operation shoud be reported or not
static bool firstRun = true;      ///< Indicates if first run did not occur yet

// ----------------------------------

static int32_t timeErrors[SPID_FILTER_LEN] = {}; ///< List of recent time errors
static int64_t accumulatedError = 0;             ///< Sum of all elements in the timeErrors array
static int64_t accumulatedSquareError = 0;       ///< Sum of the square of all elements
static uint32_t cycleIndex = 0;                  ///< Index of the current cycle
static double meanTimeError = 0.0;               ///< Mean time error
static double meanSquareTimeError = 0.0;         ///< Mean square time error
static double timeErrorVariance = 0.0;           ///< Variance of the time error
static double timeErrorDeviation = 0.0;          ///< Deviation of the time error
static bool filterEffective = false;             ///< Indicates whether the filter output is valid

// ----------------------------------

// PID servo coefficients
static float P_FACTOR = 0.5 * 0.476; ///< Proportional factor
static float I_FACTOR = 0;           ///< Integrating factor
static float D_FACTOR = 3.0;         ///< Differentiating factor

// ----------------------------------

static double rd_prev_ppb;      ///< relative frequency error measured in previous iteration
static double integrator_value; ///< value stored in the integrator

// ----------------------------------

#ifdef CLI_REG_CMD

static int CB_params(const CliToken_Type *ppArgs, uint8_t argc) {
    // set if parameters passed after command
    if (argc >= 3) {
        P_FACTOR = atof(ppArgs[0]);
        I_FACTOR = atof(ppArgs[1]);
        D_FACTOR = atof(ppArgs[2]);
    }

    MSG("> PTP params: K_p = %.3f, K_i = %.3f, K_d = %.3f\n", P_FACTOR, I_FACTOR, D_FACTOR);

    return 0;
}

static int CB_logInternals(const CliToken_Type *ppArgs, uint8_t argc) {
    if (argc >= 1) {
        int en = ONOFF(ppArgs[0]);
        if (en >= 0) {
            if (en && !logInternals) {
                MSG("\nSyncIntv. [ns] | dt [ns] | gamma [ppb]\n\n");
            }
            logInternals = en;
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

static struct {
    int params;
    int internals;
} sCliCmdIdx = {0};

static void spid_ctrl_register_cli_commands() {
    sCliCmdIdx.params = CLI_REG_CMD("ptp servo params [Kp Kd] \t\t\tSet or query K_p and K_d servo parameters", 3, 0, CB_params);
    sCliCmdIdx.internals = CLI_REG_CMD("ptp servo log internals {on|off} \t\t\tEnable or disable logging of servo internals", 4, 1, CB_logInternals);
}

static void spid_ctrl_remove_cli_commands() {
    cli_remove_command(sCliCmdIdx.params);
    cli_remove_command(sCliCmdIdx.internals);
}

#endif // CLI_REG_CMD

void spid_ctrl_init() {
    spid_ctrl_reset();

#ifdef CLI_REG_CMD
    spid_ctrl_register_cli_commands();
#endif // CLI_REG_CMD
}

void spid_ctrl_deinit() {
#ifdef CLI_REG_CMD
    spid_ctrl_remove_cli_commands();
#endif // CLI_REG_CMD
}

void spid_ctrl_reset() {
    firstRun = true;
    integrator_value = 0;

    memset(timeErrors, 0, sizeof(int32_t) * SPID_FILTER_LEN);
    filterEffective = false;
    accumulatedError = 0;
    accumulatedSquareError = 0;
    timeErrorVariance = 0;
    meanTimeError = 0.0;
    meanSquareTimeError = 0.0;
    timeErrorDeviation = 0.0;
    cycleIndex = 0;
}

float spid_ctrl_run(int32_t dt, PtpServoAuxInput *pAux) {
    if (firstRun) {
        firstRun = false;
        rd_prev_ppb = dt;
        integrator_value = 0;
        return 0;
    }

    bool filterEffective_prev = filterEffective;

    // feed the filter
    uint32_t idx = cycleIndex % SPID_FILTER_LEN;
    accumulatedError -= timeErrors[idx];
    accumulatedError += dt;
    accumulatedSquareError -= SQR(timeErrors[idx]);
    accumulatedSquareError += SQR(dt);

    timeErrors[idx] = dt;

    meanTimeError = ((double)accumulatedError) / ((double)(SPID_FILTER_LEN));
    meanSquareTimeError = ((double)accumulatedSquareError) / ((double)(SPID_FILTER_LEN));

    timeErrorVariance = meanSquareTimeError - SQR(meanTimeError);
    timeErrorDeviation = sqrt(timeErrorVariance);
    cycleIndex++;

    // maintain filter validity signal
    filterEffective = filterEffective || (cycleIndex > 2 * SPID_FILTER_LEN);

    // replace time error with the filtered one if the filter is effective
    if (filterEffective) {
        MSG(ANSI_COLOR_BGREEN "%f %f\n" ANSI_COLOR_RESET, meanTimeError,timeErrorDeviation);
    }

    if (filterEffective && !filterEffective_prev) {
        MSG("SPID smoothing is effective!\n");
    }

    // ------------------

    double C = 1.0;
    if (filterEffective && (timeErrorDeviation < 60)) {
        C = timeErrorDeviation / 60.0;
    }

    // ------------------

    // calculate relative frequency error
    double rd_ppb = ((double)dt) / (pAux->measSyncPeriodNs + dt) * 1E+09;

    // calculate difference
    double rd_D_ppb = C * D_FACTOR * (rd_ppb - rd_prev_ppb);

    // calculate output (run the PD controller)
    double corr_ppb = -(P_FACTOR * (rd_ppb + rd_D_ppb) + integrator_value) * exp((pAux->measSyncPeriodNs * 1E-09) - 1.0);

    // update integrator
    integrator_value += I_FACTOR * rd_ppb;

    // store error value (time difference) for use in next iteration
    rd_prev_ppb = rd_ppb;

    CLILOG(logInternals, "%d %f\n", dt, rd_ppb);

    // --------------------------
    if (filterEffective && (meanTimeError < SPID_SMOOTHENED_CTRL_LIMIT_NS)) {
        double corr_limit_ppb = timeErrorDeviation * SPID_LIMIT_RANGE_CONSTANT;
        //MSG(ANSI_COLOR_CYAN "%f\n" ANSI_COLOR_RESET, corr_limit_ppb);
        corr_ppb = LIMIT(corr_ppb, corr_limit_ppb, -corr_limit_ppb);
    }

    return corr_ppb;
}

// ----------------------------------
