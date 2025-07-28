/** A Kalman-filter based servo. This module implements the Kalman-filter introduced in the paper: 
 * 'Performance Analysis of Kalman-Filter-Based Clock Synchronization in IEEE 1588 Networks' by 
 * Giada Gorgi and Claudio Narduzzi (https://ieeexplore.ieee.org/document/5934411)
 */

#include "kalman_filter.h"
#include "cliutils/cli.h"
#include "flexptp/ptp_defs.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <flexptp_options.h>

/* ---- DEFAULT PARAMETERS ---- */

#ifndef SIGMA_THETA_SQUARED
#define SIGMA_THETA_SQUARED (1E-16)
#endif

#ifndef SIGMA_GAMMA_SQUARED
#define SIGMA_GAMMA_SQUARED (1E-12)
#endif

#ifndef SIGMA_CT_SQUARED
#define SIGMA_CT_SQUARED (1E-10)
#endif

/* ---- FAST TUNING PARAMETERS ---- */

#define FAST_TUNING_THRESHOLD (500E-09)
#define FAST_TUNING_COEFFICIENT (0.2)
#define CALM_TUNING_COEFFICIENT (0.01)

// ----------------

typedef double mtx[2][2];
typedef double vec[2];

#define MTX_ELEMENTWISE(ctx)              \
    for (uint8_t i = 0; i < 2; i++) {     \
        for (uint8_t j = 0; j < 2; j++) { \
            ctx                           \
        }                                 \
    }

#define VEC_ELEMENTWISE(ctx)          \
    for (uint8_t i = 0; i < 2; i++) { \
        ctx                           \
    }

static void mtx_copy(mtx dst, mtx src) {
    MTX_ELEMENTWISE(
        dst[i][j] = src[i][j];)
}

static void mtx_add(mtx r, mtx a, mtx b) {
    MTX_ELEMENTWISE(
        r[i][j] = a[i][j] + b[i][j];)
}

static void mtx_sub(mtx r, mtx a, mtx b) {
    MTX_ELEMENTWISE(
        r[i][j] = a[i][j] - b[i][j];)
}

static void mtx_scale(mtx r, double c, mtx a) {
    MTX_ELEMENTWISE(
        r[i][j] = c * a[i][j];)
}

static void mtx_dot(vec r, mtx m, vec v) {
    VEC_ELEMENTWISE(
        r[i] = m[i][0] * v[0] + m[i][1] * v[1];)
}

static void mtx_mul(mtx r, mtx a, mtx b) {
    r[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0];
    r[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1];
    r[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0];
    r[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1];
}

static void mtx_transp(mtx r, mtx m) {
    MTX_ELEMENTWISE(
        r[i][j] = m[j][i];)
}

static double mtx_det(mtx m) {
    return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}

static void mtx_inverse(mtx r, mtx m) {
    r[0][0] = m[1][1];
    r[0][1] = -m[0][1];
    r[1][0] = -m[1][0];
    r[1][1] = m[0][0];

    mtx_scale(r, 1 / mtx_det(m), r);
}

static void mtx_print(mtx m) {
    MSG("%.4f   %.4f\n%.4f   %.4f\n", m[0][0], m[0][1], m[1][0], m[1][1]);
}

static void mtx_zero(mtx m) {
    MTX_ELEMENTWISE(
        m[i][j] = 0;)
}

static void mtx_unit(mtx m) {
    MTX_ELEMENTWISE(
        m[i][j] = i == j;)
}

static void vec_copy(vec r, vec v) {
    VEC_ELEMENTWISE(
        r[i] = v[i];)
}

static void vec_add(vec r, vec a, vec b) {
    VEC_ELEMENTWISE(
        r[i] = a[i] + b[i];)
}

static void vec_sub(vec r, vec a, vec b) {
    VEC_ELEMENTWISE(
        r[i] = a[i] - b[i];)
}

static void vec_zero(vec v) {
    VEC_ELEMENTWISE(
        v[i] = 0;)
}

#define SQR(x) ((x) * (x))

// ----------------

/* ---- KALMAN FILTER VARIABLES ---- */

