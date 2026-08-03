/**
 * ============================================================
 * @file    webradio_esp32dev.ino
 * @brief   Internet radio receiver for ESP32 dev board + MAX98357A
 * @version 2.0
 * @date    2026-02-15
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
 *              Coexistence with other versions in Arduino libraries:
 *                - ESP32-audioI2S-nopsram (this fork, v1.0.0)
 *                  → ESP32 dev, no PSRAM
 *                - ESP32-audioI2S (schreibfaul1 upstream)
 *                  → boards with PSRAM, requires PSRAM
 *                Renaming Audio.h → Audio_nopsram.h disambiguates #include.
 *
 *  ⚠ DO NOT replace this library with schreibfaul1/ESP32-audioI2S.
 *    v3.x of the original requires PSRAM and will crash with OOM on this board.
 *    To receive updates from this fork, download the latest ZIP from :
 *      https://github.com/PLSousa/ESP32-audioI2S
 *
 *  Arduino IDE board settings (Tools menu):
 *    Board           : ESP32 Dev Module
 *    PSRAM           : Disabled
 *    Partition Scheme: Huge APP (3MB No OTA / 1MB SPIFFS)
 *    CPU Frequency   : 240 MHz   ← important for AAC decoding
 *
 * ============================================================
 * NO-PSRAM — v2.0.6 BEHAVIOUR
 * ============================================================
 *
 *  Unlike v3.x, ESP32-audioI2S v2.0.6 still supports boards
 *  without PSRAM. When no PSRAM is detected it automatically
 *  falls back to a smaller internal SRAM buffer (~6 KB input
 *  buffer) instead of the 704 KB PSRAM allocation that caused
 *  OOM crashes on v3.x.
 *
 *  Remaining mitigations kept in this sketch:
 *
 *  1. WiFi.setSleep(false)
 *     Prevents 100–300 ms latency spikes on the TCP receive
 *     path that would drain the small internal buffer faster
 *     than it can refill → dropouts.
 *
 *  2. Pre-fill delay (PREBUFFER_MS)
 *     Gives audio.loop() time to fill the buffer before the
 *     main loop takes over. Keeps audio.loop() running tight
 *     (no delay()) so the HTTP handshake completes quickly.
 *
 *  3. CPU at 240 MHz
 *     AAC decoding headroom — prevents the decoder from
 *     starving the WiFi/TCP task.
 *
 * ============================================================
 * BOOT SEQUENCE (same rationale as XIAO ESP32-S3 version)
 * ============================================================
 *
 *  WiFi reports WL_CONNECTED before the lwIP TCP/IP stack is
 *  fully ready. Calling audio.connecttohost() at that moment
 *  causes a silent HTTP failure with no audio output.
 *  A TCP probe to the streaming server is used to confirm the
 *  network stack is operational before starting the stream.
 *
 *  1. Set CPU to 240 MHz
 *  2. Configure I2S pins & volume
 *  3. Connect to WiFi + disable modem sleep
 *  4. TCP probe to streaming server (retry until success)
 *  5. Call connecttohost() then pre-fill buffer (PREBUFFER_MS)
 *  6. Enter main loop — audio.loop() runs at full speed
 *
 * ============================================================
 */
#include "Arduino.h"
#include "WiFi.h"
#include "Audio_nopsram.h"  // ESP32-audioI2S-nopsram v2.0.6+GCC14 patches (schreibfaul1)
                            // renamed from Audio.h to avoid conflict with
                            // ESP32-audioI2S (schreibfaul1 upstream, requires PSRAM)
                            // ⚠ DO NOT replace with schreibfaul1/ESP32-audioI2S — see SOFTWARE section above
#include "esp_system.h"     // esp_reset_reason()
#include "lwip/sockets.h"   // POSIX socket API (lwIP)
#include "lwip/netdb.h"     // getaddrinfo()

// ─────────────────────────────────────────────────────────────
// USER CONFIGURATION
// ─────────────────────────────────────────────────────────────

