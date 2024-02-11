/* The flexPTP project, (C) András Wiesner, 2024 */

#include <flexptp/ptp_core.h>
#include <flexptp/servo/pd_controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <flexptp_options.h>

// ----------------------------------

static struct {
    bool internals;
} sLog;

static struct {
    bool firstRun;
} sState;

// ----------------------------------

//static float P_FACTOR = 0.5 * 0.476;
//static float D_FACTOR = 2.0 * 0.476;

static float P_FACTOR = 0.5 * 0.476;
static float I_FACTOR = 0;
static float D_FACTOR = 3.0;

// ----------------------------------

static double rd_prev_ppb;      // relative frequency error measured in previous iteration
static double integrator_value;

// ----------------------------------

#ifdef CLI_REG_CMD

static int CB_params(const CliToken_Type * ppArgs, uint8_t argc)
{
    // set if parameters passed after command
    if (argc >= 3) {
        P_FACTOR = atof(ppArgs[0]);
        I_FACTOR = atof(ppArgs[1]);
        D_FACTOR = atof(ppArgs[2]);
    }

    MSG("> PTP params: K_p = %.3f, K_i = %.3f, K_d = %.3f\n", P_FACTOR, I_FACTOR, D_FACTOR);

    return 0;
}

static int CB_logInternals(const CliToken_Type * ppArgs, uint8_t argc)
{
    if (argc >= 1) {
        int en = ONOFF(ppArgs[0]);
        if (en >= 0) {
            if (en && !sLog.internals) {
                MSG("\nSyncIntv. [ns] | dt [ns] | gamma [ppb]\n\n");
            }
            sLog.internals = en;
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
} sCliCmdIdx = { 0 };

static void pd_ctrl_register_cli_commands()
{
    sCliCmdIdx.params = cli_register_command("ptp servo params [Kp Kd] \t\t\tSet or query K_p and K_d servo parameters", 3, 0, CB_params);
    sCliCmdIdx.internals = cli_register_command("ptp servo log internals {on|off} \t\t\tEnable or disable logging of servo internals", 4, 1, CB_logInternals);
}

static void pd_ctrl_remove_cli_commands()
{
    cli_remove_command(sCliCmdIdx.params);
    cli_remove_command(sCliCmdIdx.internals);
}

#endif                          // CLI_REG_CMD

void pd_ctrl_init()
{
    pd_ctrl_reset();

#ifdef CLI_REG_CMD
    pd_ctrl_register_cli_commands();
#endif                          // CLI_REG_CMD
}

void pd_ctrl_deinit()
{
#ifdef CLI_REG_CMD
    pd_ctrl_remove_cli_commands();
#endif                          // CLI_REG_CMD
}

void pd_ctrl_reset()
{
    sState.firstRun = true;
    integrator_value = 0;
}

float pd_ctrl_run(int32_t dt, PtpServoAuxInput * pAux)
{
    if (sState.firstRun) {
        sState.firstRun = false;
        rd_prev_ppb = dt;
        integrator_value = 0;
        return 0;
    }

    // calculate relative frequency error
    //double rd_ppb = ((double) dt) / (pAux->msgPeriodMs * 1E+06 + dt) * 1E+09;
    double rd_ppb = ((double)dt) / (pAux->measSyncPeriod + dt) * 1E+09;

    // calculate difference
    double rd_D_ppb = D_FACTOR * (rd_ppb - rd_prev_ppb);

    // calculate output (run the PD controller)
    double corr_ppb = -(P_FACTOR * (rd_ppb + rd_D_ppb) + integrator_value) * exp((pAux->measSyncPeriod * 1E-09) - 1.0);

    // update integrator
    integrator_value += I_FACTOR * rd_ppb;

    // store error value (time difference) for use in next iteration
    rd_prev_ppb = rd_ppb;

    CLILOG(sLog.internals, "%d %f\n", dt, rd_ppb);

    return corr_ppb;
}

// ----------------------------------
