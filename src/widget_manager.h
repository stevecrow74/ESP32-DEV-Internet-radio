/******************************************************************************
 *
 * widget_manager.h
 *
 ******************************************************************************/

#pragma once

enum WidgetID
{
    WIDGET_CLOCK = 0,
    WIDGET_ADSB,
    WIDGET_WEATHER,
    WIDGET_TIDES,
    WIDGET_AUDIO,
    WIDGET_SYSTEM
};

void widgetInit();
void widgetLoop();

void widgetNext();
void widgetPrevious();

void widgetDraw();

extern WidgetID currentWidget;

// Notify widget manager of user activity (resets inactivity timer)
void widgetUserActivity();

// Hold the currently selected widget for a longer period after manual selection
void widgetHoldSelected();