static const char* WIFI_SSID = "MY_SSID";
static const char* WIFI_PASSWORD = "MY_PWD";

const char* STREAM_URL    = "http://icecast.radiofrance.fr/fip-hifi.aac";
const char* STREAM_HOST   = "icecast.radiofrance.fr";
const int   STREAM_PORT   = 80;

// ─────────────────────────────────────────────────────────────
// I2S PIN MAPPING
// ─────────────────────────────────────────────────────────────

#define I2S_BCLK   27   ///< Bit clock       → GPIO27
#define I2S_LRC    26   ///< Left/right clock → GPIO26
#define I2S_DOUT   25   ///< Data out         → GPIO25

// ─────────────────────────────────────────────────────────────
// AUDIO / TIMING SETTINGS
// ─────────────────────────────────────────────────────────────

#define VOLUME               6   ///< 0 (mute) – 21 (max)

/**
 * Pre-buffer delay (ms) after connecttohost().
 *
 * Without PSRAM the internal ring buffer is small. We give the
 * library this many milliseconds to download and buffer audio
 * data before the main loop starts consuming it.
 * During this time audio.loop() is called every 20 ms so the
 * network task keeps running, but the decoder has not yet
 * started outputting samples.
 * Increase to 4000–5000 if you still hear glitches at startup.
 */
#define PREBUFFER_MS        3000

/**
 * HTTP connection timeout (ms).
 * Used only for the TCP probe — the audio lib v2.x manages
 * its own internal timeout automatically.
 */
#define CONNECTION_TIMEOUT  5000

// ─────────────────────────────────────────────────────────────
// TIMING CONSTANTS
// ─────────────────────────────────────────────────────────────

#define WIFI_TIMEOUT_MS      15000
#define RECONNECT_DELAY_MS    5000
#define TCP_PROBE_TIMEOUT_MS  8000

// ─────────────────────────────────────────────────────────────
// GLOBALS
// ─────────────────────────────────────────────────────────────

Audio audio;

// ─────────────────────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────────────────────

bool connectWiFi() {
  Serial.printf("\n[WiFi] Connecting to \"%s\" ", WIFI_SSID);
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println("\n[WiFi] ✗ Timeout!");
      return false;
    }
    delay(300);
    Serial.print(".");
  }

  /**
   * Disable WiFi modem sleep.
   *
   * By default the ESP32 WiFi stack uses power-saving sleep
   * between packets. This introduces latency spikes of 100–300 ms
   * on the TCP receive path, which causes the audio library's
   * internal buffer to drain faster than it refills → dropouts.
   * Disabling sleep keeps the radio always-on at the cost of
   * ~20–30 mA extra current — acceptable for a mains-powered device.
   */
  WiFi.setSleep(false);

  Serial.printf("\n[WiFi] ✓ Connected  IP: %s  RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  Serial.printf("[SYS]  Free heap: %d bytes\n", ESP.getFreeHeap());
  return true;
}

// ─────────────────────────────────────────────────────────────
// TCP PROBE
// ─────────────────────────────────────────────────────────────

/**
 * @brief  Confirm the lwIP TCP/IP stack is fully ready.
 *
 * Attempts a real TCP connection to STREAM_HOST:STREAM_PORT.
 * Retries every 500 ms until success or TCP_PROBE_TIMEOUT_MS.
 *
 * @return true if TCP handshake succeeded, false on timeout.
 */
bool waitTCPReady() {
  Serial.printf("[TCP]  Probing %s:%d ", STREAM_HOST, STREAM_PORT);

  unsigned long start = millis();

  while (millis() - start < TCP_PROBE_TIMEOUT_MS) {

    struct addrinfo hints = {};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", STREAM_PORT);

    if (getaddrinfo(STREAM_HOST, port_str, &hints, &res) != 0 || !res) {
      Serial.print(".");
      delay(500);
      continue;
    }

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
      freeaddrinfo(res);
      Serial.print(".");
      delay(500);
      continue;
    }

    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    int conn = connect(sock, res->ai_addr, res->ai_addrlen);
    close(sock);
    freeaddrinfo(res);

    if (conn == 0) {
      Serial.println(" ✓ TCP ready");
      return true;
    }

    Serial.print(".");
    delay(500);
  }

  Serial.println(" ✗ TCP probe timeout");
  return false;
}

