#pragma once
#include <Arduino.h>

struct SensorData {
    float   t1 = NAN, t2 = NAN, rh1 = NAN, rh2 = NAN;
    bool    ok1 = false, ok2 = false;       // pembacaan terakhir valid
    uint8_t fail1 = 0, fail2 = 0;           // gagal beruntun
};

struct ProcessedData {
    float tControl = NAN;       // suhu umpan balik (rata-rata sensor valid)
    float rhControl = NAN;      // RH untuk kipas
    float tMax = NAN;           // suhu tertinggi (untuk safety)
    bool  tValid = false;
    bool  rhValid = false;
    bool  sensorMismatch = false;
    float error = 0;            // e = SP - T
    float dError = 0;           // e[k]-e[k-1] (terfilter)
};

struct SafetyState {
    bool sensorFault = false;
    bool overTemp = false;
    bool safe = false;
    uint8_t code = 0;           // 0=OK, 1=sensor gagal, 2=suhu darurat
};

struct ControlOutput {
    float gainScale = 0.5f;     // keluaran fuzzy 0..1
    float kpEff = 0, kdEff = 0;
    float pid = 0;              // 0..1
    float heater = 0;           // 0..1 (daya ke TRIAC)
    bool  fan1 = false, fan2 = false;
};

struct Telemetry {
    float t1 = NAN, t2 = NAN, tCtrl = NAN, rh = NAN;
    float heaterPct = 0;
    bool  fan1 = false, fan2 = false;
    uint8_t safetyCode = 0;
    bool  fresh = false;
};

extern SensorData    g_sensor;
extern ProcessedData g_proc;
extern SafetyState   g_safety;
extern ControlOutput g_ctrl;
