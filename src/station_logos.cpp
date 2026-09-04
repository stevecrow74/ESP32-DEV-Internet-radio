#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TJpg_Decoder.h>
#include <vector>
#include <SPIFFS.h>

#include "display.h"
#include "station_logos.h"
#include "audio_state.h"

extern Adafruit_ST7789 tft;

static String loadedStation;
static String logoLoadedStation;
static String logoLoadedUrl;
static std::vector<uint8_t> logoData;
static bool logoIsJpeg = false;
static String logoRequestStation;
static String logoRequestUrl;
static TaskHandle_t logoTaskHandle = nullptr;
static volatile bool logoLoading = false;

static bool drawJpegBlock(int16_t x, int16_t y, uint16_t width,
                          uint16_t height, uint16_t *bitmap)
{
    if (x >= 230 || y >= 160)
        return false;

    uint16_t clippedWidth = min(width, static_cast<uint16_t>(230 - x));
    uint16_t clippedHeight = min(height, static_cast<uint16_t>(160 - y));
    tft.drawRGBBitmap(x, y, bitmap, clippedWidth, clippedHeight);
    return true;
}

static String urlEncode(const String &value)
{
    const char hex[] = "0123456789ABCDEF";
    String encoded;

    for (size_t i = 0; i < value.length(); ++i)
    {
        uint8_t c = static_cast<uint8_t>(value[i]);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            encoded += static_cast<char>(c);
        else if (c == ' ')
            encoded += '+';
        else
        {
            encoded += '%';
            encoded += hex[c >> 4];
            encoded += hex[c & 0x0F];
        }
    }

    return encoded;
}

static bool downloadImage(const String &url)
{
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    if (!http.begin(client, url))
        return false;

    http.setTimeout(5000);
    int status = http.GET();
    if (status != HTTP_CODE_OK)
    {
        http.end();
        return false;
    }

    int length = http.getSize();
    if (length <= 0 || length > 65536)
    {
        http.end();
        return false;
    }

    logoData.resize(length);
    WiFiClient *stream = http.getStreamPtr();
    size_t received = 0;
    unsigned long readStarted = millis();
    while (received < static_cast<size_t>(length))
    {
        if (millis() - readStarted >= 5000)
        {
            http.end();
            logoData.clear();
            return false;
        }

        int available = stream->available();
        if (available <= 0)
        {
            delay(1);
            continue;
        }

        size_t wanted = min(static_cast<size_t>(available),
                            static_cast<size_t>(length) - received);
        size_t bytesRead = stream->readBytes(logoData.data() + received, wanted);
        received += bytesRead;
        if (bytesRead > 0)
            readStarted = millis();
    }

    http.end();
    logoIsJpeg = logoData.size() > 3 && logoData[0] == 0xFF && logoData[1] == 0xD8;
    return logoIsJpeg;
}

static bool loadLocalLogo(const String &path)
{
    File file = SPIFFS.open(path, FILE_READ);
    if (!file || file.size() <= 0 || file.size() > 65536)
    {
        if (file)
            file.close();
        return false;
    }

    logoData.resize(file.size());
    size_t bytesRead = file.read(logoData.data(), logoData.size());
    file.close();

    logoIsJpeg = bytesRead > 3 && logoData[0] == 0xFF && logoData[1] == 0xD8;
    if (!logoIsJpeg)
        logoData.clear();
    return logoIsJpeg;
}

static bool loadStationLogo(const String &station)
{
    logoData.clear();
    logoIsJpeg = false;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient lookup;
    String lookupUrl = "https://de1.api.radio-browser.info/json/stations/bynameexact/";
    lookupUrl += urlEncode(station);
    lookupUrl += "?limit=1";

    if (!lookup.begin(client, lookupUrl))
        return false;

    lookup.setTimeout(5000);
    if (lookup.GET() != HTTP_CODE_OK)
    {
        lookup.end();
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, lookup.getStream());
    String favicon;
    if (!error && doc.is<JsonArray>() && doc.size() > 0)
        favicon = doc[0]["favicon"].as<String>();
    lookup.end();

    if (!favicon.length())
        return false;

    return downloadImage(favicon);
}

static void logoWorker(void *)
{
    String station = logoRequestStation;
    String logoUrl = logoRequestUrl;
    bool loaded = false;

    for (uint8_t attempt = 1; attempt <= 2 && !loaded; ++attempt)
    {
        if (logoUrl.startsWith("/"))
            loaded = loadLocalLogo(logoUrl);
        else if (logoUrl.length())
            loaded = downloadImage(logoUrl);
        else
            loaded = loadStationLogo(station);

        if (!loaded && attempt < 2)
            vTaskDelay(pdMS_TO_TICKS(250));
    }

    if (loaded)
    {
        logoLoadedStation = station;
        logoLoadedUrl = logoUrl;
    }
    else
    {
        logoData.clear();
        logoIsJpeg = false;
        logoLoadedStation = station;
        logoLoadedUrl = logoUrl;
    }

    logoLoading = false;
    logoTaskHandle = nullptr;
    audioChanged = true;
    vTaskDelete(nullptr);
}

static void drawFallbackLogo(const String &station)
{
    tft.fillRoundRect(175, 105, 55, 55, 6, ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(182, 125);
    tft.print(station.substring(0, 8));
}

static void drawLogoStatus(const char *text)
{
    tft.fillRoundRect(175, 105, 55, 55, 6, ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(180, 125);
    tft.print(text);
}

void drawStationLogo(const String &station, const String &logoUrl)
{
    if (station != loadedStation)
    {
        loadedStation = station;
    }

    if ((logoLoadedStation != station || logoLoadedUrl != logoUrl) && !logoLoading)
    {
        logoRequestStation = station;
        logoRequestUrl = logoUrl;
        logoLoading = true;
        if (xTaskCreatePinnedToCore(logoWorker, "logo", 12288, nullptr, 1,
                                    &logoTaskHandle, 0) != pdPASS)
        {
            logoLoading = false;
            logoTaskHandle = nullptr;
        }
    }

    if (logoLoading)
    {
        drawLogoStatus("Loading");
        return;
    }

    tft.fillRoundRect(175, 105, 55, 55, 6, ST77XX_WHITE);
    if (logoLoadedStation != station || !logoIsJpeg || logoData.empty())
    {
        drawFallbackLogo(station);
        return;
    }

    TJpgDec.setJpgScale(2);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(drawJpegBlock);
    int result = TJpgDec.drawJpg(176, 106, logoData.data(), logoData.size());
    if (result != 0)
        drawFallbackLogo(station);
}