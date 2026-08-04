#include <Arduino.h>

#include "display.h"
#include "widget_manager.h"
#include "audio_engine.h"
#include "wifi_manager.h"
#include "audio_state.h"
#include "station_manager.h"
#include "encoders.h"
#include "weather_engine.h"
#include "webui.h"
#include <SPIFFS.h>

void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println("====================================");
    Serial.println("     ESP32 SMART RADIO V2");
    Serial.println("====================================");
    Serial.println("[BOOT] setup start");

    // Initialize SPIFFS for persistent storage (favourites, web assets)
    if(!SPIFFS.begin(true)) {
        Serial.println("[WARN] SPIFFS failed to mount, continuing");
    }
    Serial.println("[BOOT] SPIFFS ready");

    displayInit();
    drawSplashScreen();
    Serial.println("[BOOT] display ready");

    wifiConnect();
    Serial.println("[BOOT] wifi attempted");

    audioInit();
    Serial.println("[BOOT] audio ready");

    stationInit();
    Serial.println("[BOOT] station manager ready");

    encodersInit();
    Serial.println("[BOOT] encoders ready");

    widgetInit();
    Serial.println("[BOOT] widgets ready");

    stationConnectCurrent();
    Serial.println("[BOOT] station connect attempted");

    // Start web UI to expose favourites management
    webuiInit();
    Serial.println("[BOOT] webui ready");

    weatherInit();
    weatherUpdate();
    Serial.println("[BOOT] weather ready");

    delay(5000);
    widgetDraw();

    Serial.println("System Ready");
}

void loop()
{
    encodersLoop();

    displayLoop();

    widgetLoop();

    audioLoop();

    webuiLoop();

    weatherLoop();
}
