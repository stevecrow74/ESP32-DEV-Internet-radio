#include "station_manager.h"
#include "audio_engine.h"
#include "audio_state.h"
#include "favourites.h"

static const char *DEFAULT_NAME = "RAdio X UK";
static const char *DEFAULT_URL = "https://icecast.thisisdax.com/RadioXUK";

static int current = 0;
static RadioStation currentRadioStation = {DEFAULT_NAME, DEFAULT_URL};
static String currentNameCache = DEFAULT_NAME;
static String currentUrlCache = DEFAULT_URL;
static String currentLogoCache;

void stationInit()
{
    favsInit();

    current = 0;

    int count = favsCount();
    for (int i = 0; i < count; i++)
    {
        FavStation s = favsGet(i);
        if (s.name.equalsIgnoreCase("Radio X Uk"))
        {
            current = i;
            break;
        }
    }
}

const RadioStation* stationCurrent()
{
    int count = favsCount();

    if (count <= 0)
    {
        currentNameCache = DEFAULT_NAME;
        currentUrlCache = DEFAULT_URL;
    }
    else
    {
        if (current >= count)
            current = 0;
        if (current < 0)
            current = count - 1;

        FavStation s = favsGet(current);
        currentNameCache = s.name;
        currentUrlCache = s.url;
        currentLogoCache = s.logoUrl;
    }

    currentRadioStation.name = currentNameCache.c_str();
    currentRadioStation.url = currentUrlCache.c_str();
    return &currentRadioStation;
}

void stationNext()
{
    int count = favsCount();
    if (count <= 0)
        return;

    current++;

    if(current >= count)
        current = 0;
}

void stationPrevious()
{
    int count = favsCount();
    if (count <= 0)
        return;

    current--;

    if(current < 0)
        current = count - 1;
}

void stationConnectCurrent()
{
    const RadioStation *st = stationCurrent();

    if (!st || !st->url || st->url[0] == '\0')
        return;

    currentStation = st->name;
    currentLogoUrl = currentLogoCache;
    currentTitle = "";
    currentBitrate = "";

    audioChanged = true;

    audioStop();

    delay(1500);

    audioConnect(st->url);
    
}

void stationPlayUrl(const char *name, const char *url, const char *logoUrl)
{
    currentStation = name;
    currentLogoUrl = logoUrl ? logoUrl : "";
    currentTitle = "";
    currentBitrate = "";

    audioChanged = true;

    audioStop();

    delay(100);

    audioConnect(url);
}
