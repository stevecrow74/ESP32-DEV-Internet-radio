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
static unsigned long lastStationPress = 0;
static unsigned long lastMutePress = 0;

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
                // Rotate right
                if (currentWidget == WIDGET_SYSTEM) {
                    widgetUserActivity();
                    favWidgetRotate(1);
                } else {
                    // Rotate selects next station
                    widgetUserActivity();
                    stationNext();
                    stationConnectCurrent();
                }
            }
            else
            {
                // Rotate left
                if (currentWidget == WIDGET_SYSTEM) {
                    widgetUserActivity();
                    favWidgetRotate(-1);
                } else {
                    // Rotate selects previous station
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

    // When in favourites widget, handle press/release differently to allow select vs delete
    if (lastStationButton == HIGH && stationButton == LOW && millis() - lastStationPress > BUTTON_DEBOUNCE)
    {
        lastStationPress = millis();

        // For non-favourites, short press cycles widgets
        if (currentWidget != WIDGET_SYSTEM) {
            widgetUserActivity();
            widgetNext();
        }
    }

    // Detect release for favourites long-press detection
    if (lastStationButton == LOW && stationButton == HIGH && millis() - lastStationPress > BUTTON_DEBOUNCE)
    {
        unsigned long held = millis() - lastStationPress;

        if (currentWidget == WIDGET_SYSTEM) {
            widgetUserActivity();
            if (held > 5000) {
                // long press (5s) -> delete
                favWidgetDeleteSelected();
            } else {
                // short press -> select/play
                favWidgetSelect();
            }
        }
    }

    lastStationButton = stationButton;

    // ==================================================
    // Mute Button
    // ==================================================

    bool muteButton = digitalRead(ENC1_SW);

    if (lastMuteButton == HIGH &&
         muteButton == LOW &&
         millis() - lastMutePress > BUTTON_DEBOUNCE)
{
         lastMutePress = millis();

         Serial.println("Mute");

         audioToggleMute();
}

lastMuteButton = muteButton;
}