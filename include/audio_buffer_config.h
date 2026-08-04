#pragma once

#include <Arduino.h>

void audioBufferConfigInit();
int audioBufferRamBytes();
int audioBufferPsramBytes();
void audioBufferConfigUpdate(int ramBytes, int psramBytes);
