#include <Arduino.h>
#include "config.h"
#include "data_types.h"
#include "sensor.h"
#include "processing.h"
#include "safety.h"
#include "pid.h"
#include "fuzzy.h"
#include "actuator.h"
#include "wifi_app.h"
#include "web_server.h"
#include "firebase_app.h"

static uint32_t s_lastSample = 0;

// Penjadwalan berbasis millis(), tanpa delay()
static bool samplingTimeReached() {
    uint32_t now = millis();
    if (now - s_lastSample >= SAMPLING_MS) { s_lastSample = now; return true; }
    return false;
}

static void printStatus() {
    Serial.printf("T1=%.1f T2=%.1f Tc=%.2f RH=%.1f | e=%.2f de=%.3f | Kp=%.3f Kd=%.2f | "
                  "Heater=%.0f%% Fan1=%d Fan2=%d | Safety=%u\n",
                  g_sensor.t1, g_sensor.t2, g_proc.tControl, g_proc.rhControl,
                  g_proc.error, g_proc.dError, g_ctrl.kpEff, g_ctrl.kdEff,
                  g_ctrl.heater * 100.0f, g_ctrl.fan1, g_ctrl.fan2, g_safety.code);
}

void setup() {
    Serial.begin(115200);
    initActuator();          // pertama: pastikan heater OFF saat boot
    initSensor();
    initProcessing();
    initSafety();
    initPID();
    initFuzzy();
    initWiFi();              // Koneksi WiFi di Core 0
    initWebServer();         // HTTP dashboard lokal di port 80
    initFirebase();          // Sinkronisasi Firebase Realtime Database (untuk Web Vercel)
    s_lastSample = millis() - SAMPLING_MS;   // sampel pertama langsung
    Serial.println("SmartHatch-ESP32 siap");
}

void loop() {
    if (samplingTimeReached()) {
        readSensor();
        processSensorData();
        checkSafety();

        if (systemSafe()) {
            calculateFuzzy();    // fuzzy menjadwalkan Kp & Kd dulu
            calculatePID();      // lalu PID memakai gain terbaru
            updateActuator();
        } else {
            heaterOff();
        }

        updateFan();
        sendFirebaseTelemetry();
        printStatus();

        // Simpan data ke buffer riwayat dashboard web
        if (g_proc.tValid) {
            webHistoryPush(g_proc.tControl, g_proc.rhControl, g_ctrl.heater * 100.0f);
        }
    }
}
