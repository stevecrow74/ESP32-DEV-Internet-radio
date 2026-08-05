#include "tides_config.h"

#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <string.h>

#include "config.h"
#include "project_config.h"

static const char *TIDES_CONFIG_FILE = "/tides_config.json";
static const char *DEFAULT_TIDE_SOURCE_URL = "https://www.tidetime.org/europe/ireland/galway.htm";

static char cfgLocation[64];
static double cfgLatitude = LATITUDE;
static double cfgLongitude = LONGITUDE;
static uint16_t cfgRefreshMinutes = (uint16_t)(TIDE_UPDATE_MS / 60000UL);
static char cfgSourceUrl[192];

static void setDefaults()
{
    strncpy(cfgLocation, TIDE_LOCATION, sizeof(cfgLocation) - 1);
    cfgLocation[sizeof(cfgLocation) - 1] = '\0';
    cfgLatitude = LATITUDE;
    cfgLongitude = LONGITUDE;
    cfgRefreshMinutes = (uint16_t)(TIDE_UPDATE_MS / 60000UL);
    strncpy(cfgSourceUrl, DEFAULT_TIDE_SOURCE_URL, sizeof(cfgSourceUrl) - 1);
    cfgSourceUrl[sizeof(cfgSourceUrl) - 1] = '\0';
}

static void saveConfig()
{
    JsonDocument doc;
    doc["location"] = cfgLocation;
    doc["latitude"] = cfgLatitude;
    doc["longitude"] = cfgLongitude;
    doc["refreshMinutes"] = cfgRefreshMinutes;
    doc["sourceUrl"] = cfgSourceUrl;

    File f = SPIFFS.open(TIDES_CONFIG_FILE, FILE_WRITE);
    if (!f)
    {
        Serial.println("[TIDES] failed to open config for write");
        return;
    }

    serializeJsonPretty(doc, f);
    f.close();
}

void tidesConfigInit()
{
    setDefaults();

    if (!SPIFFS.exists(TIDES_CONFIG_FILE))
    {
        saveConfig();
        return;
    }

    File f = SPIFFS.open(TIDES_CONFIG_FILE, FILE_READ);
    if (!f)
    {
        Serial.println("[TIDES] failed to open config for read");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        Serial.print("[TIDES] parse failed: ");
        Serial.println(err.c_str());
        return;
    }

    const char *loc = doc["location"] | TIDE_LOCATION;
    strncpy(cfgLocation, loc, sizeof(cfgLocation) - 1);
    cfgLocation[sizeof(cfgLocation) - 1] = '\0';
    cfgLatitude = doc["latitude"] | LATITUDE;
    cfgLongitude = doc["longitude"] | LONGITUDE;

    const char *url = doc["sourceUrl"] | DEFAULT_TIDE_SOURCE_URL;
    strncpy(cfgSourceUrl, url, sizeof(cfgSourceUrl) - 1);
    cfgSourceUrl[sizeof(cfgSourceUrl) - 1] = '\0';

    uint16_t mins = doc["refreshMinutes"] | (uint16_t)(TIDE_UPDATE_MS / 60000UL);
    if (mins < 1)
        mins = 1;
    if (mins > 720)
        mins = 720;
    cfgRefreshMinutes = mins;
}

String tidesLocation()
{
    return String(cfgLocation);
}

double tidesLatitude()
{
    return cfgLatitude;
}

double tidesLongitude()
{
    return cfgLongitude;
}

uint32_t tidesRefreshMs()
{
    return (uint32_t)cfgRefreshMinutes * 60000UL;
}

String tidesSourceUrl()
{
    return String(cfgSourceUrl);
}

void tidesConfigUpdate(
    const String &location,
    double latitude,
    double longitude,
    uint16_t refreshMinutes,
    const String &sourceUrl)
{
    if (location.length())
    {
        strncpy(cfgLocation, location.c_str(), sizeof(cfgLocation) - 1);
        cfgLocation[sizeof(cfgLocation) - 1] = '\0';
    }

    cfgLatitude = latitude;
    cfgLongitude = longitude;

    if (refreshMinutes < 1)
        refreshMinutes = 1;
    if (refreshMinutes > 720)
        refreshMinutes = 720;

    cfgRefreshMinutes = refreshMinutes;

    if (sourceUrl.length())
    {
        strncpy(cfgSourceUrl, sourceUrl.c_str(), sizeof(cfgSourceUrl) - 1);
        cfgSourceUrl[sizeof(cfgSourceUrl) - 1] = '\0';
    }

    saveConfig();
}
