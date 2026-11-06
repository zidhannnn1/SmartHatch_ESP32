#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "wifi_app.h"

static void wifiTask(void *) {
    uint32_t lastTry = 0;
    bool ipPrinted = false;

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            ipPrinted = false;
            if (millis() - lastTry > 10000) {
                lastTry = millis();
                Serial.printf("[WIFI] Menghubungkan ke '%s'...\n", WIFI_SSID);
                WiFi.disconnect();
                WiFi.begin(WIFI_SSID, WIFI_PASS);
            }
        } else {
            if (!ipPrinted) {
                Serial.printf("[WIFI] Berhasil terhubung! IP: %s\n", WiFi.localIP().toString().c_str());
                ipPrinted = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    xTaskCreatePinnedToCore(wifiTask, "wifi", 4096, nullptr, 1, nullptr, 0);
}

