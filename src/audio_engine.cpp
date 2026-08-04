#include <Arduino.h>
#include "audio_engine.h"
#include "project_config.h"
#include "widget_audio.h"
#include "audio_buffer_config.h"
#include <Audio_nopsram.h>

Audio audio;
static uint8_t currentVolume = DEFAULT_VOLUME;
static uint8_t previousVolume = DEFAULT_VOLUME;
static bool startupMuteActive = false;
static bool muted = false;
static unsigned long startupMuteStartMs = 0;
static const unsigned long STARTUP_MUTE_MS = 5000;

bool audioInit()
{
    Serial.println("Initialising Audio...");

    audioBufferConfigInit();
    audio.setBufsize(audioBufferRamBytes(), audioBufferPsramBytes());
    audio.setPinout(26, 25, 22);      // BCLK, LRCLK, DOUT
    currentVolume = 0;
    previousVolume = DEFAULT_VOLUME;
    startupMuteActive = true;
    muted = true;
    startupMuteStartMs = millis();
    audio.setVolume(0);

    return true;
}

void audioLoop()
{
    if (startupMuteActive && millis() - startupMuteStartMs >= STARTUP_MUTE_MS)
    {
        startupMuteActive = false;
        audioSetVolume(DEFAULT_VOLUME);
    }

    audio.loop();
}

bool audioConnect(const char *url)
{
    Serial.print("Connecting to ");
    Serial.println(url);

    return audio.connecttohost(url);
}

void audioStop()
{
    audio.stopSong();
}

void audioSetVolume(uint8_t v)
{
    if (v > 21)
        v = 21;

    startupMuteActive = false;
    currentVolume = v;
    muted = (v == 0);

    audio.setVolume(v);
}

uint8_t audioGetVolume()
{
    return currentVolume;
}

void audioToggleMute()
{
    if (muted)
    {
        uint8_t restoreVolume = (previousVolume > 0) ? previousVolume : DEFAULT_VOLUME;
        audioSetVolume(restoreVolume);
    }
    else
    {
        previousVolume = (currentVolume > 0) ? currentVolume : previousVolume;
        audioSetVolume(0);
    }

    audioWidgetUpdateVolume();
}