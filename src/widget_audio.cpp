/******************************************************************************
 *
 * widget_audio.cpp
 *
 ******************************************************************************/

#include <Arduino.h>

#include "display.h"
#include "widget_audio.h"
#include "audio_state.h"
#include "audio_engine.h"
#include "station_logos.h"
#include "theme.h"

extern Adafruit_ST7789 tft;
static int stationOffset = 0;
static int titleOffset = 0;

static unsigned long lastScroll = 0;

static bool firstDraw = true;

void drawWrappedText(int x, int y, int width, const String &text)
{
    String line = "";
    int cursorY = y;

    for (int i = 0; i < text.length(); i++)
    {
        char c = text[i];

        if (c == ' ' || i == text.length() - 1)
        {
            if (i == text.length() - 1)
                line += c;

            int16_t x1, y1;
            uint16_t w, h;

            tft.getTextBounds(line, 0, 0, &x1, &y1, &w, &h);

            if (x + w > width)
            {
                cursorY += 12;
                line = "";
            }
        }

        line += c;
    }

    cursorY = y;
    line = "";

    String word = "";

    for (int i = 0; i < text.length(); i++)
    {
        char c = text[i];

        if (c == ' ' || i == text.length() - 1)
        {
            if (i == text.length() - 1)
                word += c;

            String test = line;

            if (test.length())
                test += " ";

            test += word;

            int16_t x1, y1;
            uint16_t w, h;

            tft.getTextBounds(test, 0, 0, &x1, &y1, &w, &h);

            if (w > width)
            {
                tft.setCursor(x, cursorY);
                tft.print(line);

                cursorY += 12;
                line = word;
            }
            else
            {
                if (line.length())
                    line += " ";

                line += word;
            }

            word = "";
        }
        else
        {
            word += c;
        }
    }

    if (line.length())
    {
        tft.setCursor(x, cursorY);
        tft.print(line);
    }
}

void audioWidgetInit()
{
    audioChanged = true;      // Force initial draw
}

void audioWidgetLoop()
{
    if (audioChanged)
    {
        audioChanged = false;
        audioWidgetDraw();
    }
}

void drawScrollingText(
    int x,
    int y,
    int width,
    const String &text,
    int &offset)
{
    tft.setTextWrap(false);

    int textWidth = text.length() * 6;      // Approx. width for size 1 font

    if (textWidth <= width)
    {
        tft.setCursor(x, y);
        tft.print(text);
        return;
    }

    if (millis() - lastScroll > 150)
    {
        lastScroll = millis();

        offset++;

        if (offset > textWidth + 20)
            offset = 0;
    }

    tft.fillRect(x, y, width, 10, COLOR_BACKGROUND);

    tft.setCursor(x - offset, y);
    tft.print(text);

    tft.setCursor(x - offset + textWidth + 30, y);
    tft.print(text);
}
void audioWidgetDraw()
{
    tft.fillRect(0, 100, 240, 220, COLOR_BACKGROUND);

    // Header
    tft.fillRect(0,100,240,28,COLOR_HEADER);

    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
drawScrollingText(
    18,
    108,
    150,
    currentStation,
    stationOffset
);

    drawStationLogo(currentStation);

    // Now Playing
    tft.setTextSize(1);
    tft.setTextColor(COLOR_LABEL);

    tft.setCursor(10,145);
    tft.print("NOW PLAYING");

    tft.drawFastHLine(10,156,220,COLOR_TEXT);

    tft.setTextColor(COLOR_TEXT);

    tft.setCursor(10,170);
    drawWrappedText(10,170,220,currentTitle);

       // Bitrate

    tft.drawFastHLine(10,220,220,COLOR_TEXT);

    tft.setCursor(10,230);
    tft.print("Bitrate");

    tft.setCursor(80,230);
    tft.print(currentBitrate);

    tft.print(" kbps");

    // Volume

    tft.setCursor(10,300);
    tft.print("Volume");

    // Bar outline
    tft.drawRect(80,300,110,10,COLOR_TEXT);

    // Clear inside
    tft.fillRect(81,301,108,8,COLOR_BACKGROUND);

    // Calculate width
    int barWidth = map(audioGetVolume(),0,MAX_VOLUME,0,108);

    // Draw level
    tft.fillRect(81,301,barWidth,8,COLOR_SUCCESS);
}

void audioWidgetUpdateVolume()
{
    // Clear inside only
    tft.fillRect(81,301,108,8,COLOR_BACKGROUND);

    // Calculate width
    int barWidth = map(audioGetVolume(),0,MAX_VOLUME,0,108);

    // Draw new level
    tft.fillRect(81,301,barWidth,8,COLOR_SUCCESS);
}