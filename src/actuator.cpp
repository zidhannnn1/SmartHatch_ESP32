#include <Arduino.h>
#include <math.h>
#include "actuator.h"
#include "pid.h"
#include "config.h"
#include "pin_config.h"
#include "data_types.h"

// ---------------------------------------------------------------
//  TRIAC phase-angle control
//  Zero-cross (GPIO35, 100 Hz) -> timer hardware -> pulsa gate (GPIO18)
//  t_delay = alpha/pi * 10 ms, alpha dari invers P(a)=(pi-a)/pi + sin(2a)/(2pi)
// ---------------------------------------------------------------
static hw_timer_t *s_timer = nullptr;
static volatile uint32_t s_delayUs = 0;
static volatile bool     s_fire = false;
static float s_lut[181];                       // P(alpha) untuk alpha = 0..180 derajat
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR onFireTimer() {
    digitalWrite(PIN_TRIAC, HIGH);
    delayMicroseconds(TRIAC_PULSE_US);
    digitalWrite(PIN_TRIAC, LOW);
}

static void IRAM_ATTR onZeroCross() {
    digitalWrite(PIN_TRIAC, LOW);
    if (!s_fire) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerWrite(s_timer, 0);
    timerAlarm(s_timer, s_delayUs, false, 0);
#else
    timerWrite(s_timer, 0);
    timerAlarmWrite(s_timer, s_delayUs, false);
    timerAlarmEnable(s_timer);
#endif
}

static void buildLut() {
    for (int d = 0; d <= 180; d++) {
        float a = d * (float)M_PI / 180.0f;
        s_lut[d] = (M_PI - a) / M_PI + sinf(2 * a) / (2 * M_PI);
    }
}

// power 0..1 -> delay (us). P turun monoton dari 1 (a=0) ke 0 (a=180).
static uint32_t powerToDelayUs(float p) {
    int i = 0;
    while (i < 180 && s_lut[i + 1] > p) i++;
    float frac = (s_lut[i] - s_lut[i + 1]) > 1e-6f ? (s_lut[i] - p) / (s_lut[i] - s_lut[i + 1]) : 0;
    float alphaDeg = i + constrain(frac, 0.0f, 1.0f);
    uint32_t d = (uint32_t)(alphaDeg / 180.0f * AC_HALF_PERIOD_US) + ZC_OFFSET_US;
    return constrain(d, (uint32_t)200, AC_HALF_PERIOD_US - 500);
}

static void setHeaterPower(float p) {
    p = constrain(p, OUTPUT_MIN, OUTPUT_MAX);
    if (p < HEATER_MIN_ON) {
        s_fire = false;
        digitalWrite(PIN_TRIAC, LOW);
        return;
    }
    uint32_t d = (p > 0.99f) ? 200 : powerToDelayUs(p);
    portENTER_CRITICAL(&s_mux);
    s_delayUs = d;
    s_fire = true;
    portEXIT_CRITICAL(&s_mux);
}

// ---------------------------------------------------------------
//  Relay kipas
// ---------------------------------------------------------------
static void writeRelay(int pin, bool on) {
    digitalWrite(pin, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void initActuator() {
    // Keadaan aman dulu: gate TRIAC LOW, relay OFF
    pinMode(PIN_TRIAC, OUTPUT);
    digitalWrite(PIN_TRIAC, LOW);
    pinMode(PIN_FAN1, OUTPUT);
    pinMode(PIN_FAN2, OUTPUT);
    writeRelay(PIN_FAN1, false);
    writeRelay(PIN_FAN2, false);

    buildLut();
    pinMode(PIN_ZEROCROSS, INPUT);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    s_timer = timerBegin(1000000);                 // 1 MHz -> 1 tick = 1 us
    timerAttachInterrupt(s_timer, &onFireTimer);
#else
    s_timer = timerBegin(0, 80, true);             // 80 MHz / 80 = 1 MHz
    timerAttachInterrupt(s_timer, &onFireTimer, true);
#endif
    attachInterrupt(digitalPinToInterrupt(PIN_ZEROCROSS), onZeroCross, RISING);
}

void updateActuator() { setHeaterPower(g_ctrl.heater); }

void heaterOff() {
    g_ctrl.heater = 0;
    resetPID();
    setHeaterPower(0);
}

void updateFan() {
    static bool f1 = false, f2 = false;

    if (g_safety.overTemp) {
        // Suhu darurat: heater mati, kipas ON untuk membuang panas
        f1 = true; f2 = true;
    } else if (!g_proc.rhValid) {
        f1 = true; f2 = false;                     // RH tidak diketahui: sirkulasi saja
    } else {
        const float rh = g_proc.rhControl;
        // Kipas 1 (sirkulasi): OFF < 50 %, ON >= 50 % (histeresis)
        if (!f1 && rh >= RH_MIN_OP + RH_HYST) f1 = true;
        else if (f1 && rh < RH_MIN_OP - RH_HYST) f1 = false;
        // Kipas 2 (ventilasi): ON > 70 %, OFF saat RH < 70 - hist
        if (!f2 && rh > RH_MAX_OP) f2 = true;
        else if (f2 && rh < RH_MAX_OP - RH_HYST) f2 = false;
        if (f2) f1 = true;                         // ventilasi aktif -> sirkulasi ikut ON
    }

    g_ctrl.fan1 = f1;
    g_ctrl.fan2 = f2;
    writeRelay(PIN_FAN1, f1);
    writeRelay(PIN_FAN2, f2);
}
