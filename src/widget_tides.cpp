#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string.h>

#include "display.h"
#include "widget_tides.h"
#include "tides_config.h"

extern Adafruit_ST7789 tft;

static char tideStatus[24];
static char tideTrend[16];
static char tideLevel[16];
static char tideNextHigh[8];
static char tideNextLow[8];
static unsigned long lastFetchMs = 0;

static void setText(char *dst, size_t dstSize, const char *src)
{
	if (!dst || dstSize == 0)
		return;
	if (!src)
		src = "";
	strncpy(dst, src, dstSize - 1);
	dst[dstSize - 1] = '\0';
}

static String formatHm(time_t ts)
{
	if (ts <= 0)
		return "--:--";

	struct tm tmLocal;
	localtime_r(&ts, &tmLocal);
	char buf[8];
	strftime(buf, sizeof(buf), "%H:%M", &tmLocal);
	return String(buf);
}

static bool parseIsoLocal(const char *iso, time_t &out)
{
	if (!iso)
		return false;

	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int minute = 0;

	if (sscanf(iso, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) != 5)
		return false;

	struct tm tmLocal = {};
	tmLocal.tm_year = year - 1900;
	tmLocal.tm_mon = month - 1;
	tmLocal.tm_mday = day;
	tmLocal.tm_hour = hour;
	tmLocal.tm_min = minute;
	tmLocal.tm_isdst = -1;

	out = mktime(&tmLocal);
	return out > 0;
}

static bool fetchTides()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		setText(tideStatus, sizeof(tideStatus), "No WiFi");
		return false;
	}

	String url =
		"https://marine-api.open-meteo.com/v1/marine?latitude=" + String(tidesLatitude(), 4) +
		"&longitude=" + String(tidesLongitude(), 4) +
		"&hourly=sea_level_height_msl&forecast_days=3&timezone=auto";

	HTTPClient http;
	http.begin(url);
	int httpCode = http.GET();

	if (httpCode != HTTP_CODE_OK)
	{
		setText(tideStatus, sizeof(tideStatus), "Fetch failed");
		http.end();
		return false;
	}

	String payload = http.getString();
	http.end();

	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, payload);
	if (err)
	{
		setText(tideStatus, sizeof(tideStatus), "Parse failed");
		return false;
	}

	JsonArray times = doc["hourly"]["time"].as<JsonArray>();
	JsonArray levels = doc["hourly"]["sea_level_height_msl"].as<JsonArray>();

	if (times.isNull() || levels.isNull() || times.size() < 3 || levels.size() < 3)
	{
		setText(tideStatus, sizeof(tideStatus), "No tide data");
		return false;
	}

	const int maxSamples = 120;
	int sampleCount = min((int)times.size(), (int)levels.size());
	sampleCount = min(sampleCount, maxSamples);

	float seaLevel[maxSamples];
	time_t sampleTime[maxSamples];
	bool valid[maxSamples];

	for (int i = 0; i < sampleCount; i++)
	{
		seaLevel[i] = levels[i].isNull() ? NAN : levels[i].as<float>();
		valid[i] = !isnan(seaLevel[i]);

		const char *t = times[i].as<const char *>();
		time_t ts = 0;
		if (!parseIsoLocal(t, ts))
			valid[i] = false;
		sampleTime[i] = ts;
	}

	time_t nowTs = time(nullptr);
	bool haveClock = nowTs > 100000;

	int nowIdx = 0;
	if (haveClock)
	{
		for (int i = 0; i < sampleCount; i++)
		{
			if (valid[i] && sampleTime[i] >= nowTs)
			{
				nowIdx = i;
				break;
			}
		}
	}

	int nextValidIdx = nowIdx;
	while (nextValidIdx < sampleCount && !valid[nextValidIdx])
		nextValidIdx++;

	if (nextValidIdx >= sampleCount)
	{
		setText(tideStatus, sizeof(tideStatus), "No valid data");
		return false;
	}

	snprintf(tideLevel, sizeof(tideLevel), "%.2f m", seaLevel[nextValidIdx]);

	int trendNextIdx = nextValidIdx + 1;
	while (trendNextIdx < sampleCount && !valid[trendNextIdx])
		trendNextIdx++;

	if (trendNextIdx < sampleCount)
	{
		float delta = seaLevel[trendNextIdx] - seaLevel[nextValidIdx];
		if (delta > 0.02f)
			setText(tideTrend, sizeof(tideTrend), "Rising");
		else if (delta < -0.02f)
			setText(tideTrend, sizeof(tideTrend), "Falling");
		else
			setText(tideTrend, sizeof(tideTrend), "Steady");
	}
	else
	{
		setText(tideTrend, sizeof(tideTrend), "--");
	}

	time_t highTs = 0;
	time_t lowTs = 0;

	int startIdx = max(1, nextValidIdx);
	for (int i = startIdx; i < sampleCount - 1; i++)
	{
		if (!valid[i - 1] || !valid[i] || !valid[i + 1])
			continue;

		bool isHigh = (seaLevel[i] >= seaLevel[i - 1] && seaLevel[i] > seaLevel[i + 1]);
		bool isLow = (seaLevel[i] <= seaLevel[i - 1] && seaLevel[i] < seaLevel[i + 1]);

		if (highTs == 0 && isHigh)
			highTs = sampleTime[i];
		if (lowTs == 0 && isLow)
			lowTs = sampleTime[i];

		if (highTs != 0 && lowTs != 0)
			break;
	}

	setText(tideNextHigh, sizeof(tideNextHigh), formatHm(highTs).c_str());
	setText(tideNextLow, sizeof(tideNextLow), formatHm(lowTs).c_str());
	setText(tideStatus, sizeof(tideStatus), (highTs != 0 || lowTs != 0) ? "Updated" : "Partial data");
	return true;
}

static void drawLabelValue(int y, const char *label, const char *value)
{
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(2);
	tft.setCursor(10, y);
	tft.print(label);

	tft.setCursor(125, y);
	tft.print(value);
}

void tidesWidgetInit()
{
	tidesConfigInit();

	setText(tideStatus, sizeof(tideStatus), "Waiting...");
	setText(tideTrend, sizeof(tideTrend), "--");
	setText(tideLevel, sizeof(tideLevel), "--");
	setText(tideNextHigh, sizeof(tideNextHigh), "--:--");
	setText(tideNextLow, sizeof(tideNextLow), "--:--");
	lastFetchMs = 0;
}

void tidesWidgetLoop()
{
	if (millis() - lastFetchMs >= tidesRefreshMs())
	{
		if (fetchTides())
			lastFetchMs = millis();
		else
			lastFetchMs = millis();
	}
}

void tidesWidgetDraw()
{
	tft.fillRect(0, 100, 240, 220, ST77XX_BLACK);

	tft.setTextColor(ST77XX_CYAN);
	tft.setTextSize(2);
	tft.setCursor(70, 110);
	tft.print("TIDES");

	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(1);
	tft.setCursor(10, 135);
	tft.print(tidesLocation());

	tft.setCursor(10, 147);
	tft.print(tideStatus);

	drawLabelValue(160, "Trend", tideTrend);
	drawLabelValue(185, "Level", tideLevel);
	drawLabelValue(210, "High", tideNextHigh);
	drawLabelValue(235, "Low", tideNextLow);

	tft.setTextSize(1);
	tft.setCursor(10, 265);
	tft.print("Update ");
	tft.print(tidesRefreshMs() / 60000UL);
	tft.print(" min");
}

