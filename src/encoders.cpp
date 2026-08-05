/******************************************************************************
 *
 * encoders.cpp
 *
 ******************************************************************************/

#include <Arduino.h>

#include "config.h"
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
static unsigned long mutePressStart = 0;
static unsigned long favouritesEnterTime = 0;
static bool favouritesActive = false;

const unsigned long BUTTON_DEBOUNCE = 200;
const unsigned long LONG_PRESS_MS = 3000;
const unsigned long FAVOURITES_TIMEOUT_MS = 10000;

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
    unsigned long now = millis();

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
                if (volume < MAX_VOLUME)
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
                if (favouritesActive)
                {
                    widgetUserActivity();
                    favWidgetRotate(1);
                    favouritesEnterTime = now;
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
                if (favouritesActive)
                {
                    widgetUserActivity();
                    favWidgetRotate(-1);
                    favouritesEnterTime = now;
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

    if (lastStationButton == HIGH && stationButton == LOW && now - lastStationPress > BUTTON_DEBOUNCE)
    {
        lastStationPress = now;
        widgetUserActivity();

        if (favouritesActive)
        {
            favouritesEnterTime = now;
        }
    }

    if (lastStationButton == LOW && stationButton == HIGH && now - lastStationPress > BUTTON_DEBOUNCE)
    {
        unsigned long held = now - lastStationPress;

        if (held >= LONG_PRESS_MS)
        {
            favouritesActive = true;
            favWidgetDraw();
            widgetUserActivity();
            favouritesEnterTime = now;
        }
        else if (held >= 0)
        {
            if (favouritesActive)
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

    if (favouritesActive && now - favouritesEnterTime > FAVOURITES_TIMEOUT_MS)
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
    bool mutePressedNow;

    if (muteButtonActiveLow)
    {
        mutePressedNow = (muteButton == LOW);
    }
    else
    {
        mutePressedNow = (muteButton == HIGH);
    }

    if (muteButton != lastMuteButton && now - lastMutePress > BUTTON_DEBOUNCE)
    {
        bool wasPressed;
        if (muteButtonActiveLow)
        {
            wasPressed = (lastMuteButton == LOW);
        }
        else
        {
            wasPressed = (lastMuteButton == HIGH);
        }

        lastMutePress = now;

        // Button press edge: track start time for long-press handling.
        if (mutePressedNow && !wasPressed)
        {
            mutePressStart = now;
            widgetUserActivity();
        }

        // Button release edge: short press toggles mute, long press opens system widget.
        if (!mutePressedNow && wasPressed)
        {
            unsigned long held = now - mutePressStart;

            if (held >= LONG_PRESS_MS)
            {
                favouritesActive = false;
                currentWidget = WIDGET_SYSTEM;
                widgetDraw();
                widgetHoldSelected();
                widgetUserActivity();
            }
            else
            {
                Serial.println("Mute");
                audioToggleMute();
                widgetUserActivity();
            }
        }
    }

    lastMuteButton = muteButton;
}