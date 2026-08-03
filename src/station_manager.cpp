#include "station_manager.h"
#include "audio_engine.h"
#include "audio_state.h"
#include "favourites.h"

static const char *DEFAULT_NAME = "Rewind";
static const char *DEFAULT_URL = "http://ingest4.cdnstream1.com:7080/2551_128.mp3";

static int current = 0;
static RadioStation currentRadioStation = {DEFAULT_NAME, DEFAULT_URL};
static String currentNameCache = DEFAULT_NAME;
static String currentUrlCache = DEFAULT_URL;

void stationInit()
{
    favsInit();

    current = 0;

    int count = favsCount();
    for (int i = 0; i < count; i++)
    {
        FavStation s = favsGet(i);
        if (s.name.equalsIgnoreCase("Rewind"))
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
    currentTitle = "";
    currentBitrate = "";

    audioChanged = true;

    audioStop();

    delay(100);

    audioConnect(st->url);
}

void stationPlayUrl(const char *name, const char *url)
{
    currentStation = name;
    currentTitle = "";
    currentBitrate = "";

    audioChanged = true;

    audioStop();

    delay(100);

    audioConnect(url);
}
