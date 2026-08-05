#include "favourites.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

static const char *FAV_FILE = "/favourites.json";

static const char *DEFAULT_FAVS = R"json([
  { "name": "Radio X", "url": "https://icecast.thisisdax.com/RadioXUK" },
    { "name": "Zenith Rock", "url": "http://91.189.64.188:3644/zenith128mp3" },
  { "name": "Classic Hits", "url": "http://live-bauerie.sharp-stream.com/CLASSIC?ref=RF" },
    { "name": "Classic FM", "url": "http://media-ice.musicradio.com/ClassicFMMP3" },
  { "name": "BoB FM", "url": "http://sirius.shoutca.st:8011/stream" },
    { "name": "Onic Alt", "url": "http://onic.dublin.live.stream.broadcasting.news/stream-alternative-mobile?ref=RF" },
  { "name": "Velvet", "url": "http://stream.btsstream.com:8012/velvet.mp3" },
  { "name": "Onic 80's", "url": "http://onic.dublin.live.stream.broadcasting.news/stream-80s?ref=RF" },
    { "name": "Darkwave Radio", "url": "http://77.249.39.15:8000/;" },
    { "name": "Soma 80's", "url": "https://ice6.somafm.com/u80s-64-aac" },
    { "name": "Soma Indie", "url": "https://ice6.somafm.com/indiepop-128-mp3" },
    { "name": "Soma Doomed", "url": "https://ice5.somafm.com/doomed-128-mp3" },
    { "name": "Soma 70's", "url": "https://ice5.somafm.com/seventies-128-mp3" },
    { "name": "Mellow Mix", "url": "https://stream.radioparadise.com/mellow-320" },
    { "name": "Rock Mix", "url": "https://stream.radioparadise.com/rock-320" },
    { "name": "Rock Antenne Classic", "url": "https://stream.rockantenne.de/classic-perlen" },
    { "name": "Rock Antenne Alternative", "url": "https://stream.rockantenne.de/alternative" },
    { "name": "Rock Antenne Heavy Metal", "url": "https://stream.rockantenne.de/heavy-metal" },
    { "name": "Planet Rock", "url": "https://media-ice.musicradio.com/PlanetRockMP3" }
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
