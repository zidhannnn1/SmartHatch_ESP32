#pragma once
// =====================================================================
//  SmartHatch-ESP32 : konfigurasi sistem
// =====================================================================

// ---------- WiFi (ISI SESUAI KONEKSI ANDA) ----------
#include <cstdint>
#define WIFI_SSID "RAMANDIKA"
#define WIFI_PASS "takonbapak"

// ---------- Firebase Realtime Database (Untuk Web Vercel) ----------
// clang-format off
#define FIREBASE_HOST "https://smarthatch-iot-480b3-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "7nz8azzaBeQ75ObA1IvwDb5T4bXsGP3sIi3351cp"
// clang-format on

// ---------- Mode Pengujian (Dummy / Data Acak) ----------
// Set true untuk menguji dashboard dengan data sensor acak dinamis tanpa
// hardware sensor Set false untuk mode produksi dengan sensor DHT22 fisik
#define DUMMY_TEST_MODE true

// ---------- Setpoint & batas ----------
constexpr float SETPOINT_TEMP = 37.5f; // °C
constexpr float TEMP_MIN_OP = 37.0f;   // °C
constexpr float TEMP_MAX_OP = 38.0f;   // °C
constexpr float SETPOINT_RH = 60.0f;   // %RH
constexpr float RH_MIN_OP = 50.0f;     // %RH
constexpr float RH_MAX_OP = 70.0f;     // %RH
constexpr float RH_HYST = 2.0f;        // histeresis kipas (%RH)

// ---------- Waktu ----------
constexpr uint32_t SAMPLING_MS = 2000; // sampling sensor (DHT22 min 2 s)
constexpr float SAMPLING_S = SAMPLING_MS / 1000.0f;

// ---------- Safety ----------
constexpr float TEMP_EMERGENCY = 40.0f;       // > 40 °C -> heater OFF
constexpr float TEMP_EMERGENCY_CLEAR = 39.0f; // histeresis pelepasan latch
constexpr float SENSOR_TEMP_MIN = 0.0f;       // validasi rentang DHT22
constexpr float SENSOR_TEMP_MAX = 60.0f;
constexpr float SENSOR_DIFF_WARN = 2.0f; // selisih 2 sensor (peringatan)
constexpr uint8_t SENSOR_FAIL_LIMIT = 3; // gagal beruntun -> sensor fault

// ---------- Output kontrol ----------
constexpr float OUTPUT_MIN = 0.0f; // 0 %
constexpr float OUTPUT_MAX = 1.0f; // 100 %

// ---------- Parameter PID dasar (AWAL, perlu tuning di alat!) ----------
// Plant: K = 21,25 °C/unit, tau ~ 98 menit (D-02). Nilai ini titik awal
// konservatif.
constexpr float PID_KP = 0.40f;
constexpr float PID_KI = 0.0015f;
constexpr float PID_KD = 15.0f;
constexpr float DERIV_FILTER_ALPHA =
    0.3f; // low-pass untuk de (DHT22 resolusi 0,1 °C)

// ---------- Fuzzy gain scheduler ----------
// scale 0..1 dari fuzzy  ->  Kp_eff = Kp*(KP_SCALE_MIN +
// (KP_SCALE_MAX-KP_SCALE_MIN)*scale)
//                            Kd_eff = Kd*(KD_SCALE_MAX -
//                            (KD_SCALE_MAX-KD_SCALE_MIN)*scale)
constexpr float KP_SCALE_MIN = 0.5f;
constexpr float KP_SCALE_MAX = 1.5f;
constexpr float KD_SCALE_MIN = 0.5f;
constexpr float KD_SCALE_MAX = 1.5f;
constexpr int FUZZY_RES = 100; // resolusi centroid

// ---------- TRIAC ----------
constexpr uint32_t AC_HALF_PERIOD_US =
    10000; // 50 Hz -> 10 ms per setengah siklus
constexpr uint32_t ZC_OFFSET_US =
    0; // koreksi offset modul zero-cross (kalibrasi)
constexpr uint32_t TRIAC_PULSE_US = 100; // lebar pulsa gate
constexpr float HEATER_MIN_ON = 0.01f;   // di bawah ini heater dianggap OFF

// ---------- Relay ----------
constexpr bool RELAY_ACTIVE_LOW = true; // sesuaikan modul relay
