#pragma once

bool weatherInit();
void weatherLoop();

bool weatherUpdate();

String weatherTemperature();
String weatherDescription();
String weatherHumidity();
String weatherWind();
int weatherCode();

bool weatherValid();