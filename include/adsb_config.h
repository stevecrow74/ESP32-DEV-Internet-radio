#pragma once

#include <Arduino.h>

void adsbConfigInit();

uint16_t adsbRangeKm();
String adsbLocalFeedUrl();
String adsbPublicFeedUrl();
String adsbLocalSsid();

void adsbConfigUpdate(
    uint16_t rangeKm,
    const String &localFeedUrl,
    const String &publicFeedUrl,
    const String &localSsid);