static mtx A;     // system matrix
static mtx B;     // input matrix
static mtx H;     // output matrix
static vec u;     // control input
static mtx Q;     // process noise covariance
static mtx K;     // Kalman gain
static mtx P;     // a posteriori error covariance matrix
static mtx P_pri; // a priori P error covariance matrix
static mtx R;     // measurement covariance matrix
static mtx I;     // identity matrix
static vec z;     // measurements
static vec x_pri; // a priori state estimator
static vec x;     // a posteriori state estimator

static double sigma_theta_squared;   // offset process variance (normalized)
static double sigma_gamma_squared;   // skew process variance (normalized)
static double sigma_theta_m_squared; // measurement variance (normalized)

/* ---- OTHER VARIABLES ---- */

static uint64_t cycle;  // cycle counter
static int32_t dt_prev; // previous time error

// --------------

#ifdef CLI_REG_CMD

#ifndef CMD_FUNCTION
#error "No CMD_FUNCTION macro has been defined, cannot register CLI functions!"
#endif

static CMD_FUNCTION(sts) {
    if (argc > 0) {
        if (!strcat("default", ppArgs[0])) {
            sigma_theta_squared = SIGMA_THETA_SQUARED;
        } else {
            sigma_theta_squared = atof(ppArgs[0]);
        }
    }

    MSG("sigma_theta^2 = %e\n", sigma_theta_squared);

    return 0;
}

static CMD_FUNCTION(sgs) {
    if (argc > 0) {
        if (!strcat("default", ppArgs[0])) {
            sigma_gamma_squared = SIGMA_GAMMA_SQUARED;
        } else {
            sigma_gamma_squared = atof(ppArgs[0]);
        }
    }

    MSG("sigma_gamma^2 = %e\n", sigma_gamma_squared);

    return 0;
}

static CMD_FUNCTION(stms) {
    if (argc > 0) {
        if (!strcat("default", ppArgs[0])) {
            sigma_theta_m_squared = SIGMA_CT_SQUARED;
        } else {
            sigma_theta_m_squared = atof(ppArgs[0]);
        }
    }

    MSG("sigma_theta_m^2 = %e\n", sigma_theta_m_squared);

    return 0;
}

// --------------

typedef enum {
    KF_CMDH_SIGMA_THETA_SQ = 0,
    KF_CMDH_SIGMA_GAMMA_SQ,
    KF_CMDH_SIGMA_THETA_M_SQ,
    KF_CMDH_N
} KalmanFilterCmdHandle;

static int cmd_handles[KF_CMDH_N];

static void register_cmd_commands() {
    cmd_handles[KF_CMDH_SIGMA_THETA_SQ] = CLI_REG_CMD("ptp servo st [var|default] \t\t\t Set or get sigma_theta^2 (s^2)", 3, 0, sts);
    cmd_handles[KF_CMDH_SIGMA_GAMMA_SQ] = CLI_REG_CMD("ptp servo sg [var|default] \t\t\t Set or get sigma_gamma^2 (s^2)", 3, 0, sgs);
    cmd_handles[KF_CMDH_SIGMA_THETA_M_SQ] = CLI_REG_CMD("ptp servo stm [var|default] \t\t\t Set or get sigma_theta_m^2 (s^2)", 3, 0, stms);
}

#ifdef CLI_REMOVE_CMD
static void remove_cmd_commands() {
    for (uint8_t i = 0; i < KF_CMDH_N; i++) {
        CLI_REMOVE_CMD(i);
    }
}
#endif

#endif

// --------------

static void init_variances() {
    sigma_theta_squared = SIGMA_THETA_SQUARED;
    sigma_gamma_squared = SIGMA_GAMMA_SQUARED;
    sigma_theta_m_squared = 0.5 * SIGMA_CT_SQUARED;
}

void kalman_filter_init() {
#ifdef CLI_REG_CMD
    register_cmd_commands();
#endif

    kalman_filter_reset();
    init_variances();
}

void kalman_filter_deinit() {
#ifdef CLI_REMOVE_CMD
    remove_cmd_commands();
#endif
}

