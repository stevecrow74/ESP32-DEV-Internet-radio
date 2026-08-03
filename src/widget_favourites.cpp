#include <Arduino.h>
#include "display.h"
#include "widget_favourites.h"
#include "favourites.h"
#include "station_manager.h"
#include "theme.h"

extern Adafruit_ST7789 tft;

static int selected = 0;
static int offset = 0;

void favWidgetInit()
{
    favsInit();
}

void favWidgetLoop()
{
    // nothing dynamic for now
}

static void drawList()
{
    tft.fillRect(0, 100, 240, 220, COLOR_BACKGROUND);

    tft.fillRect(0,100,240,28,COLOR_HEADER);

    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.setCursor(10,108);
    tft.print("Favourites");

    tft.setTextSize(1);

    int count = favsCount();

    int y = 140;
    int show = 6;

    if (selected < offset) offset = selected;
    if (selected >= offset + show) offset = selected - show + 1;

    for (int i = 0; i < show; i++) {
        int idx = offset + i;
        if (idx >= count) break;

        FavStation s = favsGet(idx);

        if (idx == selected) {
            tft.fillRect(10, y-2, 220, 14, COLOR_HEADER);
            tft.setTextColor(COLOR_TEXT);
        } else {
            tft.setTextColor(COLOR_LABEL);
        }

        tft.setCursor(12, y);
        tft.print(s.name);

        y += 18;
    }

    if (count == 0) {
        tft.setTextColor(COLOR_LABEL);
        tft.setCursor(10,150);
        tft.print("No favourites");
    }
}

void favWidgetDraw()
{
    drawList();
}

void favWidgetRotate(int dir)
{
    int count = favsCount();
    if (count == 0) return;

    selected += dir;
    if (selected < 0) selected = 0;
    if (selected >= count) selected = count - 1;

    favWidgetDraw();
}

void favWidgetSelect()
{
    int count = favsCount();
    if (count == 0) return;

    FavStation s = favsGet(selected);
    stationPlayUrl(s.name.c_str(), s.url.c_str());
}

void favWidgetDeleteSelected()
{
    int count = favsCount();
    if (count == 0) return;

    favsRemove(selected);

    if (selected >= favsCount()) selected = favsCount() - 1;
    if (selected < 0) selected = 0;

    favWidgetDraw();
}
