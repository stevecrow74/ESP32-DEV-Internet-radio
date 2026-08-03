#pragma once

#include <Arduino.h>

struct FavStation {
    String name;
    String url;
};

void favsInit();
int favsCount();
bool favsAdd(const String &name, const String &url);
bool favsRemove(int index);
FavStation favsGet(int index);
String favsListJson();
