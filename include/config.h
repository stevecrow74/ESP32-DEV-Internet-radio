/******************************************************************************
 *
 *  config.h
 *  ESP32 Smart Radio V2
 *
 ******************************************************************************/

#pragma once

#include <Arduino.h>
#include <Adafruit_ST7789.h>

//==================================================
// Firmware Version
//==================================================

constexpr uint8_t VERSION_MAJOR = 2;
constexpr uint8_t VERSION_MINOR = 0;
constexpr uint8_t VERSION_BUILD = 0;

//==================================================
// Display
//==================================================

constexpr uint16_t SCREEN_WIDTH  = 240;
constexpr uint16_t SCREEN_HEIGHT = 320;

constexpr uint16_t STATUS_HEIGHT = 20;
constexpr uint16_t RADIO_HEIGHT  = 80;

constexpr uint16_t WIDGET_Y      = STATUS_HEIGHT + RADIO_HEIGHT;
constexpr uint16_t WIDGET_HEIGHT = SCREEN_HEIGHT - WIDGET_Y;

//==================================================
// Display Refresh Rates
//==================================================

constexpr uint32_t DISPLAY_REFRESH_MS = 33;      // ~30 FPS
constexpr uint32_t CLOCK_UPDATE_MS    = 1000;
constexpr uint32_t ADSB_UPDATE_MS     = 5000;
constexpr uint32_t WEATHER_UPDATE_MS  = 600000;   // 10 min
constexpr uint32_t TIDE_UPDATE_MS     = 1800000;  // 30 min

//==================================================
// Colours
//==================================================

constexpr uint16_t COLOUR_BACKGROUND = ST77XX_BLACK;
constexpr uint16_t COLOUR_TEXT       = ST77XX_WHITE;
constexpr uint16_t COLOUR_DIVIDER    = ST77XX_WHITE;
constexpr uint16_t COLOUR_ACCENT     = ST77XX_CYAN;
constexpr uint16_t COLOUR_WARNING    = ST77XX_YELLOW;
constexpr uint16_t COLOUR_ERROR      = ST77XX_RED;
constexpr uint16_t COLOUR_OK         = ST77XX_GREEN;

//==================================================
// Fonts
//==================================================

constexpr uint8_t FONT_SMALL  = 1;
constexpr uint8_t FONT_MEDIUM = 2;
constexpr uint8_t FONT_LARGE  = 3;
constexpr uint8_t FONT_HUGE   = 4;

//==================================================
// Audio
//==================================================

constexpr uint8_t MAX_VOLUME     = 21;
constexpr uint8_t MIN_VOLUME     = 0;

//==================================================
// Widgets
//==================================================

constexpr uint8_t MAX_WIDGETS = 6;

//==================================================
// ADS-B
//==================================================

constexpr uint16_t ADSB_RANGE_KM = 100;
constexpr char ADSB_LOCAL_FEED_URL[] = "http://192.168.1.14/dump1090/data/aircraft.json";
constexpr char ADSB_PUBLIC_FEED_URL[] = "http://stevecrow74.com:1974/dump1090/data/aircraft.json";
constexpr char ADSB_LOCAL_SSID[] = "";   // Optional: set to your home SSID to force local feed there.
constexpr double ADSB_LATITUDE  = 53.277;
constexpr double ADSB_LONGITUDE = -8.886;

//==================================================
// Weather
//==================================================

constexpr char WEATHER_LOCATION[] = "Clarinbridge";

//==================================================
// Tides
//==================================================

constexpr char TIDE_LOCATION[] = "Galway";

//==================================================
// Screen Saver
//==================================================

constexpr uint32_t SCREENSAVER_TIMEOUT = 60000;

//==================================================
// Debug
//==================================================

constexpr bool DEBUG_SERIAL = true;
constexpr bool DEBUG_WIFI   = false;
constexpr bool DEBUG_AUDIO  = false;
constexpr bool DEBUG_WIDGET = false;