/**
 * ============================================================
 * @file    WeatherTTS_ESP32dev_noPSRAM.ino
 * @brief   Weather bulletin via Text-to-Speech for ESP32 dev
 *          board with MAX98357A I2S amplifier — no PSRAM required
 * @version 1.0
 * @date    2026-03-14
 *
 * ============================================================
 * DESCRIPTION
 * ============================================================
 *
 *  Fetches current weather conditions from the Open-Meteo API
 *  (free, no API key required) and reads them aloud in French
 *  using Google Text-to-Speech via connecttospeech().
 *
 *  Example output :
 *    "À Strasbourg, il fait 8 degrés. Ciel couvert."
 *
 *  Flow :
 *    1. Connect to WiFi
 *    2. HTTP GET to Open-Meteo API → JSON response
 *    3. Parse temperature and weather_code (WMO standard)
 *    4. Build a French sentence from the data
 *    5. TCP probe to translate.google.com:443 — confirms the
 *       lwIP stack is ready for a new HTTPS connection after
 *       the preceding HTTP fetch
 *    6. Send to Google TTS → play MP3 response via I2S
 *
 * ============================================================
 * API
 * ============================================================
 *
 *  Weather data : Open-Meteo (https://open-meteo.com)
 *    Free, open-source, no API key, no registration required.
 *    Non-commercial use. Please credit Open-Meteo if you
 *    publish a project using their data.
 *
 *  TTS : Google Text-to-Speech (unofficial endpoint)
 *    Used internally by connecttospeech() in ESP32-audioI2S.
 *    No API key required. Availability is not guaranteed.
 *
 * ============================================================
 * HARDWARE
 * ============================================================
 *
 *  MCU  : ESP32 dev board (no PSRAM)
 *  AMP  : MAX98357A I2S Class-D amplifier module
 *
 *  I2S WIRING
 *  ┌─────────────────────────┬──────────────────────────────┐
 *  │  ESP32 dev board        │  MAX98357A board             │
 *  ├─────────────────────────┼──────────────────────────────┤
 *  │  GPIO27  – BCLK         │  BCLK  (bit clock)           │
 *  │  GPIO26  – LRC          │  LRC / WS / LRCK             │
 *  │  GPIO25  – DOUT         │  DIN   (data in)             │
 *  ├─────────────────────────┼──────────────────────────────┤
 *  │  3V3                    │  VIN                         │
 *  │  GND                    │  GND                         │
 *  │  GND                    │  GAIN  (fixed gain ~9 dB)    │
 *  │  —                      │  SD    (not connected)       │
 *  └─────────────────────────┴──────────────────────────────┘
 *
 * ============================================================
 * SOFTWARE
 * ============================================================
 *
 *  IDE       : Arduino IDE 2.x
 *  Board pkg : esp32 by Espressif (v3.x recommended)
 *
 *  Library   : ESP32-audioI2S-nopsram v1.0.0 — github.com/PLSousa/ESP32-audioI2S
 *              Base         : ESP32-audioI2S v2.0.6 by schreibfaul1
 *              Header       : Audio_nopsram.h  (renamed from Audio.h)
 *
 *              This fork applies 9 patches to v2.0.6 :
 *                Patches 1–4 : GCC 14 compatibility
 *                  1. Audio_nopsram.cpp — min() type mismatch (uint32_t vs size_t)
 *                  2. aac_decoder.cpp  — int val → int32_t val (5 functions)
 *                  3. aac_decoder.cpp  — (uint32_t*)last → (unsigned int*)last
 *                  4. aac_decoder.cpp  — DecodeHuffmanScalar: uint32_t bitBuf
 *                                        → unsigned int bitBuf
 *                Patches 5–9 : stability fixes backported from v3.2.1
 *                  5. parseHttpResponseHeader() — vTaskDelay 3→5 ticks
 *                  6. parseHttpResponseHeader() — fixed inverted log condition
 *                                                 ("chunked data transfer")
 *                  7. parseHttpResponseHeader() — fixed inverted log condition
 *                                                 ("icy-name" metadata)
 *                  8. findNextSync() MP3         — added return on no sync found
 *                  9. sendBytes()                — added mid-stream ID3 tag handling
 *
 *  ⚠ DO NOT replace this library with schreibfaul1/ESP32-audioI2S.
 *    v3.x of the original requires PSRAM and will crash with OOM on this board.
 *    To receive updates from this fork, download the latest ZIP from :
 *      https://github.com/PLSousa/ESP32-audioI2S
 *
 *  Arduino IDE board settings (Tools menu):
 *    Board            : ESP32 Dev Module
 *    PSRAM            : Disabled
 *    Partition Scheme : Huge APP (3MB No OTA / 1MB SPIFFS)
 *    CPU Frequency    : 240 MHz
 *
 * ============================================================
 */

