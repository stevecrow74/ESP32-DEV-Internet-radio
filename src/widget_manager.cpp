/******************************************************************************
 *
 * widget_manager.cpp
 *
 ******************************************************************************/

#include "widget_manager.h"
#include "config.h"

#include "widget_clock.h"
#include "widget_adsb.h"
#include "widget_audio.h"
#include "widget_favourites.h"
#include "widget_tides.h"

#include "widget_weather.h"

WidgetID currentWidget = WIDGET_AUDIO;      // Start on Audio screen

static unsigned long lastActivity = 0;
static unsigned long lastAutoCycle = 0;
static unsigned long holdSelectedUntil = 0;
static const unsigned long AUTO_CYCLE_MS = 10000;
static const unsigned long SELECTED_HOLD_MS = 20000;

static WidgetID nextAutoCycleWidget(WidgetID from)
{
    WidgetID next = static_cast<WidgetID>((from + 1) % MAX_WIDGETS);

    // Keep favourites out of timed auto-cycle; it remains available via manual switching.
    if (next == WIDGET_SYSTEM)
        next = static_cast<WidgetID>((next + 1) % MAX_WIDGETS);

    return next;
}

void widgetUserActivity()
{
    lastActivity = millis();
}

void widgetHoldSelected()
{
    holdSelectedUntil = millis() + SELECTED_HOLD_MS;
    lastActivity = millis();
    lastAutoCycle = millis();
}

void widgetInit()
{
    clockWidgetInit();
    adsbWidgetInit();
    audioWidgetInit();
    weatherWidgetInit();
    tidesWidgetInit();
    favWidgetInit();

    // initialize activity timer
    widgetUserActivity();
    holdSelectedUntil = 0;
    lastAutoCycle = millis();
}

void widgetLoop()
{
    unsigned long now = millis();

    // While manually selected, hold on the current widget for 20 seconds.
    if (holdSelectedUntil != 0)
    {
        if ((long)(now - holdSelectedUntil) < 0)
        {
            // Still in hold period.
        }
        else
        {
            holdSelectedUntil = 0;
            lastActivity = now;
        }
    }
    else if (currentWidget != WIDGET_CLOCK && (now - lastActivity) > AUTO_CYCLE_MS)
    {
        // No manual widget selection active: cycle to the next widget every 10 seconds.
        currentWidget = nextAutoCycleWidget(currentWidget);
        widgetDraw();
        lastActivity = now;
        lastAutoCycle = now;
    }
    else if (holdSelectedUntil == 0 && (now - lastAutoCycle) > AUTO_CYCLE_MS)
    {
        currentWidget = nextAutoCycleWidget(currentWidget);
        widgetDraw();
        lastAutoCycle = now;
        lastActivity = now;
    }
    switch (currentWidget)
    {
        case WIDGET_CLOCK:
            clockWidgetLoop();
            break;

        case WIDGET_ADSB:
            adsbWidgetLoop();
            break;

        case WIDGET_SYSTEM:
            favWidgetLoop();
            break;

        case WIDGET_AUDIO:
            audioWidgetLoop();
            break;

        case WIDGET_WEATHER:
            weatherWidgetLoop();
            break;

        case WIDGET_TIDES:
            tidesWidgetLoop();
            break;

        default:
            break;
    }
}

void widgetNext()
{
    currentWidget =
        static_cast<WidgetID>((currentWidget + 1) % MAX_WIDGETS);

    widgetHoldSelected();
    widgetDraw();
}

void widgetPrevious()
{
    currentWidget =
        static_cast<WidgetID>((currentWidget + MAX_WIDGETS - 1) % MAX_WIDGETS);

    widgetHoldSelected();
    widgetDraw();
}

void widgetDraw()
{
    switch (currentWidget)
    {
        case WIDGET_CLOCK:
            clockWidgetDraw();
            break;

        case WIDGET_ADSB:
            adsbWidgetDraw();
            break;

        case WIDGET_AUDIO:
            audioWidgetDraw();
            break;

        case WIDGET_WEATHER:
            weatherWidgetDraw();
            break;

        case WIDGET_TIDES:
            tidesWidgetDraw();
            break;

        case WIDGET_SYSTEM:
            favWidgetDraw();
            break;

        default:
            break;
    }
}