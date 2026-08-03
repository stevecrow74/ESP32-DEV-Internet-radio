/******************************************************************************
 *
 * display.h
 *
 ******************************************************************************/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Global TFT object

extern Adafruit_ST7789 tft;

// Initialisation

void displayInit();
void displayLoop();
void displayRefresh();

// Screen sections

void drawStatusBar();
void drawRadioPanel();
void drawWidgetArea();

// Dynamic updates

void updateStation(const String &station);
void updateTitle(const String &title);
void updateVolume(uint8_t volume);
void updateClock(const String &timeStr);