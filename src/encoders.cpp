/******************************************************************************
 *
 * encoders.cpp
 *
 ******************************************************************************/

#include <Arduino.h>

#include "pins.h"
#include "encoders.h"
#include "station_manager.h"
#include "audio_engine.h"
#include "widget_manager.h"
#include "widget_audio.h"
#include "widget_favourites.h"

static int lastStationCLK;
static int lastVolumeCLK;

static bool lastStationButton = HIGH;
static bool lastMuteButton = HIGH;
static bool muteButtonActiveLow = true;
static unsigned long lastStationPress = 0;
static unsigned long lastMutePress = 0;
static unsigned long favouritesEnterTime = 0;
static bool favouritesActive = false;

const unsigned long BUTTON_DEBOUNCE = 200;

void encodersInit()
{
    // Volume encoder
    pinMode(ENC1_CLK, INPUT_PULLUP);
    pinMode(ENC1_DT, INPUT_PULLUP);
    pinMode(ENC1_SW, INPUT_PULLUP);

    // Widget / Station encoder
    pinMode(ENC2_CLK, INPUT_PULLUP);
    pinMode(ENC2_DT, INPUT_PULLUP);
    pinMode(ENC2_SW, INPUT_PULLUP);

    lastVolumeCLK = digitalRead(ENC1_CLK);
    lastStationCLK = digitalRead(ENC2_CLK);

    int initialMuteState = digitalRead(ENC1_SW);
    muteButtonActiveLow = (initialMuteState == HIGH);
    lastMuteButton = initialMuteState;
}

void encodersLoop()
{
    // ==================================================
    // Volume Encoder
    // ==================================================

    int vclk = digitalRead(ENC1_CLK);

    if (vclk != lastVolumeCLK)
    {
        if (vclk == LOW)
        {
            uint8_t volume = audioGetVolume();

            if (digitalRead(ENC1_DT) != vclk)
            {
                if (volume < 21)
                    volume++;
            }
            else
            {
                if (volume > 0)
                    volume--;
            }

            audioSetVolume(volume);

            audioWidgetUpdateVolume();

            Serial.print("Volume: ");
            Serial.println(volume);
        }

        lastVolumeCLK = vclk;
    }

    // ==================================================
    // Widget Selector Encoder
    // ==================================================

    int clk = digitalRead(ENC2_CLK);

    if (clk != lastStationCLK)
    {
        if (clk == LOW)
        {
            if (digitalRead(ENC2_DT) != clk)
            {
                if (favouritesActive || currentWidget == WIDGET_SYSTEM)
                {
                    widgetUserActivity();
                    favWidgetRotate(1);
                }
                else
                {
                    widgetUserActivity();
                    stationNext();
                    stationConnectCurrent();
                }
            }
            else
            {
                if (favouritesActive || currentWidget == WIDGET_SYSTEM)
                {
                    widgetUserActivity();
                    favWidgetRotate(-1);
                }
                else
                {
                    widgetUserActivity();
                    stationPrevious();
                    stationConnectCurrent();
                }
            }
        }

        lastStationCLK = clk;
    }

    // ==================================================
    // Station Change Button
    // ==================================================

    bool stationButton = digitalRead(ENC2_SW);
    unsigned long now = millis();

    if (lastStationButton == HIGH && stationButton == LOW && now - lastStationPress > BUTTON_DEBOUNCE)
    {
        lastStationPress = now;
        widgetUserActivity();
    }

    if (lastStationButton == LOW && stationButton == HIGH && now - lastStationPress > BUTTON_DEBOUNCE)
    {
        unsigned long held = now - lastStationPress;

        if (held >= 3000)
        {
            favouritesActive = true;
            currentWidget = WIDGET_SYSTEM;
            widgetDraw();
            widgetUserActivity();
            favouritesEnterTime = now;
        }
        else if (held >= 0)
        {
            if (favouritesActive || currentWidget == WIDGET_SYSTEM)
            {
                widgetUserActivity();
                favWidgetSelect();
                favouritesActive = false;
                currentWidget = WIDGET_AUDIO;
                widgetDraw();
            }
            else
            {
                widgetUserActivity();
                widgetNext();
            }
        }
    }

    if (favouritesActive && now - favouritesEnterTime > 10000)
    {
        favouritesActive = false;
        currentWidget = WIDGET_AUDIO;
        widgetDraw();
        widgetUserActivity();
    }

    lastStationButton = stationButton;

    // ==================================================
    // Mute Button
    // ==================================================

    bool muteButton = digitalRead(ENC1_SW);
    bool mutePressed = false;

    if (muteButton != lastMuteButton && millis() - lastMutePress > BUTTON_DEBOUNCE)
    {
        if (muteButtonActiveLow)
        {
            mutePressed = (muteButton == LOW && lastMuteButton == HIGH);
        }
        else
        {
            mutePressed = (muteButton == HIGH && lastMuteButton == LOW);
        }

        if (mutePressed)
        {
            lastMutePress = millis();
            Serial.println("Mute");
            audioToggleMute();
        }
    }

    lastMuteButton = muteButton;
}