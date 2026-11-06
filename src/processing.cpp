#include <Arduino.h>
#include <math.h>
#include "processing.h"
#include "config.h"
#include "data_types.h"
#include "setpoint.h"

ProcessedData g_proc;

static float s_prevError = 0, s_dFilt = 0;
static bool  s_hasPrev = false;

void initProcessing() { s_hasPrev = false; s_dFilt = 0; }

void processSensorData() {
    const bool a = g_sensor.ok1, b = g_sensor.ok2;

    // Suhu kontrol & RH: rata-rata sensor yang valid
    if (a && b) {
        g_proc.tControl = 0.5f * (g_sensor.t1 + g_sensor.t2);
        g_proc.rhControl = 0.5f * (g_sensor.rh1 + g_sensor.rh2);
        g_proc.tMax = fmaxf(g_sensor.t1, g_sensor.t2);
        g_proc.sensorMismatch = fabsf(g_sensor.t1 - g_sensor.t2) > SENSOR_DIFF_WARN;
    } else if (a || b) {
        g_proc.tControl = a ? g_sensor.t1 : g_sensor.t2;
        g_proc.rhControl = a ? g_sensor.rh1 : g_sensor.rh2;
        g_proc.tMax = g_proc.tControl;
        g_proc.sensorMismatch = false;
    } else {
        g_proc.tControl = NAN; g_proc.rhControl = NAN; g_proc.tMax = NAN;
        g_proc.sensorMismatch = false;
    }
    g_proc.tValid  = !isnan(g_proc.tControl);
    g_proc.rhValid = !isnan(g_proc.rhControl);

    // Error dan perubahan error (hanya jika suhu valid)
    if (g_proc.tValid) {
        float e = g_setpointTemp - g_proc.tControl;
        if (!s_hasPrev) { s_prevError = e; s_hasPrev = true; s_dFilt = 0; }
        float raw = e - s_prevError;
        s_dFilt = DERIV_FILTER_ALPHA * raw + (1.0f - DERIV_FILTER_ALPHA) * s_dFilt;
        s_prevError = e;
        g_proc.error = e;
        g_proc.dError = s_dFilt;
    } else {
        s_hasPrev = false;
    }
}
