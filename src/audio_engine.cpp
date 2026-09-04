
#include "audio_engine.h"
#include "project_config.h"
#include "pins.h"
#include "widget_audio.h"
#include "audio_buffer_config.h"
#include "Audio.h"
#include <Preferences.h>

Audio audio;

static uint8_t currentVolume = DEFAULT_VOLUME;
static uint8_t previousVolume = DEFAULT_VOLUME;

static bool startupMuteActive = false;
static bool muted = false;

static unsigned long startupMuteStartMs = 0;
static uint8_t startupVolume = DEFAULT_VOLUME;

static const unsigned long STARTUP_MUTE_MS = 5000;

// ============================================================
// AUDIO INITIALISATION
// ============================================================

bool audioInit()
{
    Serial.println("Initialising Audio...");

    // --------------------------------------------------------
    // Audio buffer configuration
    // --------------------------------------------------------

    audioBufferConfigInit();

    Preferences preferences;
    preferences.begin("radio", true);
    startupVolume = preferences.getUChar("volume", DEFAULT_VOLUME);
    preferences.end();
    if (startupVolume > MAX_VOLUME)
        startupVolume = DEFAULT_VOLUME;

    audio.setBufsize(
        audioBufferRamBytes(),
        audioBufferPsramBytes()
    );

    // --------------------------------------------------------
    // PCM5102A I2S
    //
    // BCLK  = GPIO 16
    // LRCLK = GPIO 15
    // DOUT  = GPIO 17
    // --------------------------------------------------------

    audio.setPinout(
        I2S_BCLK,
        I2S_LRC,
        I2S_DOUT
    );

    Serial.println("[AUDIO] I2S configured:");
    Serial.println("[AUDIO] BCLK  = GPIO 16");
    Serial.println("[AUDIO] LRCLK = GPIO 15");
    Serial.println("[AUDIO] DOUT  = GPIO 17");

    // --------------------------------------------------------
    // Startup mute
    // --------------------------------------------------------

    currentVolume = 0;
    previousVolume = startupVolume;

    startupMuteActive = true;
    muted = true;

    startupMuteStartMs = millis();

    audio.setVolume(0);

    Serial.println("[AUDIO] Startup mute active");

    return true;
}

// ============================================================
// AUDIO LOOP
// ============================================================

void audioLoop()
{
    // --------------------------------------------------------
    // Release startup mute after 5 seconds
    // --------------------------------------------------------

    if (startupMuteActive &&
        millis() - startupMuteStartMs >= STARTUP_MUTE_MS)
    {
        startupMuteActive = false;

        audioSetVolume(startupVolume);

        Serial.println("[AUDIO] Startup mute released");
    }

    // --------------------------------------------------------
    // Keep audio engine running
    // --------------------------------------------------------

    audio.loop();
}

// ============================================================
// CONNECT TO STREAM
// ============================================================

bool audioConnect(const char *url)
{
    Serial.print("Connecting to ");
    Serial.println(url);

    return audio.connecttohost(url);
}

// ============================================================
// STOP AUDIO
// ============================================================

void audioStop()
{
    audio.stopSong();
}

// ============================================================
// SET VOLUME
// ============================================================

void audioSetVolume(uint8_t v)
{
    if (v > MAX_VOLUME)
        v = MAX_VOLUME;

    startupMuteActive = false;

    currentVolume = v;

    muted = (v == 0);

    audio.setVolume(v);

    Preferences preferences;
    preferences.begin("radio", false);
    preferences.putUChar("volume", v);
    preferences.end();
}

// ============================================================
// GET VOLUME
// ============================================================

uint8_t audioGetVolume()
{
    return currentVolume;
}

// ============================================================
// TOGGLE MUTE
// ============================================================

void audioToggleMute()
{
    if (muted)
    {
        uint8_t restoreVolume =
            (previousVolume > 0)
                ? previousVolume
                : DEFAULT_VOLUME;

        audioSetVolume(restoreVolume);
    }
    else
    {
        previousVolume =
            (currentVolume > 0)
                ? currentVolume
                : previousVolume;

        audioSetVolume(0);
    }

    audioWidgetUpdateVolume();
}