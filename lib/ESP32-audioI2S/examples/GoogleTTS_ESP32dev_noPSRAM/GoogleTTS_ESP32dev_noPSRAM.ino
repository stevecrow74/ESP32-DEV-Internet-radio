/**
 * ============================================================
 * @file    GoogleTTS_ESP32dev_noPSRAM.ino
 * @brief   Google Text-to-Speech demo for ESP32 dev board
 *          with MAX98357A I2S amplifier — no PSRAM required
 * @version 1.0
 * @date    2026-03-10
 *
 * ============================================================
 * DESCRIPTION
 * ============================================================
 *
 *  Demonstrates Google Text-to-Speech (TTS) on an ESP32 dev
 *  board without PSRAM, using the ESP32-audioI2S-nopsram fork.
 *
 *  connecttospeech() sends the text to the Google TTS API over
 *  HTTPS and streams the response as a chunked MP3 directly to
 *  the I2S amplifier. No SD card required.
 *
 *  Google TTS returns mono MP3 at 24000 Hz / 64 kbps — a very
 *  light stream that fits comfortably in the 6 KB internal
 *  buffer available without PSRAM.
 *
 *  This example is adapted from the original ESP32-audioI2S
 *  repository by schreibfaul1. The upstream example targets
 *  the v3.x API (Audio::msg_t, audio_info_callback) which is
 *  not available in v2.0.6. This version uses the v2.x weak
 *  callback API instead.
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
 * API COMPATIBILITY NOTE
 * ============================================================
 *
 *  The upstream example uses the v3.x callback API :
 *    void my_audio_info(Audio::msg_t m) { ... }
 *    Audio::audio_info_callback = my_audio_info;
 *
 *  This API does not exist in v2.0.6. The v2.x equivalent is
 *  a set of global weak functions that the library calls
 *  automatically when defined in the sketch :
 *    void audio_info(const char* info) { ... }
 *    void audio_eof_speech(const char* info) { ... }
 *
 * ============================================================
 */

#include "Arduino.h"
#include "Audio_nopsram.h"  // ESP32-audioI2S-nopsram v2.0.6+GCC14 patches (schreibfaul1)
                            // renamed from Audio.h to avoid conflict with
                            // ESP32-audioI2S (schreibfaul1 upstream, requires PSRAM)
                            // ⚠ DO NOT replace with schreibfaul1/ESP32-audioI2S — see SOFTWARE section above
#include "WiFi.h"

// ─────────────────────────────────────────────────────────────
// USER CONFIGURATION
// ─────────────────────────────────────────────────────────────

static const char* WIFI_SSID = "**********";
static const char* WIFI_PASSWORD = "**********";

// Text to speak and language code (Google TTS)
// Language codes : "fr" French, "en" English, "de" German, etc.
static const char* TTS_TEXT = "Il faut manger pour vivre et non pas vivre pour manger."; // Molière, L'Avare (1668), Acte III, scène 1 
static const char* TTS_LANG = "fr";

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
#define WIFI_TIMEOUT_MS 15000  ///< Max time to wait for WiFi

// ─────────────────────────────────────────────────────────────
// GLOBALS
// ─────────────────────────────────────────────────────────────

Audio audio;

// ─────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Google TTS — ESP32 dev (no PSRAM) ===");

    // Connect to WiFi
    Serial.printf("[WiFi] Connecting to \"%s\" ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println("\n[WiFi] Timeout — restarting");
            ESP.restart();
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected  IP: %s\n", WiFi.localIP().toString().c_str());

    // Disable WiFi modem sleep to avoid latency spikes on the
    // TCP receive path that could cause audio dropouts
    WiFi.setSleep(false);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(VOLUME);

    Serial.printf("[TTS]  Speaking: \"%s\" [%s]\n", TTS_TEXT, TTS_LANG);
    audio.connecttospeech(TTS_TEXT, TTS_LANG);
}

// ─────────────────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────────────────

void loop() {
    audio.loop();
    vTaskDelay(1);  // yield to RTOS — required for stable WiFi/TCP operation
}

// ─────────────────────────────────────────────────────────────
// ESP32-audioI2S CALLBACKS  (v2.x weak function API)
// ─────────────────────────────────────────────────────────────

void audio_info(const char* info)        { Serial.printf("[info] %s\n", info); }
void audio_eof_speech(const char* info)  { Serial.printf("[eof]  %s\n", info); }
