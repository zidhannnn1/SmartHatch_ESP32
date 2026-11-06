# SmartHatch_ESP32

Sistem Otomasi dan Kontrol Inkubator Telur Pintar berbasis **ESP32**, algoritma **PID + Fuzzy Logic**, dan terhubung ke **Firebase Realtime Database** dengan Dashboard Web siap deploy ke **Vercel**.

---

## 🚀 Fitur Utama

- **Kontrol Suhu & Kelembaban Presisi**:
  - Pemanas AC dikontrol melalui TRIAC zero-crossing dengan kontrol daya duty cycle.
  - Kipas 1 & Kipas 2 dengan kontrol histeresis kelembaban.
  - Kombinasi kontrol adaptif **Fuzzy Logic (Sugeno)** untuk *gain scheduling* $K_p$ dan $K_d$, serta loop **PID**.
- **Dual Sensor DHT22 & Safety Interlock**:
  - Redundansi sensor ganda dengan validasi *cross-check*, deteksi kegagalan sensor, dan *emergency cut-off* jika suhu $\ge 40^\circ\text{C}$.
- **Konektivitas Cloud Firebase Realtime Database**:
  - Telemetri dikirim tiap 2 detik (`/smarthatch/telemetry`).
  - Sinkronisasi setpoint dua arah secara real-time (`/smarthatch/setpoint`).
- **Dashboard Web Modern**:
  - Berbasis Web Component & Chart.js, responsive mobile/desktop.
  - Terintegrasi dengan Firebase SDK v9.
  - Siap di-deploy instan ke Vercel (folder `/web`).

---

## 📁 Struktur Direktori

```text
SmartHatch_ESP32/
├── include/                 # Header & konfigurasi sistem
│   ├── config.h             # Kredensial WiFi, Firebase, & parameter kontrol
│   ├── pin_config.h         # Pemetaan pin GPIO ESP32
│   └── ...
├── src/                     # Firmware ESP32 (Arduino Framework)
│   ├── main.cpp             # Scheduler & loop utama
│   ├── firebase_app.cpp     # Komunikasi Firebase RTDB
│   ├── wifi_app.cpp         # FreeRTOS WiFi Task
│   ├── pid.cpp & fuzzy.cpp  # Algoritma kontrol cerdas
│   └── ...
├── web/                     # Web Dashboard (Deploy ke Vercel)
│   ├── index.html           # UI Dashboard
│   ├── firebase-config.js   # Konfigurasi Firebase Web SDK
│   └── vercel.json          # Konfigurasi deploy Vercel
├── platformio.ini           # Konfigurasi PlatformIO
└── README.md
```

---

## 🛠️ Panduan Penggunaan

### 1. Konfigurasi Firmware (ESP32)

Buka `include/config.h` dan sesuaikan kredensial WiFi Anda:

```cpp
#define WIFI_SSID "NamaWiFiAnda"
#define WIFI_PASS "PasswordWiFiAnda"
```

> **Mode Uji Coba**: Jika ingin menguji tanpa sensor fisik terhubung, biarkan `#define DUMMY_TEST_MODE true`. Jika siap dipasang pada inkubator fisik, ubah menjadi `false`.

### 2. Upload Firmware via PlatformIO

Gunakan terminal atau PlatformIO extension di VS Code:

```bash
# Build firmware
pio run

# Upload ke modul ESP32
pio run -t upload

# Buka Serial Monitor (115200 baud)
pio device monitor -b 115200
```

### 3. Deploy Web Dashboard ke Vercel

```bash
cd web
npx vercel
```
Ikuti instruksi di terminal (tekan Enter untuk opsi default). Dashboard Anda akan langsung live di domain Vercel (HTTPS).