#include "Arduino.h"
#include "Audio_nopsram.h"  // ESP32-audioI2S-nopsram v2.0.6+GCC14 patches (schreibfaul1)
                            // renamed from Audio.h to avoid conflict with
                            // ESP32-audioI2S (schreibfaul1 upstream, requires PSRAM)
                            // ⚠ DO NOT replace with schreibfaul1/ESP32-audioI2S — see SOFTWARE section above
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"    // Install "ArduinoJson" by Benoit Blanchon via Library Manager
#include "lwip/sockets.h"   // POSIX socket API (lwIP)
#include "lwip/netdb.h"     // getaddrinfo()

// ─────────────────────────────────────────────────────────────
// USER CONFIGURATION
// ─────────────────────────────────────────────────────────────

/** WiFi network name (SSID) */
static const char* WIFI_SSID = "***********";

/** WiFi password */
static const char* WIFI_PASSWORD = "*********";

// Location — update coordinates and city name for your location.
// To find coordinates : https://open-meteo.com (use the map)
static const char* CITY_NAME    = "Strasbourg";
static const float LATITUDE     = 48.5734f;
static const float LONGITUDE    =  7.7521f;

// TTS language
static const char* TTS_LANG     = "fr";

// ─────────────────────────────────────────────────────────────
// PIN DEFINITIONS — I2S amplifier (MAX98357A)
// ─────────────────────────────────────────────────────────────

#define I2S_BCLK      27   ///< Bit clock       → GPIO27
#define I2S_LRC       26   ///< Left/right clock → GPIO26
#define I2S_DOUT      25   ///< Data out         → GPIO25

// ─────────────────────────────────────────────────────────────
// SETTINGS
// ─────────────────────────────────────────────────────────────

#define VOLUME            12   ///< 0 (mute) to 21 (maximum)
#define WIFI_TIMEOUT_MS 15000

// ─────────────────────────────────────────────────────────────
// WMO WEATHER CODE → FRENCH DESCRIPTION
// ─────────────────────────────────────────────────────────────
//
// Open-Meteo returns a WMO weather interpretation code.
// Reference : https://open-meteo.com/en/docs (section Variables)
//
const char* wmoToFrench(int code) {
    if (code == 0)                return "Ciel dégagé";
    if (code == 1)                return "Principalement dégagé";
    if (code == 2)                return "Partiellement nuageux";
    if (code == 3)                return "Ciel couvert";
    if (code >= 45 && code <= 48) return "Brouillard";
    if (code >= 51 && code <= 55) return "Bruine légère";
    if (code >= 56 && code <= 57) return "Bruine verglaçante";
    if (code >= 61 && code <= 63) return "Pluie modérée";
    if (code == 65)               return "Pluie forte";
    if (code >= 66 && code <= 67) return "Pluie verglaçante";
    if (code >= 71 && code <= 73) return "Neige modérée";
    if (code == 75)               return "Neige forte";
    if (code == 77)               return "Grains de neige";
    if (code >= 80 && code <= 82) return "Averses de pluie";
    if (code >= 85 && code <= 86) return "Averses de neige";
    if (code >= 95 && code <= 99) return "Orage";
    return "Conditions inconnues";
}

// ─────────────────────────────────────────────────────────────
// GLOBALS
// ─────────────────────────────────────────────────────────────

