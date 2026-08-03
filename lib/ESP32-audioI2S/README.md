# ESP32-audioI2S-nopsram

This is a fork of the excellent [ESP32-audioI2S library by schreibfaul1](https://github.com/schreibfaul1/ESP32-audioI2S), maintained here for ESP32 boards without PSRAM.

## Why this fork ?

Starting with v3.0.0, the original library requires PSRAM and allocates a 704 KB audio buffer at boot. On ESP32 dev boards without PSRAM, this causes an immediate out-of-memory crash. v2.0.6 is the last version that supports no-PSRAM boards, but it does not compile with the ESP32 board package v3.x, which ships with the xtensa-esp32-elf GCC 14 toolchain. GCC 14 enforces stricter type checking that exposes several incompatibilities in the original source.

The goal of this fork is to allow makers who own ESP32 boards without PSRAM to keep using them for simple audio projects — internet radio, audio notifications, basic streaming — without having to downgrade their Arduino IDE or their ESP32 board package. These boards are perfectly capable hardware for many practical use cases, and there is no good reason to abandon them just because the upstream library moved on.

This fork applies 9 targeted patches to make v2.0.6 compile and run correctly with ESP32 board package v3.x and GCC 14, and to backport selected stability fixes from v3.2.1. It is versioned independently from the upstream library, starting at v1.0.0. Compatibility has been validated with board package v3.3.7 and Arduino IDE 2.3.8. We will do our best to maintain this compatibility as new versions of the board package are released, but we cannot guarantee it indefinitely — if a future board package version introduces breaking changes that cannot be easily patched, this will be documented here.

## Patches applied

Patch 1 — Audio_nopsram.cpp : fixed a min() type mismatch between uint32_t and size_t that GCC 14 rejects as an ambiguous overload.

Patch 2 — aac_decoder.cpp : changed int val to int32_t val in 5 functions (UnpackQuads, UnpackPairsNoEsc, UnpackPairsEsc, DecodeOneScaleFactor, DecodeOneSymbol) to match the int32_t* pointer type expected by DecodeHuffmanScalar.

Patch 3 — aac_decoder.cpp : fixed a cast incompatibility — (uint32_t*)last replaced by (unsigned int*)last to resolve a C++ name mangling conflict under GCC 14.

Patch 4 — aac_decoder.cpp : corrected the DecodeHuffmanScalar function definition signature — uint32_t bitBuf replaced by unsigned int bitBuf to align declaration and definition and eliminate the linker error.

The following patches (5–9) are stability fixes backported from v3.2.1 into Audio_nopsram.cpp.

Patch 5 — parseHttpResponseHeader() : increased the vTaskDelay from 3 to 5 ticks to give the TCP stack slightly more time during HTTP header parsing, reducing the risk of incomplete reads on slow connections.

Patch 6 — parseHttpResponseHeader() : fixed an inverted log condition that suppressed the "chunked data transfer" info message. The message now appears correctly when logging is enabled.

Patch 7 — parseHttpResponseHeader() : fixed the same inverted log condition for the "icy-name" metadata field.

Patch 8 — findNextSync() MP3 : added a return statement when no MP3 sync word is found within the search window, preventing an infinite loop that could occur with malformed or unexpected stream data.

Patch 9 — sendBytes() : added handling for ID3 tags injected mid-stream in chunked MP3 transfers, skipping them cleanly instead of passing them to the decoder and causing a decode error.

## Header rename

To allow coexistence with other versions of ESP32-audioI2S in the same Arduino libraries folder, Audio.h and Audio.cpp have been renamed to Audio_nopsram.h and Audio_nopsram.cpp. All internal #include references have been updated accordingly. In your sketch, use #include "Audio_nopsram.h" instead of #include "Audio.h". This makes it possible to install this fork alongside the original library or its v3.x versions without any conflict.

## Installation

Download or clone this repository and place the folder in your Arduino libraries directory (or use Sketch > Include Library > Add .ZIP Library in Arduino IDE). In your sketch, use #include "Audio_nopsram.h" instead of #include "Audio.h". Select your ESP32 dev board with PSRAM disabled and partition scheme set to Huge APP (3MB No OTA / 1MB SPIFFS). Set CPU frequency to 240 MHz for reliable AAC decoding.

Do not replace this library with schreibfaul1/ESP32-audioI2S via the Arduino IDE Library Manager. The two libraries have different names so the IDE will not confuse them, but installing the upstream v3.x version alongside this fork and changing your #include will break compilation on no-PSRAM boards.

## Tested environment

Arduino IDE : 2.3.8. ESP32 board package (Espressif) : 3.3.7. Compiler : xtensa-esp32-elf-g++ (esp-x32 toolchain, build 2511). esptool : 5.1.0. All system libraries (WiFi, SPI, NetworkClientSecure, FS, SPIFFS, FFat, SD, SD_MMC) are included in the board package v3.3.7 and require no separate installation.

## Arduino IDE board settings

Board : ESP32 Dev Module. PSRAM : Disabled. Partition Scheme : Huge APP (3MB No OTA / 1MB SPIFFS). CPU Frequency : 240 MHz.

## Validated examples

All examples have been tested and validated on hardware (ESP32 dev board without PSRAM, MAX98357A I2S amplifier, ESP32 board package v3.3.7, GCC 14). Only verified examples are published in this repository.

WiFi_Radio_ESP32dev_noPSRAM : a complete internet radio sketch. Streams AAC and MP3 stations over WiFi. Includes a TCP probe mechanism to ensure the lwIP stack is fully ready before connecting to the stream, a WiFi watchdog for automatic reconnection, and a pre-fill buffer phase to prevent dropouts on startup.

AudioPlayer_SD_ESP32dev_noPSRAM : plays WAV (16-bit PCM) and MP3 files from a microSD card via SPI. No WiFi required. The audio file is specified by name in the sketch and played once at boot. Includes SD wiring and ffmpeg conversion instructions for WAV files that are not 16-bit PCM. Note on MAX98357A wiring : the SD/SHUTDOWN pin is left unconnected in the example, which works correctly on genuine Adafruit modules where it defaults to enabled. On some third-party clones this pin defaults to muted when left floating, resulting in complete silence despite the decoder running normally. If you get no audio output at all, wire the SD/SHUTDOWN pin to 3.3V as a first diagnostic step.

FlashAudioPlayer_ESP32dev_noPSRAM : plays an audio file stored in the ESP32 internal flash using LittleFS or SPIFFS. No SD card, no WiFi required. The file is uploaded to the data/ folder via the arduino-littlefs-upload plugin. A #define switches between LittleFS (default) and SPIFFS. The example ships with a short WAV sound effect in the data/ folder so it can be used immediately after cloning. Useful for audio notifications, sound effects, and any application where a small audio clip needs to play without external hardware.

GoogleTTS_ESP32dev_noPSRAM : demonstrates Google Text-to-Speech via connecttospeech(). Sends a text string to the Google TTS API over HTTPS and plays the MP3 response directly through the I2S amplifier. This example also illustrates the API difference between v2.0.6 and v3.x : the upstream v3.x example uses Audio::msg_t and audio_info_callback, which do not exist in v2.0.6. This fork uses the v2.x weak callback functions (audio_info, audio_eof_speech) instead. The example text is a line from Molière's L'Avare (1668), public domain.

WeatherTTS_ESP32dev_noPSRAM_FR : fetches current weather conditions from the Open-Meteo API (free, no API key required) and reads them aloud in French via Google TTS. Retrieves temperature and WMO weather code, builds a natural language sentence, and speaks it at boot. Includes a TCP probe to translate.google.com:443 between the HTTP weather fetch and the HTTPS TTS request, which is required to let the lwIP stack stabilise between two consecutive network connections. Automatic retry on HTTP failure. Example output : "À Strasbourg, il fait 8 degrés. Ciel couvert." Requires ArduinoJson by Benoit Blanchon.

## Hardware tested

MCU : ESP32 dev board (no PSRAM), 240 MHz. Amplifier : MAX98357A I2S Class-D module.

## No-PSRAM behaviour

Unlike v3.x, ESP32-audioI2S v2.0.6 supports boards without PSRAM. When no PSRAM is detected it automatically falls back to a smaller internal SRAM buffer (approximately 6 KB input buffer) instead of the 704 KB PSRAM allocation that caused out-of-memory crashes on v3.x. To get reliable audio streaming with this small buffer, it is important to disable WiFi modem sleep (WiFi.setSleep(false)), set CPU frequency to 240 MHz for AAC decoding headroom, and allow a pre-fill phase after connecttohost() before the main loop takes over.

## Codec support

v2.0.6 includes decoders for MP3, AAC, AAC+ (HE-AAC), WAV, FLAC, and M4A. VORBIS and OPUS decoders were added in v3.x and are not available in this fork. The original library documents AAC+ on plain ESP32 as mono only, with full stereo (SBR, Parametric Stereo) requiring an ESP32-S3 or ESP32-P4.

The limiting factor for no-PSRAM boards is not the codec itself but the input buffer size. Without PSRAM the buffer is approximately 6 KB, compared to 300 KB with PSRAM. This is sufficient for AAC and MP3 streams at typical internet radio bitrates (96–192 kbps). Higher bitrates or codecs with large frame sizes (FLAC in particular, with blocks up to 24 KB) may cause buffer underruns. Validated codecs on this fork are AAC and MP3. WAV has been tested and works. FLAC and M4A are present in the decoder but have not been tested on no-PSRAM hardware.

## Troubleshooting

**No audio output, decoder appears to start normally (stream ready, syncword found in Serial monitor)**

The SD/SHUTDOWN pin on the MAX98357A may be floating. On genuine Adafruit modules this pin defaults to enabled when unconnected. On some third-party clones it defaults to muted, causing complete silence even though the I2S signal is present and the decoder is running correctly. Wire the SD/SHUTDOWN pin to 3.3V to enable the amplifier unconditionally. The GAIN pin can remain unconnected (defaults to approximately 9 dB when tied to GND, or 15 dB when floating — behaviour varies by board).

**No audio output from a WAV file (decoder reports "BitsPerSample is 24, must be 8 or 16")**

The WAV decoder in v2.0.6 only supports 8-bit and 16-bit PCM. Files encoded in 24-bit or 32-bit will be rejected immediately with no audio output. Convert the file to 16-bit PCM before copying it to the SD card :

```
ffmpeg -i input.wav -acodec pcm_s16le -ar 44100 -ac 2 output.wav
```


## Credits

Original library by [schreibfaul1](https://github.com/schreibfaul1/ESP32-audioI2S) — all audio decoding and streaming logic is his work. This fork only adds GCC 14 compatibility patches for no-PSRAM boards and contributes validated examples for this hardware configuration.
