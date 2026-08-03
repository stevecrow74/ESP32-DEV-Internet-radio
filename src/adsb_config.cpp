#include "adsb_config.h"

#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "config.h"

static const char *ADSB_CONFIG_FILE = "/adsb_config.json";

static uint16_t configRangeKm = ADSB_RANGE_KM;
static String configLocalFeedUrl = ADSB_LOCAL_FEED_URL;
static String configPublicFeedUrl = ADSB_PUBLIC_FEED_URL;
static String configLocalSsid = ADSB_LOCAL_SSID;

static void saveConfig()
{
    JsonDocument doc;
    doc["rangeKm"] = configRangeKm;
    doc["localFeedUrl"] = configLocalFeedUrl;
    doc["publicFeedUrl"] = configPublicFeedUrl;
    doc["localSsid"] = configLocalSsid;

    File f = SPIFFS.open(ADSB_CONFIG_FILE, FILE_WRITE);
    if (!f)
    {
        Serial.println("[ADSB] failed to open config for write");
        return;
    }

    serializeJsonPretty(doc, f);
    f.close();
}

void adsbConfigInit()
{
    if (!SPIFFS.exists(ADSB_CONFIG_FILE))
    {
        saveConfig();
        return;
    }

    File f = SPIFFS.open(ADSB_CONFIG_FILE, FILE_READ);
    if (!f)
    {
        Serial.println("[ADSB] failed to open config for read");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        Serial.print("[ADSB] config parse failed: ");
        Serial.println(err.c_str());
        return;
    }

    configRangeKm = doc["rangeKm"] | ADSB_RANGE_KM;
    configLocalFeedUrl = doc["localFeedUrl"] | ADSB_LOCAL_FEED_URL;
    configPublicFeedUrl = doc["publicFeedUrl"] | ADSB_PUBLIC_FEED_URL;
    configLocalSsid = doc["localSsid"] | ADSB_LOCAL_SSID;
}

uint16_t adsbRangeKm()
{
    return configRangeKm;
}

String adsbLocalFeedUrl()
{
    return configLocalFeedUrl;
}

String adsbPublicFeedUrl()
{
    return configPublicFeedUrl;
}

String adsbLocalSsid()
{
    return configLocalSsid;
}

void adsbConfigUpdate(
    uint16_t rangeKm,
    const String &localFeedUrl,
    const String &publicFeedUrl,
    const String &localSsid)
{
    configRangeKm = rangeKm;
    configLocalFeedUrl = localFeedUrl;
    configPublicFeedUrl = publicFeedUrl;
    configLocalSsid = localSsid;
    saveConfig();
}
