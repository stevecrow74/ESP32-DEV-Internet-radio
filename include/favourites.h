#pragma once

#include <Arduino.h>

struct FavStation {
    String name;
    String url;
    String logoUrl;
};

void favsInit();
int favsCount();
bool favsAdd(const String &name, const String &url, const String &logoUrl = String());
bool favsSetLogoUrl(int index, const String &logoUrl);
bool favsRemove(int index);
FavStation favsGet(int index);
String favsListJson();
