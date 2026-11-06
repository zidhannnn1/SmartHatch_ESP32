# Panduan Deploy SmartHatch ke Vercel & Firebase

Panduan lengkap menghubungkan **ESP32 Inkubator** ke **Cloud Web Dashboard di Vercel** melalui **Firebase Realtime Database**.

---

## 1. Setup Firebase Realtime Database (Gratis - 3 Menit)

1. Buka [Firebase Console](https://console.firebase.google.com/) dan login dengan akun Google.
2. Klik **"Add project"** &rarr; beri nama (misal: `smarthatch-iot`) &rarr; lanjutkan sampai selesai.
3. Di menu sidebar kiri, buka **Build** &rarr; **Realtime Database** &rarr; klik **Create Database**.
4. Pilih lokasi terdekat (misal: **Singapore / `asia-southeast1`** atau **US Central**).
5. Pada pilihan Security Rules, pilih **Start in test mode** (agar bisa dibaca dan ditulis), klik **Enable**.
6. Masuk ke tab **Rules**, pastikan aturannya seperti ini lalu klik **Publish**:
   ```json
   {
     "rules": {
       ".read": true,
       ".write": true
     }
   }
   ```
7. Catat URL database kamu di atas tabel, contohnya:
   `https://smarthatch-iot-default-rtdb.asia-southeast1.firebasedatabase.app`

---

## 2. Dapatkan Kredensial Firebase

### A. Untuk Web Dashboard (Vercel):
1. Di Firebase Console, klik ikon **Gerigi (Project settings)** di kiri atas.
2. Di tab **General**, scroll ke bawah ke bagian **Your apps** &rarr; klik ikon **Web (`</>`)**.
3. Beri nama app (misal: `SmartHatch Web`) &rarr; klik **Register app**.
4. Salin objek `firebaseConfig` yang muncul, lalu buka file:
   [`web/firebase-config.js`](firebase-config.js)
5. Tempelkan kredensial tersebut ke dalam file tersebut:
   ```javascript
   const firebaseConfig = {
     apiKey: "AIzaSy...",
     authDomain: "smarthatch-iot.firebaseapp.com",
     databaseURL: "https://smarthatch-iot-default-rtdb.asia-southeast1.firebasedatabase.app",
     projectId: "smarthatch-iot",
     storageBucket: "smarthatch-iot.appspot.com",
     messagingSenderId: "123456789",
     appId: "1:123456789:web:..."
   };
   ```

### B. Untuk Firmware ESP32:
1. Di Firebase Console, buka **Project settings** &rarr; tab **Service accounts**.
2. Klik **Database secrets** (di bagian bawah) &rarr; klik **Show** pada secret token &rarr; salin token tersebut.
3. Buka file [`include/config.h`](../include/config.h) pada proyek ESP32, lalu isi:
   ```cpp
   #define FIREBASE_HOST "https://smarthatch-iot-default-rtdb.asia-southeast1.firebasedatabase.app"
   #define FIREBASE_AUTH "TOKEN_DATABASE_SECRET_ANDA"
   ```

---

## 3. Upload Kode ke ESP32

1. Pastikan WiFi SSID dan Password di [`include/config.h`](../include/config.h) sudah diisi.
2. Sambungkan ESP32 ke laptop via USB.
3. Jalankan perintah upload:
   ```bash
   pio run -t upload
   ```
4. Buka Serial Monitor (115200 baud). ESP32 akan menampilkan:
   ```
   [WIFI] Terhubung! IP: 192.168.1.xxx
   [FIREBASE] Terhubung ke Firebase Realtime Database!
   ```

---

## 4. Deploy Web Dashboard ke Vercel (1 Menit)

### Cara 1: Menggunakan Vercel CLI (Paling Cepat)
1. Buka terminal di folder `web`:
   ```bash
   cd /Users/mac/smt7/capstone/SmartHatch_ESP32/web
   ```
2. Jalankan perintah:
   ```bash
   npx vercel
   ```
3. Login dan ikuti instruksi terminal (cukup tekan **Enter** untuk semua pilihan default).
4. Selesai! Kamu akan mendapatkan link publik Vercel, misalnya:
   `https://smarthatch-cloud.vercel.app`

### Cara 2: Menggunakan GitHub + Vercel Dashboard
1. Push project ini ke GitHub.
2. Buka [Vercel Dashboard](https://vercel.com/new).
3. Import repository GitHub kamu.
4. Pada bagian **Root Directory**, pilih folder `web`.
5. Klik **Deploy**!

---

## Selesai! 🎉
Sekarang dashboard inkubator kamu bisa diakses oleh siapa saja dari mana saja melalui link Vercel dengan sinkronisasi data real-time dua arah ke ESP32!

