#pragma once

#include <Arduino.h>
#include "config.h"

bool audioInit();
void audioLoop();

bool audioConnect(const char *url);
void audioStop();
void audioToggleMute();
void audioSetVolume(uint8_t v);
uint8_t audioGetVolume();