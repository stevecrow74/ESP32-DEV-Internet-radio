#pragma once

struct RadioStation
{
    const char *name;
    const char *url;
};

void stationInit();
const RadioStation *stationCurrent();

void stationNext();
void stationPrevious();

void stationConnectCurrent();
void stationPlayUrl(const char *name, const char *url);