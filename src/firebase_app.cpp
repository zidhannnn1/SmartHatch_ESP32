#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h> // ESP32 Firebase Realtime Database
#include "config.h"
#include "data_types.h"
#include "setpoint.h"
#include "firebase_app.h"

static FirebaseData s_fbdo;
static FirebaseAuth s_auth;
static FirebaseConfig s_config;

static Telemetry s_snap;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

void sendFirebaseTelemetry() {
    Telemetry t;
    t.t1 = g_sensor.ok1 ? g_sensor.t1 : NAN;
    t.t2 = g_sensor.ok2 ? g_sensor.t2 : NAN;
    t.tCtrl = g_proc.tValid ? g_proc.tControl : NAN;
    t.rh = g_proc.rhValid ? g_proc.rhControl : NAN;
    t.heaterPct = g_ctrl.heater * 100.0f;
    t.fan1 = g_ctrl.fan1;
    t.fan2 = g_ctrl.fan2;
    t.safetyCode = g_safety.code;
    t.fresh = true;

    portENTER_CRITICAL(&s_mux);
    s_snap = t;
    portEXIT_CRITICAL(&s_mux);
}

static const char *safetyText(uint8_t c) {
    switch (c) {
        case 0:  return "AMAN";
        case 1:  return "SENSOR GAGAL";
        default: return "SUHU DARURAT";
    }
}

static void firebaseTask(void *) {
    bool isConfigured = (strstr(FIREBASE_HOST, "GANTI_") == nullptr);
    if (!isConfigured) {
        Serial.println("[FIREBASE] Konfigurasi belum diisi di include/config.h. Task Firebase standby.");
        vTaskDelete(nullptr);
        return;
    }

    // Tunggu sampai WiFi terhubung
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    s_config.host = FIREBASE_HOST;
    s_config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&s_config, &s_auth);
    Firebase.reconnectWiFi(true);
    s_fbdo.setResponseSize(1024);

    Serial.println("[FIREBASE] Terhubung ke Firebase Realtime Database!");

    uint32_t lastPush = 0;
    uint32_t lastReadSetpoint = 0;

    for (;;) {
        if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
            uint32_t now = millis();

            // 1. Push Telemetri setiap 2 detik
            if (now - lastPush >= 2000) {
                Telemetry t;
                portENTER_CRITICAL(&s_mux);
                t = s_snap;
                portEXIT_CRITICAL(&s_mux);

                if (t.fresh) {
                    FirebaseJson json;
                    if (!isnan(t.t1)) json.set("temp1", t.t1);
                    if (!isnan(t.t2)) json.set("temp2", t.t2);
                    if (!isnan(t.tCtrl)) json.set("tempAvg", t.tCtrl);
                    if (!isnan(t.rh)) json.set("humidAvg", t.rh);
                    json.set("sensor1Ok", g_sensor.ok1);
                    json.set("sensor2Ok", g_sensor.ok2);
                    json.set("dutyCycle", t.heaterPct);
                    json.set("error", g_proc.error);
                    json.set("errorRate", g_proc.dError);
                    json.set("fuzzyDelta", g_ctrl.gainScale);
                    json.set("kp", g_ctrl.kpEff);
                    json.set("ki", PID_KI);
                    json.set("kd", g_ctrl.kdEff);
                    json.set("heaterOn", t.heaterPct > 1.0f);
                    json.set("fan1On", t.fan1);
                    json.set("fan2On", t.fan2);
                    json.set("safeMode", !g_safety.safe);
                    json.set("safetyReason", safetyText(t.safetyCode));
                    json.set("tempSetpoint", (float)g_setpointTemp);
                    json.set("humidSetpoint", (float)g_setpointHumid);
                    json.set("uptimeSeconds", (uint32_t)(millis() / 1000));

                    if (Firebase.updateNode(s_fbdo, "/smarthatch/telemetry", json)) {
                        lastPush = now;
                    } else {
                        Serial.printf("[FIREBASE] Gagal push: %s\n", s_fbdo.errorReason().c_str());
                    }
                }
            }

            // 2. Baca Setpoint dari Web setiap 3 detik
            if (now - lastReadSetpoint >= 3000) {
                lastReadSetpoint = now;
                if (Firebase.getJSON(s_fbdo, "/smarthatch/setpoint")) {
                    FirebaseJsonData val;
                    FirebaseJson &j = s_fbdo.jsonObject();
                    if (j.get(val, "temp")) {
                        float spT = val.floatValue;
                        if (spT >= 35.0f && spT <= 40.0f && fabsf(spT - g_setpointTemp) > 0.05f) {
                            g_setpointTemp = spT;
                            Serial.printf("[FIREBASE] Setpoint Suhu diperbarui dari Web: %.1f °C\n", spT);
                        }
                    }
                    if (j.get(val, "humid")) {
                        float spH = val.floatValue;
                        if (spH >= 50.0f && spH <= 70.0f && fabsf(spH - g_setpointHumid) > 0.5f) {
                            g_setpointHumid = spH;
                            Serial.printf("[FIREBASE] Setpoint Humid diperbarui dari Web: %.0f %%\n", spH);
                        }
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void initFirebase() {
    xTaskCreatePinnedToCore(firebaseTask, "firebase", 8192, nullptr, 1, nullptr, 0);
}

