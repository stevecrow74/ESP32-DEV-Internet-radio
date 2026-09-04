#pragma once

/******************************************************************************
 *
 * ESP32 Smart Radio V2
 * Project Configuration
 *
 ******************************************************************************/

//------------------------------------------------------
// Version
//------------------------------------------------------

constexpr char PROJECT_NAME[] = "ESP32 Smart Radio V2";
constexpr char PROJECT_VERSION[] = "0.2.0";

//------------------------------------------------------
// Audio
//------------------------------------------------------

constexpr uint8_t DEFAULT_VOLUME = 5;

// Override the audio input buffer size. The library uses these values
// for RAM and PSRAM-backed buffering respectively.
// Set to -1 to leave the library default unchanged.
constexpr int AUDIO_BUFFER_RAM_BYTES = 1600 * 5;
constexpr int AUDIO_BUFFER_PSRAM_BYTES = 300000;

//------------------------------------------------------
// Weather
//------------------------------------------------------

constexpr char WEATHER_API_KEY[] = "";

constexpr double LATITUDE  = 53.277;
constexpr double LONGITUDE = -8.886;

constexpr uint32_t WEATHER_REFRESH_MS = 15UL * 60UL * 1000UL;

//------------------------------------------------------
// Clock
//------------------------------------------------------

constexpr char NTP_SERVER[] = "pool.ntp.org";

constexpr long GMT_OFFSET = 0;
constexpr int DAYLIGHT_OFFSET = 3600;

//------------------------------------------------------
// Display
//------------------------------------------------------

constexpr uint16_t DISPLAY_BRIGHTNESS = 255;

//------------------------------------------------------
// Station Logos
//------------------------------------------------------

constexpr char LOGO_FOLDER[] = "/logos/";