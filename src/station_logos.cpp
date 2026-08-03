#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "display.h"
#include "station_logos.h"

extern Adafruit_ST7789 tft;

void drawStationLogo(const String &station)
{
    // Logo background
    tft.fillRoundRect(175, 105, 55, 55, 6, ST77XX_WHITE);

    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(1);

    if (station == "Rewind")
    {
        tft.setCursor(185,125);
        tft.print("REW");
    }
    else if (station == "FIP")
    {
        tft.setCursor(190,125);
        tft.print("FIP");
    }
    else
    {
        tft.setCursor(182,125);
        tft.print("RADIO");
    }
}