/******************************************************************************
 *
 * theme.h
 *
 ******************************************************************************/

#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

//--------------------------------------------------
// General Colours
//--------------------------------------------------

constexpr uint16_t COLOR_BACKGROUND = ST77XX_BLACK;

constexpr uint16_t COLOR_HEADER     = ST77XX_BLUE;

constexpr uint16_t COLOR_TEXT       = ST77XX_WHITE;

constexpr uint16_t COLOR_LABEL      = ST77XX_YELLOW;

constexpr uint16_t COLOR_DIVIDER    = ST77XX_WHITE;

constexpr uint16_t COLOR_ACCENT     = ST77XX_CYAN;

constexpr uint16_t COLOR_WARNING    = ST77XX_RED;

constexpr uint16_t COLOR_SUCCESS    = ST77XX_GREEN;

//--------------------------------------------------
// Audio Widget
//--------------------------------------------------

constexpr uint16_t COLOR_VOLUME_BAR = ST77XX_GREEN;

constexpr uint16_t COLOR_VOLUME_BG  = 0x39E7;

//--------------------------------------------------
// Weather
//--------------------------------------------------

constexpr uint16_t COLOR_TEMP = ST77XX_CYAN;

constexpr uint16_t COLOR_WIND = ST77XX_GREEN;

constexpr uint16_t COLOR_HUMIDITY = ST77XX_YELLOW;