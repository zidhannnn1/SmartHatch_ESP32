#include <Arduino.h>
#include "fuzzy.h"
#include "config.h"
#include "data_types.h"

// Fuzzy Mamdani sebagai GAIN SCHEDULER PID:
//  Input 1 : error suhu e  (7 label: NB NM NS ZO PS PM PB)
//  Input 2 : perubahan error de (5 label: NB NS ZO PS PB)
//  Output  : gain scale 0..1 (5 label: Z S M B VB)  -> Kp naik, Kd turun saat scale naik
//  35 aturan, MF segitiga (ujung berupa bahu), inferensi MIN-MAX, defuzzifikasi COA.

struct Tri { float a, b, c; };

static const Tri MF_E[7]  = { {-4,-4,-2.5f}, {-4,-2.5f,-1}, {-2.5f,-1,0}, {-1,0,1},
                              {0,1,2.5f},    {1,2.5f,4},     {2.5f,4,4} };
static const Tri MF_DE[5] = { {-0.5f,-0.5f,-0.15f}, {-0.5f,-0.15f,0}, {-0.15f,0,0.15f},
                              {0,0.15f,0.5f},       {0.15f,0.5f,0.5f} };
static const Tri MF_OUT[5]= { {0,0,0.25f}, {0,0.25f,0.5f}, {0.25f,0.5f,0.75f},
                              {0.5f,0.75f,1}, {0.75f,1,1} };

// RULE[e][de] -> indeks output (0=Z, 1=S, 2=M, 3=B, 4=VB)
static const uint8_t RULE[7][5] = {
 //  de: NB NS ZO PS PB
    {   4, 4, 4, 4, 4 },   // e = NB
    {   3, 3, 3, 4, 4 },   // NM
    {   2, 2, 2, 3, 3 },   // NS
    {   1, 0, 0, 1, 1 },   // ZO  (dekat setpoint: Kp kecil, Kd besar)
    {   2, 2, 2, 3, 3 },   // PS
    {   3, 3, 3, 4, 4 },   // PM
    {   4, 4, 4, 4, 4 },   // PB  (cold start: Kp besar)
};

static float tri(float x, const Tri &m) {
    if (m.a == m.b && x <= m.b) return 1.0f;       // bahu kiri
    if (m.b == m.c && x >= m.b) return 1.0f;       // bahu kanan
    if (x <= m.a || x >= m.c) return 0.0f;
    if (x == m.b) return 1.0f;
    return (x < m.b) ? (x - m.a) / (m.b - m.a) : (m.c - x) / (m.c - m.b);
}

void initFuzzy() { g_ctrl.gainScale = 0.5f; }

void calculateFuzzy() {
    float mE[7], mD[5], str[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 7; i++) mE[i] = tri(g_proc.error, MF_E[i]);
    for (int j = 0; j < 5; j++) mD[j] = tri(g_proc.dError, MF_DE[j]);

    // Inferensi: 35 aturan, AND = min, agregasi = max
    for (int i = 0; i < 7; i++)
        for (int j = 0; j < 5; j++) {
            float w = fminf(mE[i], mD[j]);
            uint8_t k = RULE[i][j];
            if (w > str[k]) str[k] = w;
        }

    // Defuzzifikasi Centroid of Area
    float num = 0, den = 0;
    for (int n = 0; n <= FUZZY_RES; n++) {
        float x = (float)n / FUZZY_RES, mu = 0;
        for (int k = 0; k < 5; k++) mu = fmaxf(mu, fminf(str[k], tri(x, MF_OUT[k])));
        num += mu * x; den += mu;
    }
    float s = (den > 1e-6f) ? num / den : 0.5f;
    g_ctrl.gainScale = s;
    g_ctrl.kpEff = PID_KP * (KP_SCALE_MIN + (KP_SCALE_MAX - KP_SCALE_MIN) * s);
    g_ctrl.kdEff = PID_KD * (KD_SCALE_MAX - (KD_SCALE_MAX - KD_SCALE_MIN) * s);
}
