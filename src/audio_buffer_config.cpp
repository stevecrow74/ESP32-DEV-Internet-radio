#include "audio_buffer_config.h"

#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "project_config.h"

static const char *AUDIO_BUFFER_CONFIG_FILE = "/audio_buffer_config.json";

static int configRamBytes = AUDIO_BUFFER_RAM_BYTES;
static int configPsramBytes = AUDIO_BUFFER_PSRAM_BYTES;

static void saveConfig()
{
    JsonDocument doc;
    doc["ramBytes"] = configRamBytes;
    doc["psramBytes"] = configPsramBytes;

    File f = SPIFFS.open(AUDIO_BUFFER_CONFIG_FILE, FILE_WRITE);
    if (!f)
    {
        Serial.println("[AUDIOBUF] failed to open config for write");
        return;
    }

    serializeJsonPretty(doc, f);
    f.close();
}

void audioBufferConfigInit()
{
    if (!SPIFFS.exists(AUDIO_BUFFER_CONFIG_FILE))
    {
        saveConfig();
        return;
    }

    File f = SPIFFS.open(AUDIO_BUFFER_CONFIG_FILE, FILE_READ);
    if (!f)
    {
        Serial.println("[AUDIOBUF] failed to open config for read");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        Serial.print("[AUDIOBUF] config parse failed: ");
        Serial.println(err.c_str());
        return;
    }

    configRamBytes = doc["ramBytes"] | AUDIO_BUFFER_RAM_BYTES;
    configPsramBytes = doc["psramBytes"] | AUDIO_BUFFER_PSRAM_BYTES;
}

int audioBufferRamBytes()
{
    return configRamBytes;
}

int audioBufferPsramBytes()
{
    return configPsramBytes;
}

void audioBufferConfigUpdate(int ramBytes, int psramBytes)
{
    configRamBytes = ramBytes;
    configPsramBytes = psramBytes;
    saveConfig();
}
