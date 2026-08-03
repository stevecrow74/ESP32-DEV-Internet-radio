/******************************************************************************
 *
 * display.cpp
 *
 ******************************************************************************/

#include "display.h"
#include "config.h"
#include "pins.h"
#include "audio_state.h"
#include "wifi_manager.h"

#include <WiFi.h>
#include <time.h>

static bool displayNeedsRefresh = true;
static unsigned long lastStatusRefreshMs = 0;
static bool statusTimeConfigured = false;
static const char *TZ_IRELAND = "GMT0IST,M3.5.0/1,M10.5.0";

static void ensureStatusTimeConfigured()
{
    if (statusTimeConfigured) {
        return;
    }

    if (!wifiConnected()) {
        return;
    }

    configTzTime(TZ_IRELAND, "pool.ntp.org", "time.nist.gov");
    statusTimeConfigured = true;
}

static String statusClockText()
{
    ensureStatusTimeConfigured();

    struct tm tmNow;
    if (!getLocalTime(&tmNow, 10)) {
        return "--:--";
    }

    char buf[8];
    strftime(buf, sizeof(buf), "%H:%M", &tmNow);
    return String(buf);
}

static const char *signalLabelFromRssi(long rssi)
{
    if (rssi >= -55) return "||||";
    if (rssi >= -67) return "|||.";
    if (rssi >= -75) return "||..";
    if (rssi >= -85) return "|...";
    return "....";
}

//------------------------------------------------------
// TFT Object
//------------------------------------------------------

Adafruit_ST7789 tft = Adafruit_ST7789(
    TFT_CS,
    TFT_DC,
    TFT_RST
);

//------------------------------------------------------
// Initialisation
//------------------------------------------------------

void displayInit()
{
    tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);

    tft.setRotation(0);

    tft.fillScreen(COLOUR_BACKGROUND);

    displayRefresh();
}

void displayLoop()
{
    // Keep top status bar live (time + network) once per second.
    if (millis() - lastStatusRefreshMs >= 1000)
    {
        drawStatusBar();
        lastStatusRefreshMs = millis();
    }

    if (displayNeedsRefresh)
    {
     drawStatusBar();
     drawRadioPanel();

    displayNeedsRefresh = false;
    }
}
//------------------------------------------------------
// Draw complete screen
//------------------------------------------------------

void displayRefresh()
{
    displayNeedsRefresh = true;

    drawWidgetArea();
}

//------------------------------------------------------
// Status Bar
//------------------------------------------------------

void drawStatusBar()
{
    tft.fillRect(
        0,
        0,
        SCREEN_WIDTH,
        STATUS_HEIGHT,
        COLOUR_BACKGROUND
    );

    tft.drawFastHLine(
        0,
        STATUS_HEIGHT - 1,
        SCREEN_WIDTH,
        COLOUR_DIVIDER
    );

    tft.setTextSize(1);

    // WiFi
    tft.setTextColor(COLOUR_TEXT);
    tft.setCursor(5,5);
    tft.print(wifiConnected() ? "WiFi" : "NoWiFi");

    // Signal bars
    tft.setTextColor(wifiConnected() ? ST77XX_GREEN : COLOUR_WARNING);
    tft.setCursor(42,5);
    tft.print(wifiConnected() ? signalLabelFromRssi(WiFi.RSSI()) : "....");

    // SSID
    tft.setTextColor(COLOUR_TEXT);
    tft.setCursor(80,5);
    String ssid = wifiConnected() ? WiFi.SSID() : "offline";
    if (ssid.length() > 9) {
        ssid = ssid.substring(0, 9);
    }
    tft.print(ssid);

    // Clock
    tft.setCursor(196,5);
    tft.print(statusClockText());
}
//------------------------------------------------------
// Radio Panel
//------------------------------------------------------

void drawRadioPanel()
{
    // Clear radio panel
    tft.fillRect(
        0,
        STATUS_HEIGHT,
        SCREEN_WIDTH,
        WIDGET_Y - STATUS_HEIGHT,
        COLOUR_BACKGROUND
    );

    // Divider above widget area
    tft.drawFastHLine(
        0,
        WIDGET_Y,
        SCREEN_WIDTH,
        COLOUR_DIVIDER
    );

    int16_t x1, y1;
    uint16_t w, h;

    tft.setTextColor(COLOUR_TEXT);

    //--------------------------------------------------
    // Connecting...
    //--------------------------------------------------

    if (!currentStation.length())
    {
        tft.setTextSize(2);

        String text = "Connecting...";

        tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

        tft.setCursor((SCREEN_WIDTH - w) / 2, 55);
        tft.print(text);

        return;
    }

    //--------------------------------------------------
    // Station
    //--------------------------------------------------

    tft.setTextSize(2);

    tft.getTextBounds(currentStation, 0, 0, &x1, &y1, &w, &h);

    tft.setCursor((SCREEN_WIDTH - w) / 2, 24);
    tft.print(currentStation);

    //--------------------------------------------------
    // Split Artist / Song
    //--------------------------------------------------

    String artist = "";
    String song = "";

    if (currentTitle.length())
    {
        int dash = currentTitle.indexOf(" - ");

        if (dash > 0)
        {
            artist = currentTitle.substring(0, dash);
            song = currentTitle.substring(dash + 3);
        }
        else
        {
            song = currentTitle;
        }
    }

    //--------------------------------------------------
    // Artist
    //--------------------------------------------------

    if (artist.length())
    {
        tft.setTextSize(1);

        tft.getTextBounds(artist, 0, 0, &x1, &y1, &w, &h);

        tft.setCursor((SCREEN_WIDTH - w) / 2, 50);
        tft.print(artist);
    }

    //--------------------------------------------------
    // Song
    //--------------------------------------------------

    if (song.length())
    {
        tft.setTextSize(1);

        tft.getTextBounds(song, 0, 0, &x1, &y1, &w, &h);

        tft.setCursor((SCREEN_WIDTH - w) / 2, 66);
        tft.print(song);
    }
}
//------------------------------------------------------
// Widget Area
//------------------------------------------------------

void drawWidgetArea()
{
    tft.fillRect(
        0,
        WIDGET_Y,
        SCREEN_WIDTH,
        WIDGET_HEIGHT,
        COLOUR_BACKGROUND
    );
}

//------------------------------------------------------
// Dynamic Updates
//------------------------------------------------------

void updateStation(const String &station)
{
    displayNeedsRefresh = true;
}

void updateTitle(const String &title)
{
    displayNeedsRefresh = true;
}

void updateVolume(uint8_t volume)
{
    displayNeedsRefresh = true;
}

void updateClock(const String &timeStr)
{
    displayNeedsRefresh = true;
}