void kalman_filter_reset() {
    // reset matrices and vectors
    mtx_unit(A);
    mtx_unit(B);
    mtx_unit(H);
    vec_zero(u);
    mtx_zero(Q);
    mtx_zero(K);
    mtx_copy(P_pri, Q);
    mtx_zero(P);
    mtx_unit(I);
    vec_zero(z);
    vec_zero(x_pri);
    vec_zero(x);

    // reset cycle count and previous time error
    cycle = 0;
    dt_prev = 0;
}

static void insert_DT(double DT) {
    A[0][1] = DT;
    B[0][1] = -DT;

    Q[0][0] = DT * sigma_theta_squared;
    Q[1][1] = DT * sigma_gamma_squared;

    R[0][0] = sigma_theta_m_squared;
    R[0][1] = R[1][0] = sigma_theta_m_squared / DT;
    R[1][1] = 2.0 * sigma_theta_m_squared / SQR(DT);
}

float kalman_filter_run(int32_t dt, PtpServoAuxInput *pAux) {
    double tuning_ppb = 0.0;

    // in the very first cycle skip running the filter
    if (cycle == 0) {
        goto retain_cycle_data;
    }

    /* ---- PREPARE THE PARAMETERS ---- */

    // calculate input data
    double skew = ((double)(dt - dt_prev)) / ((double)pAux->measSyncPeriodNs); // skew
    double offset = ((double)dt) * 1E-09;                                      // offset in seconds

    // compose measurement vector
    z[0] = offset;
    z[1] = skew;

    // inject DT into relevant places
    double DT = ((double)pAux->measSyncPeriodNs) * 1E-09;
    insert_DT(DT);

    /* ---- RUN THE FILTER ---- */

    if (cycle == 1) {
        mtx_copy(P, Q); // P(1|0) = Q
        vec_copy(x, z); // x = z
    }

    // prediction equations

    // (27)
    vec Ax, Bu;
    mtx_dot(Ax, A, x);      // Ax = A * x(n-1)
    mtx_dot(Bu, B, u);      // Bu = B * u(n-1)
    vec_add(x_pri, Ax, Bu); // x(n|n-1) = Ax + Bu

    // (28)
    mtx AP, At, APAt;
    mtx_mul(AP, A, P);       // AP = A * P
    mtx_transp(At, A);       // At = A'
    mtx_mul(APAt, AP, At);   // APAt = AP * At
    mtx_add(P_pri, APAt, Q); // P(n|n-1) = APAt + Q

    // correction equations

    // (30)
    mtx P_priR, P_priRinv;
    mtx_add(P_priR, P_pri, R);      // P_priR = P(n|n-1) + R
    mtx_inverse(P_priRinv, P_priR); // P_priRinv = (P_priR)^(-1)
    mtx_mul(K, P_pri, P_priRinv);   // K = P(n|n-1) * P_priRinv

    // (31)
    vec zxpri, Kzxpri;
    vec_sub(zxpri, z, x_pri);  // zxpri = z - x(n|n-1)
    mtx_dot(Kzxpri, K, zxpri); // Kzxpri = K * zxpri
    vec_add(x, x_pri, Kzxpri); // x = x(n|n-1) + Kzxpri

    // (32)
    mtx IK;
    mtx_sub(IK, I, K);     // IK = I - K
    mtx_mul(P, IK, P_pri); // P = IK * P(n|n-1)

    /* ---- TUNING ---- */

    // determine tuning
    double tuning_coefficient = (fabs(x[0]) > FAST_TUNING_THRESHOLD) ? FAST_TUNING_COEFFICIENT : CALM_TUNING_COEFFICIENT;
    double tuning = -x[1] + ((-x[0] * 1E+09 / pAux->measSyncPeriodNs) * tuning_coefficient);
    tuning_ppb = tuning * 1E+09;

    // feed back tuning
    u[0] = 0;
    u[1] = tuning;

    //MSG(PTP_COLOR_YELLOW "%12f" PTP_COLOR_BGREEN " %12f\n" PTP_COLOR_RESET, x[0] * 1E+09, x[1] * 1E+09);

    /* ---- DATA RETENTION ---- */

retain_cycle_data:
    dt_prev = dt;

    cycle++;

    return tuning_ppb;
}