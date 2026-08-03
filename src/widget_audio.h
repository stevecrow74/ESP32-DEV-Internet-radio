#pragma once
#include <Arduino.h>

void audioWidgetInit();
void audioWidgetLoop();
void audioWidgetDraw();
void audioWidgetUpdateVolume();
void drawWrappedText(int x, int y, int width, const String &text);
void drawScrollingText(
    int x,
    int y,
    int width,
    const String &text,
    int &offset
);