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
#include "widget_system.h"

#include "widget_weather.h"

WidgetID currentWidget = WIDGET_AUDIO;      // Start on Audio screen

void widgetUserActivity()
{
}

void widgetHoldSelected()
{
}

void widgetHoldSelectedFor(unsigned long durationMs)
{
    (void)durationMs;
}

void widgetInit()
{
    clockWidgetInit();
    adsbWidgetInit();
    audioWidgetInit();
    weatherWidgetInit();
    tidesWidgetInit();
    systemWidgetInit();
    favWidgetInit();

}

void widgetLoop()
{
    // Keep tide data refreshed in the background even when the Tides widget is not selected.
    tidesWidgetLoop();
    switch (currentWidget)
    {
        case WIDGET_CLOCK:
            clockWidgetLoop();
            break;

        case WIDGET_ADSB:
            adsbWidgetLoop();
            break;

        case WIDGET_SYSTEM:
            systemWidgetLoop();
            break;

        case WIDGET_AUDIO:
            audioWidgetLoop();
            break;

        case WIDGET_FAVORITES:
            favWidgetLoop();
            break;

        case WIDGET_WEATHER:
            weatherWidgetLoop();
            break;

        case WIDGET_TIDES:
            break;

        default:
            break;
    }
}

void widgetNext()
{
    currentWidget =
        static_cast<WidgetID>((currentWidget + 1) % MAX_WIDGETS);

    widgetDraw();
}

void widgetPrevious()
{
    currentWidget =
        static_cast<WidgetID>((currentWidget + MAX_WIDGETS - 1) % MAX_WIDGETS);

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

        case WIDGET_FAVORITES:
            favWidgetDraw();
            break;

        case WIDGET_WEATHER:
            weatherWidgetDraw();
            break;

        case WIDGET_TIDES:
            tidesWidgetDraw();
            break;

        case WIDGET_SYSTEM:
            systemWidgetDraw();
            break;

        default:
            break;
    }
}