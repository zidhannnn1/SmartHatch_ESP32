#pragma once
void initActuator();
void updateActuator();   // terapkan g_ctrl.heater ke TRIAC
void heaterOff();        // paksa heater OFF + reset PID
void updateFan();        // kipas berdasarkan RH
