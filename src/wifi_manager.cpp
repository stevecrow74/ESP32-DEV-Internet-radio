/******************************************************************************
 *
 * wifi_manager.cpp
 *
 ******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "wifi_manager.h"

static WiFiManager wm;

bool wifiConnect()
{
    Serial.println("Connecting to WiFi...");

    if (!wm.autoConnect("ESP32-Radio"))
    {
        Serial.println("WiFi connection failed.");
        return false;
    }

    Serial.println("WiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    return true;
}

bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String wifiIPAddress()
{
    return WiFi.localIP().toString();
}

void wifiLoop()
{
    // Reserved for future use
}