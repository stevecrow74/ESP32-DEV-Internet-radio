#include "saved_networks.h"

#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

static const char *SAVED_NETWORKS_FILE = "/saved_networks.json";
static std::vector<SavedNetwork> savedNetworks;

static void saveNetworks()
{
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto &n : savedNetworks)
    {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = n.ssid;
        o["password"] = n.password;
    }

    File f = SPIFFS.open(SAVED_NETWORKS_FILE, FILE_WRITE);
    if (!f)
    {
        Serial.println("[NET] failed to write saved networks");
        return;
    }

    serializeJson(doc, f);
    f.close();
}

void savedNetworksInit()
{
    savedNetworks.clear();

    if (!SPIFFS.exists(SAVED_NETWORKS_FILE))
        return;

    File f = SPIFFS.open(SAVED_NETWORKS_FILE, FILE_READ);
    if (!f)
    {
        Serial.println("[NET] failed to read saved networks");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err || !doc.is<JsonArray>())
        return;

    for (JsonObject o : doc.as<JsonArray>())
    {
        SavedNetwork n;
        n.ssid = o["ssid"].as<const char *>() ? String(o["ssid"].as<const char *>()) : String("");
        n.password = o["password"].as<const char *>() ? String(o["password"].as<const char *>()) : String("");
        if (n.ssid.length())
            savedNetworks.push_back(n);
    }
}

int savedNetworksCount()
{
    return (int)savedNetworks.size();
}

SavedNetwork savedNetworksGet(int index)
{
    SavedNetwork empty;
    if (index < 0 || index >= (int)savedNetworks.size())
        return empty;
    return savedNetworks[index];
}

bool savedNetworksAdd(const String &ssid, const String &password)
{
    if (!ssid.length())
        return false;

    for (auto &n : savedNetworks)
    {
        if (n.ssid == ssid)
        {
            n.password = password;
            saveNetworks();
            return true;
        }
    }

    SavedNetwork n;
    n.ssid = ssid;
    n.password = password;
    savedNetworks.push_back(n);
    saveNetworks();
    return true;
}

bool savedNetworksRemove(int index)
{
    if (index < 0 || index >= (int)savedNetworks.size())
        return false;
    savedNetworks.erase(savedNetworks.begin() + index);
    saveNetworks();
    return true;
}

String savedNetworksListJson()
{
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto &n : savedNetworks)
    {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = n.ssid;
        o["password"] = n.password;
    }

    String out;
    serializeJson(doc, out);
    return out;
}