// ─────────────────────────────────────────────────────────────
// STREAM CONTROL
// ─────────────────────────────────────────────────────────────

/**
 * @brief  Open the stream and wait for the buffer to pre-fill.
 *
 * Calls connecttohost() then runs a reduced-rate audio.loop()
 * for PREBUFFER_MS milliseconds. This lets the library download
 * enough data into its internal ring buffer before full-speed
 * decoding begins, preventing startup glitches on no-PSRAM boards.
 */
void startStream() {
  Serial.printf("[Audio] Free heap before connect: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("[Audio] Opening stream: %s\n", STREAM_URL);
  audio.connecttohost(STREAM_URL);

  // Pre-fill: call audio.loop() as fast as possible for PREBUFFER_MS.
  // NO delay() here — the HTTP handshake and initial buffer fill
  // require audio.loop() to run without interruption.
  Serial.printf("[Audio] Pre-buffering for %d ms...\n", PREBUFFER_MS);
  unsigned long start = millis();
  while (millis() - start < PREBUFFER_MS) {
    audio.loop();
  }
  Serial.printf("[Audio] Pre-buffer done — free heap: %d bytes\n", ESP.getFreeHeap());
}

// ─────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Web Radio ESP32 dev (no PSRAM) ===");

  // 1) Ensure CPU runs at 240 MHz for AAC decoding headroom
  setCpuFrequencyMhz(240);
  Serial.printf("[SYS]  CPU frequency: %d MHz\n", getCpuFrequencyMhz());
  Serial.printf("[SYS]  Free heap at boot: %d bytes\n", ESP.getFreeHeap());

  // 2) Configure I2S and volume
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(VOLUME);

  // 3) Connect to WiFi
  while (!connectWiFi()) {
    Serial.printf("[WiFi] Retrying in %d s...\n", RECONNECT_DELAY_MS / 1000);
    delay(RECONNECT_DELAY_MS);
  }

  // 4) Wait for TCP/IP stack to be fully operational
  while (!waitTCPReady()) {
    Serial.println("[TCP]  Retrying probe in 2 s...");
    delay(2000);
  }

  // 5) Open stream + pre-fill buffer
  startStream();
  Serial.printf("[SYS]  Free heap after stream start: %d bytes\n", ESP.getFreeHeap());
}

// ─────────────────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────────────────

void loop() {
  // Must run as fast as possible — any delay > ~20 ms risks audio gaps
  audio.loop();

  // WiFi watchdog — checked every 5 s
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 5000) {
    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Connection lost — reconnecting...");
      audio.stopSong();

      if (connectWiFi() && waitTCPReady()) {
        delay(200);
        startStream();
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────
// ESP32-audioI2S CALLBACKS
// ─────────────────────────────────────────────────────────────

void audio_info(const char* info)            { Serial.printf("[info]    %s\n", info); }
void audio_id3data(const char* info)         { Serial.printf("[id3]     %s\n", info); }
void audio_showstation(const char* info)     { Serial.printf("[station] %s\n", info); }
void audio_showstreamtitle(const char* info) { Serial.printf("[title]   %s\n", info); }
void audio_bitrate(const char* info)         { Serial.printf("[bitrate] %s\n", info); }
void audio_error(const char* info)           { Serial.printf("[ERROR]   %s\n", info); }

void audio_eof_stream(const char* info) {
  Serial.printf("[eof]     %s — restarting in 2 s...\n", info);
  delay(2000);
  startStream();
}
