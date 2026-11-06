#pragma once
#include <Arduino.h>

// Runtime setpoints (initialized from config.h defaults, changeable via dashboard/Firebase)
extern volatile float g_setpointTemp;
extern volatile float g_setpointHumid;

void resetSetpoints();   // restore defaults from config.h

