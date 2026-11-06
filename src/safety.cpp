#include <Arduino.h>
#include "safety.h"
#include "config.h"
#include "data_types.h"

SafetyState g_safety;

void initSafety() { g_safety = SafetyState(); }   // default: tidak aman sampai dicek

void checkSafety() {
    const bool prevSafe = g_safety.safe;

    // 1) Sensor gagal (salah satu sensor gagal beruntun >= batas) -> heater OFF
    g_safety.sensorFault = (g_sensor.fail1 >= SENSOR_FAIL_LIMIT) ||
                           (g_sensor.fail2 >= SENSOR_FAIL_LIMIT) ||
                           !g_proc.tValid;

    // 2) Suhu darurat > 40 °C, latch dengan histeresis
    if (g_proc.tValid) {
        if (g_proc.tMax > TEMP_EMERGENCY) g_safety.overTemp = true;
        else if (g_proc.tMax < TEMP_EMERGENCY_CLEAR) g_safety.overTemp = false;
    }

    g_safety.safe = !g_safety.sensorFault && !g_safety.overTemp;
    g_safety.code = g_safety.overTemp ? 2 : (g_safety.sensorFault ? 1 : 0);

    if (g_safety.safe != prevSafe) {
        Serial.printf("[SAFETY] %s (code=%u)\n",
                      g_safety.safe ? "AMAN, kontrol diizinkan" : "GANGGUAN, heater OFF",
                      g_safety.code);
    }
}

bool systemSafe() { return g_safety.safe; }
