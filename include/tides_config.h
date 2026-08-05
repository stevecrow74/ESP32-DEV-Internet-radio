#pragma once

#include <Arduino.h>

void tidesConfigInit();

String tidesLocation();
double tidesLatitude();
double tidesLongitude();
uint32_t tidesRefreshMs();
String tidesSourceUrl();

void tidesConfigUpdate(
    const String &location,
    double latitude,
    double longitude,
    uint16_t refreshMinutes,
    const String &sourceUrl);
