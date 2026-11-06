#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "web_server.h"
#include "config.h"
#include "data_types.h"
#include "setpoint.h"
#include "dashboard_html.h"

static AsyncWebServer s_server(80);

// Ring buffer for history (last 120 samples ~ 4 minutes at 2s interval)
static constexpr int HIST_LEN = 120;
static float s_histTemp[HIST_LEN];
static float s_histHumid[HIST_LEN];
static float s_histDuty[HIST_LEN];
static int   s_histHead  = 0;
static int   s_histCount = 0;
static portMUX_TYPE s_histMux = portMUX_INITIALIZER_UNLOCKED;

void webHistoryPush(float temp, float humid, float duty) {
    portENTER_CRITICAL(&s_histMux);
    s_histTemp[s_histHead]  = temp;
    s_histHumid[s_histHead] = humid;
    s_histDuty[s_histHead]  = duty;
    s_histHead = (s_histHead + 1) % HIST_LEN;
    if (s_histCount < HIST_LEN) s_histCount++;
    portEXIT_CRITICAL(&s_histMux);
}

static const char *safetyReasonText(uint8_t code) {
    switch (code) {
        case 0:  return "";
        case 1:  return "Sensor gagal beruntun";
        default: return "Suhu melebihi batas darurat";
    }
}

static void handleApiData(AsyncWebServerRequest *req) {
    JsonDocument doc;

    doc["temp1"]     = g_sensor.ok1 ? g_sensor.t1 : NAN;
    doc["temp2"]     = g_sensor.ok2 ? g_sensor.t2 : NAN;
    doc["humid1"]    = g_sensor.ok1 ? g_sensor.rh1 : NAN;
    doc["humid2"]    = g_sensor.ok2 ? g_sensor.rh2 : NAN;
    doc["tempAvg"]   = g_proc.tValid ? g_proc.tControl : NAN;
    doc["humidAvg"]  = g_proc.rhValid ? g_proc.rhControl : NAN;
    doc["sensor1Ok"] = g_sensor.ok1;
    doc["sensor2Ok"] = g_sensor.ok2;

    doc["tempSetpoint"]  = (float)g_setpointTemp;
    doc["humidSetpoint"] = (float)g_setpointHumid;
    doc["dutyCycle"]     = g_ctrl.heater * 100.0f;

    doc["error"]      = g_proc.error;
    doc["errorRate"]   = g_proc.dError;
    doc["fuzzyDelta"]  = g_ctrl.gainScale;
    doc["kp"]          = g_ctrl.kpEff;
    doc["ki"]          = PID_KI;
    doc["kd"]          = g_ctrl.kdEff;

    doc["heaterOn"] = g_ctrl.heater > HEATER_MIN_ON;
    doc["fan1On"]   = g_ctrl.fan1;
    doc["fan2On"]   = g_ctrl.fan2;

    doc["safeMode"]      = !g_safety.safe;
    doc["safetyReason"]  = safetyReasonText(g_safety.code);
    doc["uptimeSeconds"] = (uint32_t)(millis() / 1000);

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
}

static void handleApiHistory(AsyncWebServerRequest *req) {
    JsonDocument doc;
    JsonArray tArr = doc["temp"].to<JsonArray>();
    JsonArray hArr = doc["humid"].to<JsonArray>();
    JsonArray dArr = doc["duty"].to<JsonArray>();

    portENTER_CRITICAL(&s_histMux);
    int count = s_histCount;
    int start = (s_histHead - count + HIST_LEN) % HIST_LEN;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % HIST_LEN;
        tArr.add(s_histTemp[idx]);
        hArr.add(s_histHumid[idx]);
        dArr.add(s_histDuty[idx]);
    }
    portEXIT_CRITICAL(&s_histMux);

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
}

static void handleApiSetpoint(AsyncWebServerRequest *req, uint8_t *data, size_t len,
                               size_t /*index*/, size_t /*total*/) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        req->send(400, "application/json", "{\"error\":\"JSON tidak valid\"}");
        return;
    }

    float temp  = doc["temp"]  | NAN;
    float humid = doc["humid"] | NAN;

    if (isnan(temp) || temp < 35.0f || temp > 40.0f) {
        req->send(400, "application/json", "{\"error\":\"Suhu harus 35-40\"}");
        return;
    }
    if (isnan(humid) || humid < 50.0f || humid > 70.0f) {
        req->send(400, "application/json", "{\"error\":\"Kelembapan harus 50-70\"}");
        return;
    }

    g_setpointTemp  = temp;
    g_setpointHumid = humid;
    Serial.printf("[WEB] Setpoint diperbarui: T=%.1f RH=%.0f\n", temp, humid);
    req->send(200, "application/json", "{\"ok\":true}");
}

static void handleApiReset(AsyncWebServerRequest *req) {
    resetSetpoints();

    // Reset history buffer
    portENTER_CRITICAL(&s_histMux);
    s_histHead  = 0;
    s_histCount = 0;
    portEXIT_CRITICAL(&s_histMux);

    Serial.println("[WEB] Sistem direset ke default");
    req->send(200, "application/json", "{\"ok\":true}");
}

void initWebServer() {
    // Serve gzipped dashboard HTML
    s_server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        AsyncWebServerResponse *resp = req->beginResponse(200, "text/html",
            DASHBOARD_HTML_GZ, DASHBOARD_HTML_GZ_LEN);
        resp->addHeader("Content-Encoding", "gzip");
        req->send(resp);
    });

    // REST API
    s_server.on("/api/data", HTTP_GET, handleApiData);
    s_server.on("/api/history", HTTP_GET, handleApiHistory);

    s_server.on("/api/setpoint", HTTP_POST, [](AsyncWebServerRequest *req) {
        // body handler sends response
    }, nullptr, handleApiSetpoint);

    s_server.on("/api/reset", HTTP_POST, handleApiReset);

    s_server.begin();
    Serial.println("[WEB] Server aktif di port 80");
}

