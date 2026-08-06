#include <Arduino.h>
#include <time.h>

#include "display.h"
#include "widget_clock.h"
#include "wifi_manager.h"

extern Adafruit_ST7789 tft;

static bool timeConfigured = false;
static unsigned long lastNtpAttemptMs = 0;
static int lastDisplayedMinute = -1;

static const unsigned long NTP_RETRY_MS = 30000;
static const char *NTP_1 = "pool.ntp.org";
static const char *NTP_2 = "time.nist.gov";
static const char *TZ_IRELAND = "GMT0IST,M3.5.0/1,M10.5.0";

static bool clockHasValidTime()
{
    struct tm tmNow;
    if (!getLocalTime(&tmNow, 10)) {
        return false;
    }

    // Unix epoch year baseline check; below this is likely unsynced time.
    return tmNow.tm_year >= (2024 - 1900);
}

static void clockConfigureTimeIfNeeded()
{
    if (timeConfigured) {
        return;
    }

    if (!wifiConnected()) {
        return;
    }

    configTzTime(TZ_IRELAND, NTP_1, NTP_2);
    timeConfigured = true;
    lastNtpAttemptMs = millis();
}

static void clockDrawContent()
{
    struct tm tmNow;
    bool ok = getLocalTime(&tmNow, 10);

    tft.fillRect(0, 100, 240, 220, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);

    if (!ok || !clockHasValidTime()) {
        tft.setTextSize(3);
        tft.setCursor(28, 170);
        tft.setTextColor(ST77XX_RED);
        tft.print("DONT PANIC!");

        tft.setTextSize(1);
        tft.setCursor(60, 260);
        tft.setTextColor(ST77XX_WHITE);
        tft.print("So long and thanks");
        tft.setCursor(70, 270);
        tft.print("for all the fish");


        return;
    }

    char timeBuf[16];
    char dateBuf[24];
    int16_t x1, y1;
    uint16_t w, h;

    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &tmNow);
    strftime(dateBuf, sizeof(dateBuf), "%a %d %b %Y", &tmNow);

    tft.setTextSize(4);
    tft.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((240 - w) / 2, 150);
    tft.print(timeBuf);

    tft.setTextSize(2);
    tft.getTextBounds(dateBuf, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((240 - w) / 2, 205);
    tft.print(dateBuf);
}

void clockWidgetInit()
{
    clockConfigureTimeIfNeeded();
    lastDisplayedMinute = -1;
    clockWidgetDraw();
}

void clockWidgetLoop()
{
    struct tm tmNow;
    bool hasTime = getLocalTime(&tmNow, 10) && clockHasValidTime();

    // Retry NTP config periodically until a valid clock is available.
    if ((!timeConfigured || !hasTime) && (millis() - lastNtpAttemptMs > NTP_RETRY_MS)) {
        timeConfigured = false;
        clockConfigureTimeIfNeeded();
    }

    if (hasTime && tmNow.tm_min != lastDisplayedMinute) {
        lastDisplayedMinute = tmNow.tm_min;
        clockWidgetDraw();
    }
}

void clockWidgetDraw()
{
    clockConfigureTimeIfNeeded();
    clockDrawContent();
}