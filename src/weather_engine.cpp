#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "project_config.h"
#include "weather_engine.h"


static String temp = "--";
static String desc = "Waiting...";
static String humidity = "--";
static String wind = "--";
static int code = -1;

static unsigned long lastUpdate = 0;
static TaskHandle_t weatherTaskHandle = nullptr;
static volatile bool weatherRequestPending = false;

static const uint32_t WEATHER_REQUEST_TIMEOUT_MS = 5000;
static const uint8_t WEATHER_MAX_ATTEMPTS = 2;

const unsigned long WEATHER_UPDATE_INTERVAL =
    15UL * 60UL * 1000UL;

bool weatherInit()
{
    return true;
}

void weatherLoop()
{
    if (!weatherRequestPending && millis() - lastUpdate >= WEATHER_UPDATE_INTERVAL)
    {
        lastUpdate = millis();
        weatherRequestPending = true;
        xTaskCreatePinnedToCore([](void *) {
            for (uint8_t attempt = 1; attempt <= WEATHER_MAX_ATTEMPTS; ++attempt)
            {
                if (weatherUpdate())
                    break;
                if (attempt < WEATHER_MAX_ATTEMPTS)
                    vTaskDelay(pdMS_TO_TICKS(250));
            }
            weatherRequestPending = false;
            weatherTaskHandle = nullptr;
            vTaskDelete(nullptr);
        }, "weather", 8192, nullptr, 1, &weatherTaskHandle, 0);
    }
}

bool weatherUpdate()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Weather: No WiFi");
        return false;
    }

    HTTPClient http;

    String url =
        "https://api.open-meteo.com/v1/forecast?"
        "latitude=" + String(LATITUDE, 4) +
        "&longitude=" + String(LONGITUDE, 4) +
        "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code";

    Serial.println();
    Serial.println("Weather Request:");
    Serial.println(url);

    http.begin(url);
    http.setTimeout(WEATHER_REQUEST_TIMEOUT_MS);

    int httpCode = http.GET();

    Serial.print("HTTP Response: ");
    Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK)
{
    String payload = http.getString();

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
    {
        temp = String(doc["current"]["temperature_2m"].as<float>(), 1) + " C";
        code = doc["current"]["weather_code"].as<int>();

        humidity =
            String(doc["current"]["relative_humidity_2m"].as<int>()) + "%";

        wind =
            String(doc["current"]["wind_speed_10m"].as<float>(), 1) + " km/h";

        switch (code)
        {
            case 0: desc = "Clear sky"; break;
            case 1: desc = "Mainly clear"; break;
            case 2: desc = "Partly cloudy"; break;
            case 3: desc = "Overcast"; break;
            case 45:
            case 48: desc = "Fog"; break;
            case 51:
            case 53:
            case 55: desc = "Drizzle"; break;
            case 61:
            case 63:
            case 65: desc = "Rain"; break;
            case 71:
            case 73:
            case 75: desc = "Snow"; break;
            case 80:
            case 81:
            case 82: desc = "Showers"; break;
            case 95:
            case 96:
            case 99: desc = "Thunderstorm"; break;
            default: desc = "Current Conditions"; break;
        }

        Serial.println();
        Serial.println("Weather Parsed");

        Serial.print("Temperature: ");
        Serial.println(temp);

        Serial.print("Humidity: ");
        Serial.println(humidity);

        Serial.print("Wind: ");
        Serial.println(wind);
    }
    else
    {
        Serial.println("JSON Parse Failed");
    }

    http.end();
    return true;

    }

    http.end();

    return false;
}

String weatherTemperature()
{
    return temp;
}

String weatherDescription()
{
    return desc;
}

String weatherHumidity()
{
    return humidity;
}

String weatherWind()
{
    return wind;
}

int weatherCode()
{
    return code;
}

bool weatherValid()
{
    return code >= 0;
}