Audio audio;

// ─────────────────────────────────────────────────────────────
// FETCH WEATHER
// ─────────────────────────────────────────────────────────────

/**
 * @brief  Fetch current weather from Open-Meteo and build TTS sentence.
 *
 * @param  sentence  Output buffer for the TTS text.
 * @param  maxLen    Size of the output buffer.
 * @return true on success, false on HTTP or JSON error.
 */
bool fetchWeather(char* sentence, size_t maxLen) {
    char url[256];
    snprintf(url, sizeof(url),
        "http://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,weather_code"
        "&timezone=Europe%%2FParis",
        LATITUDE, LONGITUDE);

    Serial.printf("[HTTP]  GET %s\n", url);

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[HTTP]  Error: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[JSON]  Parse error: %s\n", err.c_str());
        return false;
    }

    float temp    = doc["current"]["temperature_2m"].as<float>();
    int   wmoCode = doc["current"]["weather_code"].as<int>();
    int   tempInt = (int)roundf(temp);

    Serial.printf("[Weather] %s : %d°C, WMO code %d (%s)\n",
                  CITY_NAME, tempInt, wmoCode, wmoToFrench(wmoCode));

    snprintf(sentence, maxLen,
        "À %s, il fait %d degrés. %s.",
        CITY_NAME, tempInt, wmoToFrench(wmoCode));

    return true;
}

// ─────────────────────────────────────────────────────────────
// TCP PROBE
// ─────────────────────────────────────────────────────────────

/**
 * @brief  Confirm the lwIP TCP/IP stack is ready for a new HTTPS connection.
 *
 * After an HTTP fetch, the TCP stack needs a moment to close the previous
 * connection and be ready for the next one. This probe attempts a real TCP
 * handshake to translate.google.com:443 and retries until success.
 *
 * @return true if TCP handshake succeeded, false on timeout.
 */
bool waitTCPReady() {
    Serial.print("[TCP]   Probing translate.google.com:443 ");
    unsigned long start = millis();
    while (millis() - start < 10000) {
        struct addrinfo hints = {};
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo* res = nullptr;
        if (getaddrinfo("translate.google.com", "443", &hints, &res) != 0 || !res) {
            Serial.print(".");
            delay(500);
            continue;
        }
        int sock = socket(res->ai_family, res->ai_socktype, 0);
        if (sock < 0) { freeaddrinfo(res); Serial.print("."); delay(500); continue; }
        struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        int conn = connect(sock, res->ai_addr, res->ai_addrlen);
        close(sock);
        freeaddrinfo(res);
        if (conn == 0) { Serial.println(" ✓ ready"); return true; }
        Serial.print(".");
        delay(500);
    }
    Serial.println(" ✗ timeout");
    return false;
}

// ─────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Weather TTS — ESP32 dev (no PSRAM) ===");

    // Connect to WiFi
    Serial.printf("[WiFi]  Connecting to \"%s\" ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println("\n[WiFi]  Timeout — restarting");
            ESP.restart();
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi]  Connected  IP: %s\n", WiFi.localIP().toString().c_str());
    WiFi.setSleep(false);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(VOLUME);

    // Fetch weather with retry, then speak
    char sentence[128];
    while (!fetchWeather(sentence, sizeof(sentence))) {
        Serial.println("[INFO]  Retry in 5 seconds...");
        delay(5000);
    }

    Serial.printf("[TTS]   Speaking: \"%s\"\n", sentence);

    // Wait for TCP stack to be ready for HTTPS before calling connecttospeech()
    waitTCPReady();
    audio.connecttospeech(sentence, TTS_LANG);
}

// ─────────────────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────────────────

void loop() {
    audio.loop();
    vTaskDelay(1);
}

// ─────────────────────────────────────────────────────────────
// ESP32-audioI2S CALLBACKS
// ─────────────────────────────────────────────────────────────

void audio_info(const char* info)       { Serial.printf("[info]  %s\n", info); }
void audio_eof_speech(const char* info) { Serial.printf("[eof]   %s\n", info); }
