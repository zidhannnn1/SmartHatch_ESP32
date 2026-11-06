#include <Arduino.h>
#include <DHT.h>
#include "sensor.h"
#include "config.h"
#include "pin_config.h"
#include "data_types.h"

SensorData g_sensor;

static DHT dht1(PIN_DHT1, DHT22);
static DHT dht2(PIN_DHT2, DHT22);

static bool valid(float t, float h) {
    return !isnan(t) && !isnan(h) &&
           t >= SENSOR_TEMP_MIN && t <= SENSOR_TEMP_MAX &&
           h >= 0.0f && h <= 100.0f;
}

void initSensor() {
    dht1.begin();
    dht2.begin();
}

void readSensor() {
#if DUMMY_TEST_MODE
    // Simulasi random walk realistis untuk pengujian integrasi IoT <-> Web Dashboard
    static float s_simTemp = 36.8f;
    static float s_simHumid = 58.0f;

    // Perubahan acak kecil pada setiap siklus
    float noiseT = (random(-15, 16)) / 100.0f; // -0.15 .. +0.15 °C
    s_simTemp += noiseT;
    s_simTemp = constrain(s_simTemp, 35.8f, 38.5f);

    float noiseH = (random(-40, 41)) / 100.0f; // -0.4 .. +0.4 %
    s_simHumid += noiseH;
    s_simHumid = constrain(s_simHumid, 50.0f, 68.0f);

    // Dua sensor dengan sedikit variasi acak (meniru 2 sensor fisik)
    float t1 = s_simTemp + (random(-8, 9)) / 100.0f;
    float h1 = s_simHumid + (random(-20, 21)) / 100.0f;
    float t2 = s_simTemp + (random(-8, 9)) / 100.0f;
    float h2 = s_simHumid + (random(-20, 21)) / 100.0f;

    g_sensor.ok1 = true;
    g_sensor.t1 = t1;
    g_sensor.rh1 = h1;
    g_sensor.fail1 = 0;

    g_sensor.ok2 = true;
    g_sensor.t2 = t2;
    g_sensor.rh2 = h2;
    g_sensor.fail2 = 0;
#else
    float t1 = dht1.readTemperature(), h1 = dht1.readHumidity();
    float t2 = dht2.readTemperature(), h2 = dht2.readHumidity();

    g_sensor.ok1 = valid(t1, h1);
    g_sensor.ok2 = valid(t2, h2);

    if (g_sensor.ok1) { g_sensor.t1 = t1; g_sensor.rh1 = h1; g_sensor.fail1 = 0; }
    else { if (g_sensor.fail1 < 255) g_sensor.fail1++; g_sensor.t1 = NAN; g_sensor.rh1 = NAN; }

    if (g_sensor.ok2) { g_sensor.t2 = t2; g_sensor.rh2 = h2; g_sensor.fail2 = 0; }
    else { if (g_sensor.fail2 < 255) g_sensor.fail2++; g_sensor.t2 = NAN; g_sensor.rh2 = NAN; }
#endif
}
