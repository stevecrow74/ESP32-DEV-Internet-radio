#include "favourites.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

static const char *FAV_FILE = "/favourites.json";

static const char *DEFAULT_FAVS = R"json([
  { "name": "Rewind", "url": "http://ingest4.cdnstream1.com:7080/2551_128.mp3" },
  { "name": "FIP", "url": "https://icecast.radiofrance.fr/fip-hifi.aac" },
  { "name": "Radio Paradise", "url": "http://stream.radioparadise.com/mp3-192" },
  { "name": "SomaFM - Groove Salad", "url": "http://ice1.somafm.com/groovesalad-128-mp3" },
  { "name": "SomaFM - Secret Agent", "url": "http://ice1.somafm.com/secretagent-128-mp3" },
  { "name": "SomaFM - Jazz24", "url": "http://ice1.somafm.com/jazz24-128-mp3" },
  { "name": "NPR (example)", "url": "https://npr-ice.stream.publicradio.org/npr.mp3" },
  { "name": "KCRW", "url": "http://kcrw.stream.publicradio.org/kcrw_192k_mp3" },
  { "name": "Classic FM (example)", "url": "http://media-ice.musicradio.com/ClassicFMMP3" },
  { "name": "Radio Example", "url": "http://stream.example.com/stream.mp3" }
])json";

static void ensureDefaultFavFile()
{
    if (SPIFFS.exists(FAV_FILE))
        return;

    File f = SPIFFS.open(FAV_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[WARN] cannot create default favourites file");
        return;
    }
    f.print(DEFAULT_FAVS);
    f.close();
    Serial.print("[INFO] default favourites written to ");
    Serial.println(FAV_FILE);
}

static std::vector<FavStation> favs;

static void saveFavs()
{
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto &f : favs) {
        JsonObject o = arr.add<JsonObject>();
        o["name"] = f.name;
        o["url"] = f.url;
    }

    File f = SPIFFS.open(FAV_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[ERROR] Unable to open favourites file for write");
        return;
    }
    serializeJson(doc, f);
    f.close();
}

void favsInit()
{
    favs.clear();

    ensureDefaultFavFile();

    if (!SPIFFS.exists(FAV_FILE))
        return;

    File f = SPIFFS.open(FAV_FILE, FILE_READ);
    if (!f) {
        Serial.println("[WARN] could not open favourites file");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.print("[WARN] favourites parse failed: ");
        Serial.println(err.c_str());
        return;
    }

    if (!doc.is<JsonArray>())
        return;

    for (JsonObject o : doc.as<JsonArray>()) {
        FavStation s;
        s.name = o["name"].as<const char*>() ? String(o["name"].as<const char*>()) : String("");
        s.url = o["url"].as<const char*>() ? String(o["url"].as<const char*>()) : String("");
        favs.push_back(s);
    }
}

int favsCount()
{
    return (int)favs.size();
}

bool favsAdd(const String &name, const String &url)
{
    FavStation s;
    s.name = name;
    s.url = url;
    favs.push_back(s);
    saveFavs();
    return true;
}

bool favsRemove(int index)
{
    if (index < 0 || index >= (int)favs.size())
        return false;
    favs.erase(favs.begin() + index);
    saveFavs();
    return true;
}

FavStation favsGet(int index)
{
    FavStation empty;
    if (index < 0 || index >= (int)favs.size())
        return empty;
    return favs[index];
}

String favsListJson()
{
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto &f : favs) {
        JsonObject o = arr.add<JsonObject>();
        o["name"] = f.name;
        o["url"] = f.url;
    }

    String out;
    serializeJson(doc, out);
    return out;
}
