/**
 * ============================================================
 * @file    AudioPlayer_SD_ESP32dev_noPSRAM.ino
 * @brief   Audio file player from SD card for ESP32 dev board
 *          with MAX98357A I2S amplifier — no PSRAM required
 * @version 1.0
 * @date    2026-03-09
 *
 * ============================================================
 * DESCRIPTION
 * ============================================================
 *
 *  Plays an audio file from a microSD card via I2S.
 *  Supported formats : WAV (16-bit PCM), MP3.
 *
 *  WAV files must be 16-bit PCM. Files in 24-bit or 32-bit
 *  float will be rejected by the decoder with no audio output.
 *  To convert any WAV file to the correct format, use ffmpeg :
 *
 *    ffmpeg -i input.wav -acodec pcm_s16le -ar 44100 -ac 2 output.wav
 *
 *  MP3 files play directly with no conversion required.
 *
 * ============================================================
 * HARDWARE
 * ============================================================
 *
 *  MCU  : ESP32 dev board (no PSRAM), 240 MHz
 *  AMP  : MAX98357A I2S Class-D amplifier module
 *  SD   : microSD card module (SPI), FAT32 formatted
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
 *  SD CARD WIRING (SPI)
 *  ┌─────────────────────────┬──────────────────────────────┐
 *  │  ESP32 dev board        │  SD card module              │
 *  ├─────────────────────────┼──────────────────────────────┤
 *  │  GPIO5   – CS           │  CS                          │
 *  │  GPIO18  – SCK          │  SCK                         │
 *  │  GPIO19  – MISO         │  MISO                        │
 *  │  GPIO23  – MOSI         │  MOSI                        │
 *  ├─────────────────────────┼──────────────────────────────┤
 *  │  3V3                    │  VCC                         │
 *  │  GND                    │  GND                         │
 *  └─────────────────────────┴──────────────────────────────┘
 *
 *  SD card must be formatted as FAT32.
 *  Place your audio file in the root directory of the SD card.
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
 * AUDIO FILE
 * ============================================================
 *
 *  The example file used during development (forest16.wav) is a
 *  forest ambience recording downloaded from freesound.org under
 *  a Creative Commons licence. The original file was 32-bit float
 *  WAV and was converted to 16-bit PCM stereo 44100 Hz using the
 *  ffmpeg command shown above before copying to the SD card.
 *
 * ============================================================
 */

#include "Arduino.h"
#include "Audio_nopsram.h"  // ESP32-audioI2S-nopsram v2.0.6+GCC14 patches (schreibfaul1)
                            // renamed from Audio.h to avoid conflict with
                            // ESP32-audioI2S (schreibfaul1 upstream, requires PSRAM)
                            // ⚠ DO NOT replace with schreibfaul1/ESP32-audioI2S — see SOFTWARE section above
#include "SPI.h"
#include "SD.h"
#include "FS.h"

// ─────────────────────────────────────────────────────────────
// PIN DEFINITIONS
// ─────────────────────────────────────────────────────────────

// SD card (SPI)
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

// I2S amplifier (MAX98357A)
#define I2S_BCLK      27   ///< Bit clock       → GPIO27
#define I2S_LRC       26   ///< Left/right clock → GPIO26
#define I2S_DOUT      25   ///< Data out         → GPIO25

// ─────────────────────────────────────────────────────────────
// CONFIGURATION
// ─────────────────────────────────────────────────────────────

// File to play — must be in the root directory of the SD card.
// Supported formats : WAV (16-bit PCM only), MP3.
// Example : "/forest16.wav" or "/track01.mp3"
#define AUDIO_FILE    "/forest16.wav"

// Volume : 0 (mute) to 21 (maximum)
#define VOLUME         6

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
    Serial.println("\n=== Audio Player SD — ESP32 dev (no PSRAM) ===");

    // SD card — CS must be driven HIGH before SPI.begin() to
    // avoid bus contention at startup
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);

    if (!SD.begin(SD_CS)) {
        Serial.println("[ERROR] SD card mount failed — check wiring and FAT32 format");
        return;
    }
    Serial.printf("[SD]   Card mounted — free heap: %d bytes\n", ESP.getFreeHeap());

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(VOLUME);

    Serial.printf("[Audio] Opening file: %s\n", AUDIO_FILE);
    audio.connecttoFS(SD, AUDIO_FILE);
}

// ─────────────────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────────────────

void loop() {
    audio.loop();
}

// ─────────────────────────────────────────────────────────────
// ESP32-audioI2S CALLBACKS
// ─────────────────────────────────────────────────────────────

void audio_info(const char* info)    { Serial.printf("[info]    %s\n", info); }
void audio_id3data(const char* info) { Serial.printf("[id3]     %s\n", info); }
void audio_eof_mp3(const char* info) { Serial.printf("[eof]     %s\n", info); }
