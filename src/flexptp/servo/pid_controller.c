/* (C) András Wiesner, 2021 */

#include "pid_controller.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <flexptp_options.h>

// ----------------------------------

#ifndef K_P
#define K_P (0.5 * 0.476) ///< Default Kp parameter value
#endif

#ifndef K_I
#define K_I (0) ///< Default Ki parameters value
#endif

#ifndef K_D
#define K_D (3.0) ///< Default Kd parameter value
#endif

// ----------------------------------

static bool logInternals = false; ///< Decides if servo's internal operation shoud be reported or not
static bool firstRun = true;      ///< Indicates if first run did not occur yet

// ----------------------------------

// static float P_FACTOR = 0.5 * 0.476;
// static float D_FACTOR = 2.0 * 0.476;

// PID servo coefficients
static float Kp = K_P; ///< Proportional factor
static float Ki = K_I;           ///< Integrating factor
static float Kd = K_D;         ///< Differentiating factor

// ----------------------------------

static double rd_prev_ppb; ///< relative frequency error measured in previous iteration
static double integrator_value; ///< value stored in the integrator

// ----------------------------------

#ifdef CLI_REG_CMD

#ifndef CMD_FUNCTION
#error "No CMD_FUNCTION macro has been defined, cannot register CLI functions!"
#endif

static CMD_FUNCTION(CB_params) {
    // set if parameters passed after command
    if (argc >= 3) {
        Kp = atof(ppArgs[0]);
        Ki = atof(ppArgs[1]);
        Kd = atof(ppArgs[2]);
    }

    MSG("> PTP params: K_p = %.3f, K_i = %.3f, K_d = %.3f\n", Kp, Ki, Kd);

    return 0;
}

static CMD_FUNCTION(CB_logInternals) {
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

#ifdef CLI_REG_CMD
static void pid_ctrl_register_cli_commands() {
    sCliCmdIdx.params = CLI_REG_CMD("ptp servo params [Kp Ki Kd] \t\t\tSet or query Kp, Ki, and Kd servo parameters", 3, 0, CB_params);
    sCliCmdIdx.internals = CLI_REG_CMD("ptp servo log internals {on|off} \t\t\tEnable or disable logging of servo internals", 4, 1, CB_logInternals);
}
#endif

#ifdef CLI_REMOVE_CMD
static void pid_ctrl_remove_cli_commands() {
    CLI_REMOVE_CMD(sCliCmdIdx.params);
    CLI_REMOVE_CMD(sCliCmdIdx.internals);
}
#endif

#endif // CLI_REG_CMD

void pid_ctrl_init() {
    pid_ctrl_reset();

#ifdef CLI_REG_CMD
    pid_ctrl_register_cli_commands();
#endif // CLI_REG_CMD
}

void pid_ctrl_deinit() {
#ifdef CLI_REMOVE_CMD
    pid_ctrl_remove_cli_commands();
#endif // CLI_REMOVE_CMD
}

void pid_ctrl_reset() {
    firstRun = true;
    integrator_value = 0;
}

float pid_ctrl_run(int32_t dt, PtpServoAuxInput *pAux) {
    if (firstRun) {
        firstRun = false;
        rd_prev_ppb = dt;
        integrator_value = 0;
        return 0;
    }

    // calculate relative time error
    double rd_ppb = ((double)dt) / (pAux->measSyncPeriodNs) * 1E+09;

    // calculate difference
    double rd_D_ppb = Kd * (rd_ppb - rd_prev_ppb);

    // calculate output (run the PD controller)
    double corr_ppb = -(Kp * (rd_ppb + rd_D_ppb) + integrator_value) * exp((pAux->measSyncPeriodNs * 1E-09) - 1.0);

    // update integrator
    integrator_value += Ki * rd_ppb;

    // store error value (time difference) for use in next iteration
    rd_prev_ppb = rd_ppb;

    CLILOG(logInternals, "%d %f\n", dt, rd_ppb);

    return corr_ppb;
}

// ----------------------------------
