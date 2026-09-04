/******************************************************************************
 *
 * widget_manager.h
 *
 ******************************************************************************/

#pragma once

enum WidgetID
{
    WIDGET_CLOCK = 0,
    WIDGET_SYSTEM,
    WIDGET_AUDIO,
    WIDGET_FAVORITES,
    WIDGET_TIDES,
    WIDGET_WEATHER,
    WIDGET_ADSB,
    
};

void widgetInit();
void widgetLoop();

void widgetNext();
void widgetPrevious();

void widgetDraw();

extern WidgetID currentWidget;

// Retained for input compatibility; widgets are manually selected only.
void widgetUserActivity();

// Retained for input compatibility; widgets are manually selected only.
void widgetHoldSelected();

// Retained for input compatibility; widgets are manually selected only.
void widgetHoldSelectedFor(unsigned long durationMs);