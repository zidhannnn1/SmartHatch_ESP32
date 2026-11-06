#include "setpoint.h"
#include "config.h"

volatile float g_setpointTemp  = SETPOINT_TEMP;
volatile float g_setpointHumid = SETPOINT_RH;

void resetSetpoints() {
    g_setpointTemp  = SETPOINT_TEMP;
    g_setpointHumid = SETPOINT_RH;
}

