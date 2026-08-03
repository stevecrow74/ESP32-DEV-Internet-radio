/**
 * ============================================================
 * @file    FlashAudioPlayer_ESP32dev_noPSRAM.ino
 * @brief   Audio player from internal flash (LittleFS / SPIFFS)
 *          for ESP32 dev board with MAX98357A — no PSRAM required
 * @version 1.0
 * @date    2026-03-10
 *
 * ============================================================
 * DESCRIPTION
 * ============================================================
 *
 *  Plays an audio file stored in the ESP32 internal flash
 *  filesystem (LittleFS or SPIFFS) via I2S. No SD card required.
 *
 *  The audio file is uploaded to the flash partition using the
 *  Arduino IDE LittleFS (or SPIFFS) data upload plugin and
 *  persists across reboots.
 *
 *  Use #define USE_LITTLEFS (default) or comment it out to
 *  switch to SPIFFS. Both filesystems are otherwise identical
 *  from the audio library's perspective.
 *
 *  Supported formats : WAV (16-bit PCM), MP3.
 *
 *  WAV files must be 16-bit PCM. Files in 24-bit or 32-bit
 *  float will be rejected by the decoder with no audio output.
 *  To convert any WAV file to the correct format, use ffmpeg :
 *
 *    ffmpeg -i input.wav -acodec pcm_s16le -ar 44100 -ac 2 output.wav
 *
 * ============================================================
 * FLASH PARTITION CAPACITY
 * ============================================================
 *
 *  Partition scheme : Huge APP (3MB No OTA / 1MB SPIFFS)
 *    → Program flash  : ~3 MB
 *    → Data partition : ~1.5 MB  (LittleFS or SPIFFS)
 *
 *  Approximate audio capacity at 1.5 MB :
 *    WAV 16-bit 48000 Hz mono  : ~13 seconds
 *    WAV 16-bit 44100 Hz stereo : ~8 seconds
 *    MP3 64 kbps               : ~3 minutes
 *
 *  Example file used in this sketch :
 *    bossfight.wav — monster roar sound effect
 *    Source  : freesound.org (Creative Commons licence)
 *    Format  : PCM 16-bit, 48000 Hz, mono, 768 kb/s
 *    Duration: 3.18 s  —  Size: 305 KB
 *
 * ============================================================
 * UPLOADING THE AUDIO FILE
 * ============================================================
 *
 *  1. Create a folder named "data" in the sketch directory
 *  2. Copy your audio file into the data/ folder
 *  3. In Arduino IDE : Tools → ESP32 LittleFS Data Upload
 *     (or Tools → ESP32 Sketch Data Upload for SPIFFS)
 *  4. Upload the sketch as usual
 *
 *  The data upload plugin is separate from the sketch upload.
 *  Both steps are required.
 *
 *  Plugin : arduino-littlefs-upload
 *           github.com/earlephilhower/arduino-littlefs-upload
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

// ─────────────────────────────────────────────────────────────
// FILESYSTEM SELECTION
// Comment out USE_LITTLEFS to use SPIFFS instead.
// Both options require the matching data upload plugin.
// ─────────────────────────────────────────────────────────────

#define USE_LITTLEFS

#ifdef USE_LITTLEFS
  #include "LittleFS.h"
  #define FILESYSTEM  LittleFS
  #define FS_LABEL    "LittleFS"
#else
  #include "SPIFFS.h"
  #define FILESYSTEM  SPIFFS
  #define FS_LABEL    "SPIFFS"
#endif

// ─────────────────────────────────────────────────────────────
// PIN DEFINITIONS — I2S amplifier (MAX98357A)
// ─────────────────────────────────────────────────────────────

#define I2S_BCLK      27   ///< Bit clock       → GPIO27
#define I2S_LRC       26   ///< Left/right clock → GPIO26
#define I2S_DOUT      25   ///< Data out         → GPIO25

// ─────────────────────────────────────────────────────────────
// CONFIGURATION
// ─────────────────────────────────────────────────────────────

// File to play — must be uploaded to the data/ folder first.
// Supported formats : WAV (16-bit PCM only), MP3.
#define AUDIO_FILE    "/bossfight.wav"

// Volume : 0 (mute) to 21 (maximum)
#define VOLUME         12

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
    Serial.printf("\n=== Flash Audio Player (%s) — ESP32 dev (no PSRAM) ===\n", FS_LABEL);

    if (!FILESYSTEM.begin()) {
        Serial.printf("[ERROR] %s mount failed — did you upload the data folder?\n", FS_LABEL);
        return;
    }
    Serial.printf("[%s] Mounted — free heap: %d bytes\n", FS_LABEL, ESP.getFreeHeap());

    // Verify the file exists before trying to play it
    if (!FILESYSTEM.exists(AUDIO_FILE)) {
        Serial.printf("[ERROR] File not found: %s\n", AUDIO_FILE);
        Serial.println("[ERROR] Upload the data/ folder via Tools → ESP32 LittleFS Data Upload");
        return;
    }
    Serial.printf("[%s] File found: %s\n", FS_LABEL, AUDIO_FILE);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(VOLUME);

    Serial.printf("[Audio] Opening: %s\n", AUDIO_FILE);
    audio.connecttoFS(FILESYSTEM, AUDIO_FILE);
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
