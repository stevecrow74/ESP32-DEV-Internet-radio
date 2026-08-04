#include <Arduino.h>
#include <WiFi.h>
#include "display.h"
#include "widget_system.h"
#include "theme.h"
#include "wifi_manager.h"

extern Adafruit_ST7789 tft;

void systemWidgetInit()
{
    // nothing to initialize
}

void systemWidgetLoop()
{
    // nothing dynamic for now
}

void systemWidgetDraw()
{
    tft.fillRect(0, 100, 240, 220, COLOR_BACKGROUND);
    tft.fillRect(0, 100, 240, 28, COLOR_HEADER);

    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.setCursor(10, 108);
    tft.print("System");

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);

    const int centerX = 120;
    const int textY = 142;
    const char *lines[] = {"Internet radio", "by", "stevecrow74"};

    for (int i = 0; i < 3; ++i)
    {
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);
        int16_t x = centerX - (w / 2);
        tft.setCursor(x, textY + (i * 14));
        tft.print(lines[i]);
    }

    tft.setCursor(10, 196);
    tft.setTextColor(COLOR_LABEL);
    tft.print("SSID:");
    tft.setCursor(50, 196);
    tft.print(wifiConnected() ? WiFi.SSID() : "offline");

    tft.setCursor(10, 208);
    tft.print("IP:");
    tft.setCursor(50, 208);
    tft.print(wifiConnected() ? WiFi.localIP().toString() : "not connected");

    tft.setCursor(10, 220);
    tft.print("RSSI:");
    tft.setCursor(50, 220);
    tft.print(wifiConnected() ? String(WiFi.RSSI()) + " dBm" : "n/a");

    tft.setCursor(10, 232);
    tft.print("Temp:");
    tft.setCursor(50, 232);
    tft.print(String(temperatureRead()) + "C");

    tft.setCursor(10, 244);
    tft.print("Heap:");
    tft.setCursor(50, 244);
    tft.print(String(ESP.getFreeHeap()));

    tft.setCursor(10, 256);
    tft.print("Uptime:");
    tft.setCursor(50, 256);
    tft.print(String(millis() / 1000) + "s");
}
