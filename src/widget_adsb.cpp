/******************************************************************************
 *
 * widget_adsb.cpp
 *
 ******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "display.h"
#include "config.h"
#include "adsb_config.h"
#include "widget_adsb.h"

extern Adafruit_ST7789 tft;

struct AdsbAircraft
{
    String flight;
    String hex;
    float lat = NAN;
    float lon = NAN;
    float alt = NAN;
    float speed = NAN;
    float track = NAN;
    float seen = NAN;
    float rssi = NAN;
    float distanceKm = NAN;
};

static AdsbAircraft nearestAircraft;
static int aircraftCount = -1;
static String adsbStatus = "Waiting...";
static unsigned long lastFetchMs = 0;
static bool fetchInProgress = false;

static const unsigned long ADSB_FETCH_MS = ADSB_UPDATE_MS;

static String adsbFeedUrl()
{
    IPAddress ip = WiFi.localIP();
    bool onLocalLan = (ip[0] == 192 && ip[1] == 168 && ip[2] == 1);
    String localSsid = adsbLocalSsid();
    bool ssidMatches = (localSsid.length() > 0 && WiFi.SSID() == localSsid);

    if (ssidMatches || onLocalLan)
        return adsbLocalFeedUrl();

    return adsbPublicFeedUrl();
}

static float deg2rad(float deg)
{
    return deg * 0.01745329252f;
}

static float distanceKm(float lat1, float lon1, float lat2, float lon2)
{
    const float earthRadiusKm = 6371.0f;
    float dLat = deg2rad(lat2 - lat1);
    float dLon = deg2rad(lon2 - lon1);
    float a = sinf(dLat / 2.0f) * sinf(dLat / 2.0f) +
              cosf(deg2rad(lat1)) * cosf(deg2rad(lat2)) *
              sinf(dLon / 2.0f) * sinf(dLon / 2.0f);
    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    return earthRadiusKm * c;
}

static String trimFlight(const char *flight)
{
    if (!flight)
        return "";

    String s = flight;
    s.trim();
    return s;
}

static float readFloatOrNan(JsonObject obj, std::initializer_list<const char *> keys)
{
    for (const char *key : keys)
    {
        if (obj.containsKey(key) && !obj[key].isNull())
        {
            return obj[key].as<float>();
        }
    }

    return NAN;
}

static void clearAircraft()
{
    nearestAircraft = AdsbAircraft();
    aircraftCount = 0;
}

static void fetchAdsbData()
{
    if (fetchInProgress)
        return;

    if (WiFi.status() != WL_CONNECTED)
    {
        adsbStatus = "No WiFi";
        clearAircraft();
        return;
    }

    String feedUrl = adsbFeedUrl();

    if (feedUrl.length() == 0)
    {
        adsbStatus = "Set ADSB URL";
        clearAircraft();
        return;
    }

    fetchInProgress = true;
    adsbStatus = "Updating...";

    HTTPClient http;
    http.begin(feedUrl);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        adsbStatus = "Fetch failed";
        clearAircraft();
        http.end();
        fetchInProgress = false;
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        adsbStatus = "Parse failed";
        clearAircraft();
        fetchInProgress = false;
        return;
    }

    JsonArray aircraft = doc["aircraft"].as<JsonArray>();
    if (aircraft.isNull())
    {
        adsbStatus = "No aircraft";
        clearAircraft();
        fetchInProgress = false;
        return;
    }

    aircraftCount = 0;
    nearestAircraft = AdsbAircraft();
    float nearestDistance = 1e9f;

    for (JsonObject obj : aircraft)
    {
        float lat = obj["lat"] | NAN;
        float lon = obj["lon"] | NAN;
        if (isnan(lat) || isnan(lon))
            continue;

        float d = distanceKm(ADSB_LATITUDE, ADSB_LONGITUDE, lat, lon);
        if (d > adsbRangeKm())
            continue;

        aircraftCount++;

        if (d < nearestDistance)
        {
            nearestDistance = d;
            nearestAircraft.flight = trimFlight(obj["flight"] | obj["r"] | "UNKNOWN");
            nearestAircraft.hex = obj["hex"] | "";
            nearestAircraft.lat = lat;
            nearestAircraft.lon = lon;
            nearestAircraft.alt = readFloatOrNan(obj, {"alt_baro", "alt_geom", "altitude", "baro_altitude", "alt"});
            nearestAircraft.speed = readFloatOrNan(obj, {"gs", "tas", "ias", "speed", "groundspeed", "velocity"});
            nearestAircraft.track = obj["track"] | NAN;
            nearestAircraft.seen = obj["seen"] | NAN;
            nearestAircraft.rssi = obj["rssi"] | NAN;
            nearestAircraft.distanceKm = d;
        }
    }

    if (aircraftCount == 0)
    {
        adsbStatus = "No nearby aircraft";
    }
    else
    {
        adsbStatus = "Updated";
    }

    lastFetchMs = millis();
    fetchInProgress = false;
}

static void drawLabelValue(int y, const char *label, const String &value)
{
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, y);
    tft.print(label);

    tft.setCursor(130, y);
    tft.print(value);
}

void adsbWidgetInit()
{
    lastFetchMs = 0;
    aircraftCount = -1;
    adsbStatus = "Waiting...";
}

void adsbWidgetLoop()
{
    if (millis() - lastFetchMs >= ADSB_FETCH_MS)
    {
        fetchAdsbData();
    }
}

void adsbWidgetDraw()
{
    tft.fillRect(0, 100, 240, 220, ST77XX_BLACK);

    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(2);
    tft.setCursor(70, 110);
    tft.print("ADS-B");

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, 135);
    tft.print(adsbStatus);

    if (aircraftCount < 0)
    {
        tft.setCursor(10, 155);
        tft.print("Waiting for data...");
        return;
    }

    drawLabelValue(155, "Nearby", String(aircraftCount));

    if (aircraftCount == 0)
    {
        tft.setCursor(10, 185);
        tft.print("No aircraft in range");
        tft.setCursor(10, 205);
        tft.print("Set ADSB_FEED_URL");
        return;
    }

    tft.setTextSize(2);
    tft.setCursor(10, 185);
    tft.print(nearestAircraft.flight.length() ? nearestAircraft.flight : "UNKNOWN");

    tft.setTextSize(1);
    tft.setCursor(10, 210);
    tft.print("Hex: ");
    tft.print(nearestAircraft.hex);

    tft.setCursor(10, 225);
    tft.print("Dist: ");
    tft.print(String(nearestAircraft.distanceKm, 1));
    tft.print(" km");

    tft.setCursor(10, 240);
    tft.print("Alt: ");
    if (!isnan(nearestAircraft.alt))
        tft.print(String(nearestAircraft.alt, 0));
    else
        tft.print("N/A");
    tft.print(" ft");

    tft.setCursor(10, 255);
    tft.print("Spd: ");
    if (!isnan(nearestAircraft.speed))
        tft.print(String(nearestAircraft.speed, 0));
    else
        tft.print("N/A");
    tft.print(" kt");

    tft.setCursor(10, 270);
    tft.print("Trk: ");
    if (!isnan(nearestAircraft.track))
        tft.print(String(nearestAircraft.track, 0));
    else
        tft.print("N/A");
    tft.print(" deg");
}
