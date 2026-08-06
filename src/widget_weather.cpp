/******************************************************************************
 *
 * widget_weather.cpp
 *
 ******************************************************************************/

#include <Arduino.h>

#include "display.h"
#include "weather_engine.h"

extern Adafruit_ST7789 tft;

static void drawWeatherIcon(int code, int x, int y)
{
    // Simple vector icons using the built-in GFX primitives.
    if (code == 0 || code == 1)
    {
        // Sun
        tft.fillCircle(x + 18, y + 18, 10, ST77XX_YELLOW);
        tft.drawCircle(x + 18, y + 18, 12, ST77XX_YELLOW);
        tft.drawLine(x + 18, y + 0, x + 18, y + 8, ST77XX_YELLOW);
        tft.drawLine(x + 18, y + 28, x + 18, y + 36, ST77XX_YELLOW);
        tft.drawLine(x + 0, y + 18, x + 8, y + 18, ST77XX_YELLOW);
        tft.drawLine(x + 28, y + 18, x + 36, y + 18, ST77XX_YELLOW);
        tft.drawLine(x + 6, y + 6, x + 11, y + 11, ST77XX_YELLOW);
        tft.drawLine(x + 25, y + 25, x + 30, y + 30, ST77XX_YELLOW);
        tft.drawLine(x + 6, y + 30, x + 11, y + 25, ST77XX_YELLOW);
        tft.drawLine(x + 25, y + 11, x + 30, y + 6, ST77XX_YELLOW);
        return;
    }

    if (code == 2 || code == 3)
    {
        // Cloud
        tft.fillCircle(x + 14, y + 22, 8, ST77XX_WHITE);
        tft.fillCircle(x + 24, y + 18, 10, ST77XX_WHITE);
        tft.fillCircle(x + 34, y + 23, 7, ST77XX_WHITE);
        tft.fillRect(x + 10, y + 22, 30, 12, ST77XX_WHITE);
        return;
    }

    if (code >= 51 && code <= 67)
    {
        // Rain cloud
        tft.fillCircle(x + 14, y + 18, 8, ST77XX_WHITE);
        tft.fillCircle(x + 24, y + 14, 10, ST77XX_WHITE);
        tft.fillCircle(x + 34, y + 19, 7, ST77XX_WHITE);
        tft.fillRect(x + 10, y + 18, 30, 12, ST77XX_WHITE);
        tft.drawLine(x + 16, y + 34, x + 14, y + 42, ST77XX_CYAN);
        tft.drawLine(x + 24, y + 34, x + 22, y + 42, ST77XX_CYAN);
        tft.drawLine(x + 32, y + 34, x + 30, y + 42, ST77XX_CYAN);
        return;
    }

    if (code >= 71 && code <= 86)
    {
        // Snow cloud
        tft.fillCircle(x + 14, y + 18, 8, ST77XX_WHITE);
        tft.fillCircle(x + 24, y + 14, 10, ST77XX_WHITE);
        tft.fillCircle(x + 34, y + 19, 7, ST77XX_WHITE);
        tft.fillRect(x + 10, y + 18, 30, 12, ST77XX_WHITE);
        tft.drawPixel(x + 16, y + 36, ST77XX_CYAN);
        tft.drawPixel(x + 24, y + 40, ST77XX_CYAN);
        tft.drawPixel(x + 32, y + 36, ST77XX_CYAN);
        tft.drawLine(x + 16, y + 34, x + 16, y + 42, ST77XX_CYAN);
        tft.drawLine(x + 12, y + 38, x + 20, y + 38, ST77XX_CYAN);
        tft.drawLine(x + 21, y + 36, x + 27, y + 42, ST77XX_CYAN);
        tft.drawLine(x + 27, y + 36, x + 21, y + 42, ST77XX_CYAN);
        return;
    }

    if (code >= 95)
    {
        // Thunderstorm cloud
        tft.fillCircle(x + 14, y + 18, 8, ST77XX_WHITE);
        tft.fillCircle(x + 24, y + 14, 10, ST77XX_WHITE);
        tft.fillCircle(x + 34, y + 19, 7, ST77XX_WHITE);
        tft.fillRect(x + 10, y + 18, 30, 12, ST77XX_WHITE);
        tft.fillTriangle(x + 22, y + 32, x + 28, y + 32, x + 24, y + 44, ST77XX_YELLOW);
        return;
    }

    // Default: simple cloud
    tft.fillCircle(x + 14, y + 22, 8, ST77XX_WHITE);
    tft.fillCircle(x + 24, y + 18, 10, ST77XX_WHITE);
    tft.fillCircle(x + 34, y + 23, 7, ST77XX_WHITE);
    tft.fillRect(x + 10, y + 22, 30, 12, ST77XX_WHITE);
}

void weatherWidgetInit()
{
}

void weatherWidgetLoop()
{
}

void weatherWidgetDraw()
{
    tft.fillRect(0, 100, 240, 220, ST77XX_BLACK);

    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(2);
    tft.setCursor(65,110);
    tft.print("WEATHER");

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);

    tft.setCursor(10,145);
    tft.print("Temperature");

    tft.setCursor(145,145);
    tft.print(weatherTemperature());

    tft.setCursor(10,170);
    tft.print("Humidity");

    tft.setCursor(145,170);
    tft.print(weatherHumidity());

    tft.setCursor(10,195);
    tft.print("Wind");

    tft.setCursor(145,195);
    tft.print(weatherWind());

    tft.setCursor(10,215);
    tft.print(weatherDescription());

    drawWeatherIcon(weatherCode(), 100, 240);
}