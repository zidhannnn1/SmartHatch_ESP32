#include <Arduino.h>
#include "pid.h"
#include "config.h"
#include "data_types.h"

ControlOutput g_ctrl;

static float s_integral = 0;                       // sum(e * Ts)
static constexpr float I_MAX = 1.0f / PID_KI;      // batas clamp: Ki*I <= 1

void resetPID() { s_integral = 0; g_ctrl.pid = 0; }

void initPID() { g_ctrl = ControlOutput(); g_ctrl.kpEff = PID_KP; g_ctrl.kdEff = PID_KD; resetPID(); }

// u = Kp*e + Ki*sum(e*Ts) + Kd*(de/Ts), Kp & Kd dijadwalkan oleh fuzzy
void calculatePID() {
    const float e = g_proc.error;
    const float kp = g_ctrl.kpEff, kd = g_ctrl.kdEff;

    float p = kp * e;
    float d = kd * (g_proc.dError / SAMPLING_S);

    // Integral dengan anti-windup: conditional integration + clamping
    float iNew = s_integral + e * SAMPLING_S;
    float uTry = p + PID_KI * iNew + d;
    bool satHigh = (uTry > OUTPUT_MAX) && (e > 0);
    bool satLow  = (uTry < OUTPUT_MIN) && (e < 0);
    if (!satHigh && !satLow) s_integral = iNew;
    s_integral = constrain(s_integral, 0.0f, I_MAX);

    float u = p + PID_KI * s_integral + d;
    g_ctrl.pid = constrain(u, OUTPUT_MIN, OUTPUT_MAX);
    g_ctrl.heater = g_ctrl.pid;
}
