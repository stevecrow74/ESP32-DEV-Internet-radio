#pragma once

#include <Arduino.h>

void tidesConfigInit();

String tidesLocation();
double tidesLatitude();
double tidesLongitude();
uint32_t tidesRefreshMs();

void tidesConfigUpdate(
    const String &location,
    double latitude,
    double longitude,
    uint16_t refreshMinutes);
