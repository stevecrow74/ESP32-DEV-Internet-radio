#include <Arduino.h>
#include "audio_state.h"
#include "display.h"

void audio_showstation(const char *info)
{
    currentStation = info;
    audioChanged = true;
    updateStation(currentStation);
    Serial.print("Station: ");
    Serial.println(info);
}

void audio_showstreamtitle(const char *info)
{
    currentTitle = info;
    audioChanged = true;
    updateTitle(currentTitle);
    Serial.print("Title: ");
    Serial.println(info);
}

void audio_bitrate(const char *info)
{
    currentBitrate = info;
    audioChanged = true;

    Serial.print("Bitrate: ");
    Serial.println(info);
}

void audio_eof_stream(const char *)
{
    Serial.println("Stream ended");
}

void audio_info(const char *info)
{
    Serial.println(info);
}