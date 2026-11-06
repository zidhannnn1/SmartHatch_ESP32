#!/usr/bin/env python3
"""
SmartHatch ESP32 IoT Simulator & Local Dashboard Server
======================================================
Menyimulasikan logika firmware ESP32 (Sensor, PID, Fuzzy, Fans, dan REST API)
sehingga kamu bisa menguji integrasi Web Dashboard dengan backend IoT tanpa board ESP32.
"""

import http.server
import socketserver
import json
import random
import time
import os

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

# State IoT (meniru global state ESP32)
state = {
    "sp_temp": 37.5,
    "sp_humid": 60.0,
    "current_temp": 36.5,
    "current_humid": 58.0,
    "duty_cycle": 50.0,
    "error": 1.0,
    "error_rate": 0.0,
    "prev_error": 1.0,
    "fuzzy_delta": 0.5,
    "kp": 0.40,
    "ki": 0.0015,
    "kd": 15.0,
    "heater_on": True,
    "fan1_on": False,
    "fan2_on": False,
    "safe_mode": False,
    "safety_reason": "",
    "start_time": time.time(),
    "history": {
        "temp": [],
        "humid": [],
        "duty": []
    }
}

def update_iot_physics():
    """Simulasi loop kontrol IoT (mirip loop() di main.cpp ESP32)"""
    sp_t = state["sp_temp"]
    sp_h = state["sp_humid"]

    # Thermal response: jika heater aktif, suhu perlahan naik menuju setpoint
    heater_power = state["duty_cycle"] / 100.0
    ambient = 28.0
    heating_rate = 0.08 * heater_power
    cooling_rate = 0.02 * (state["current_temp"] - ambient)
    noise_t = (random.random() - 0.5) * 0.06

    state["current_temp"] += heating_rate - cooling_rate + noise_t
    state["current_temp"] = max(25.0, min(42.0, state["current_temp"]))

    # Kelembapan
    noise_h = (random.random() - 0.5) * 0.4
    state["current_humid"] += noise_h
    state["current_humid"] = max(40.0, min(80.0, state["current_humid"]))

    # Sensor 1 & 2
    t1 = state["current_temp"] + (random.random() - 0.5) * 0.1
    t2 = state["current_temp"] + (random.random() - 0.5) * 0.1
    h1 = state["current_humid"] + (random.random() - 0.5) * 0.3
    h2 = state["current_humid"] + (random.random() - 0.5) * 0.3

    t_avg = 0.5 * (t1 + t2)
    h_avg = 0.5 * (h1 + h2)

    # PID & Error calculation (sesuai processing.cpp & pid.cpp)
    e = sp_t - t_avg
    de = 0.3 * (e - state["prev_error"]) + 0.7 * state["error_rate"]
    state["prev_error"] = e
    state["error"] = e
    state["error_rate"] = de

    # Fuzzy gain scale (0..1)
    scale = max(0.0, min(1.0, 0.5 + abs(e) * 0.3))
    state["fuzzy_delta"] = scale
    state["kp"] = 0.40 * (0.5 + 1.0 * scale)
    state["kd"] = 15.0 * (1.5 - 1.0 * scale)

    # Duty cycle
    raw_duty = e * 35.0 + 30.0
    state["duty_cycle"] = max(0.0, min(100.0, raw_duty))
    state["heater_on"] = state["duty_cycle"] > 1.0

    # Fans logic (sesuai actuator.cpp updateFan)
    state["fan1_on"] = h_avg >= 52.0
    state["fan2_on"] = h_avg > 70.0

    # Ring buffer history
    hist = state["history"]
    hist["temp"].append(round(t_avg, 2))
    hist["humid"].append(round(h_avg, 1))
    hist["duty"].append(round(state["duty_cycle"], 1))
    if len(hist["temp"]) > 120:
        hist["temp"].pop(0)
        hist["humid"].pop(0)
        hist["duty"].pop(0)

    state["t1"] = t1
    state["t2"] = t2
    state["h1"] = h1
    state["h2"] = h2
    state["t_avg"] = t_avg
    state["h_avg"] = h_avg


class SmartHatchHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            self.path = "/dashboard.html"
            return super().do_GET()

        elif self.path == "/api/data":
            update_iot_physics()
            uptime = int(time.time() - state["start_time"])
            payload = {
                "temp1": round(state["t1"], 2),
                "temp2": round(state["t2"], 2),
                "humid1": round(state["h1"], 1),
                "humid2": round(state["h2"], 1),
                "tempAvg": round(state["t_avg"], 2),
                "humidAvg": round(state["h_avg"], 1),
                "sensor1Ok": True,
                "sensor2Ok": True,
                "tempSetpoint": round(state["sp_temp"], 1),
                "humidSetpoint": round(state["sp_humid"], 0),
                "dutyCycle": round(state["duty_cycle"], 1),
                "error": round(state["error"], 2),
                "errorRate": round(state["error_rate"], 3),
                "fuzzyDelta": round(state["fuzzy_delta"], 3),
                "kp": round(state["kp"], 2),
                "ki": state["ki"],
                "kd": round(state["kd"], 2),
                "heaterOn": state["heater_on"],
                "fan1On": state["fan1_on"],
                "fan2On": state["fan2_on"],
                "safeMode": state["safe_mode"],
                "safetyReason": state["safety_reason"],
                "uptimeSeconds": uptime
            }
            body = json.dumps(payload).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(body)

        elif self.path == "/api/history":
            body = json.dumps(state["history"]).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(body)

        else:
            return super().do_GET()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        post_data = self.rfile.read(length)

        if self.path == "/api/setpoint":
            try:
                data = json.loads(post_data.decode("utf-8"))
                temp = float(data.get("temp", 0))
                humid = float(data.get("humid", 0))

                if not (35.0 <= temp <= 40.0):
                    self._send_json(400, {"error": "Suhu harus 35 - 40°C"})
                    return
                if not (50.0 <= humid <= 70.0):
                    self._send_json(400, {"error": "Kelembapan harus 50 - 70%"})
                    return

                state["sp_temp"] = temp
                state["sp_humid"] = humid
                print(f"[SIMULATOR] Setpoint diperbarui dari Web: Temp={temp}°C, Humid={humid}%")
                self._send_json(200, {"ok": True})
            except Exception as e:
                self._send_json(400, {"error": str(e)})

        elif self.path == "/api/reset":
            state["sp_temp"] = 37.5
            state["sp_humid"] = 60.0
            state["history"] = {"temp": [], "humid": [], "duty": []}
            print("[SIMULATOR] Sistem direset ke default via Web")
            self._send_json(200, {"ok": True})

        else:
            self.send_response(404)
            self.end_headers()

    def _send_json(self, code, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        # Reduksi log spam GET /api/data setiap 2 detik
        if "GET /api/data" in args[0] or "GET /api/history" in args[0]:
            return
        super().log_message(format, *args)


if __name__ == "__main__":
    # Pre-populate history sedikit agar grafik langsung ada garis
    for _ in range(20):
        update_iot_physics()

    with socketserver.TCPServer(("", PORT), SmartHatchHandler) as httpd:
        print("=" * 60)
        print(f" SmartHatch IoT & Dashboard Simulator BERJALAN!")
        print(f" URL Dashboard: http://localhost:{PORT}/")
        print("=" * 60)
        print("Log request API (setpoint & reset) akan muncul di bawah ini:")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer dihentikan.")

