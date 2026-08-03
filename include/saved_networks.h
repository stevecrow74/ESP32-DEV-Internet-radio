#pragma once

#include <Arduino.h>

struct SavedNetwork
{
    String ssid;
    String password;
};

void savedNetworksInit();
int savedNetworksCount();
SavedNetwork savedNetworksGet(int index);
bool savedNetworksAdd(const String &ssid, const String &password);
bool savedNetworksRemove(int index);
String savedNetworksListJson();
