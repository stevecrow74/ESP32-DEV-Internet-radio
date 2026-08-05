#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <string.h>
#include <ctype.h>

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

static void setHttpStatus(int code)
{
	snprintf(tideStatus, sizeof(tideStatus), "HTTP %d", code);
}

static void setText(char *dst, size_t dstSize, const char *src)
{
	if (!dst || dstSize == 0)
		return;
	if (!src)
		src = "";
	strncpy(dst, src, dstSize - 1);
	dst[dstSize - 1] = '\0';
}

static String trimCopy(const String &in)
{
	int a = 0;
	int b = in.length() - 1;

	while (a <= b && isspace((unsigned char)in[a]))
		a++;
	while (b >= a && isspace((unsigned char)in[b]))
		b--;

	if (b < a)
		return "";

	return in.substring(a, b + 1);
}

static String collapseWhitespace(const String &in)
{
	String out;
	out.reserve(in.length());
	bool prevSpace = false;

	for (size_t i = 0; i < in.length(); i++)
	{
		char c = in[i];
		bool isSp = isspace((unsigned char)c);
		if (isSp)
		{
			if (!prevSpace)
				out += ' ';
			prevSpace = true;
		}
		else
		{
			out += c;
			prevSpace = false;
		}
	}

	return trimCopy(out);
}

static String stripHtml(const String &in)
{
	String out;
	out.reserve(in.length());
	bool inTag = false;

	for (size_t i = 0; i < in.length(); i++)
	{
		char c = in[i];
		if (c == '<')
		{
			inTag = true;
			continue;
		}
		if (c == '>')
		{
			inTag = false;
			continue;
		}
		if (!inTag)
			out += c;
	}

	out.replace("&nbsp;", " ");
	out.replace("&amp;", "&");
	out.replace("&deg;", "");
	return collapseWhitespace(out);
}

static bool extractBetween(const String &src, const char *startToken, const char *endToken, String &out, int fromPos = 0)
{
	if (!startToken || !endToken)
		return false;

	int a = src.indexOf(startToken, fromPos);
	if (a < 0)
		return false;
	a += strlen(startToken);

	int b = src.indexOf(endToken, a);
	if (b < 0)
		return false;

	out = src.substring(a, b);
	return true;
}

static bool readMarkerText(const String &html, const char *id, String &out)
{
	if (!id)
		return false;

	String idDq = String("id=\"") + id + "\"";
	String idSq = String("id='") + id + "'";

	int idPos = html.indexOf(idDq);
	if (idPos < 0)
		idPos = html.indexOf(idSq);
	if (idPos < 0)
		return false;

	int tagEnd = html.indexOf('>', idPos);
	if (tagEnd < 0)
		return false;

	int textStart = tagEnd + 1;
	while (textStart < (int)html.length() && isspace((unsigned char)html[textStart]))
		textStart++;

	int textEnd = html.indexOf('<', textStart);
	if (textEnd < 0)
		return false;

	out = html.substring(textStart, textEnd);
	out = stripHtml(out);
	return out.length() > 0;
}

static bool parse24hMinutes(const String &hhmm, int &mins)
{
	int colon = hhmm.indexOf(':');
	if (colon < 1)
		return false;

	int h = hhmm.substring(0, colon).toInt();
	int m = hhmm.substring(colon + 1).toInt();
	if (h < 0 || h > 23 || m < 0 || m > 59)
		return false;

	mins = h * 60 + m;
	return true;
}

static bool extractFirstTime24(const String &text, String &out)
{
	for (size_t i = 0; i + 4 < text.length(); i++)
	{
		char c0 = text[i + 0];
		char c1 = text[i + 1];
		char c2 = text[i + 2];
		char c3 = text[i + 3];
		char c4 = text[i + 4];

		if (isdigit((unsigned char)c0) && isdigit((unsigned char)c1) && c2 == ':' &&
			isdigit((unsigned char)c3) && isdigit((unsigned char)c4))
		{
			out = text.substring(i, i + 5);
			return true;
		}
	}

	return false;
}

static bool parse12hMinutes(const String &tRaw, int &mins)
{
	String t = trimCopy(tRaw);
	String tl = t;
	tl.toLowerCase();

	bool hasAm = tl.endsWith("am");
	bool hasPm = tl.endsWith("pm");
	if (!hasAm && !hasPm)
		return false;

	String core = trimCopy(tl.substring(0, tl.length() - 2));
	int colon = core.indexOf(':');
	if (colon < 1)
		return false;

	int h = core.substring(0, colon).toInt();
	int m = core.substring(colon + 1).toInt();
	if (h < 1 || h > 12 || m < 0 || m > 59)
		return false;

	if (h == 12)
		h = 0;
	if (hasPm)
		h += 12;

	mins = h * 60 + m;
	return true;
}

static int parseCountdownMinutes(const String &raw)
{
	String s = raw;
	s.toLowerCase();

	int total = 0;
	int hPos = s.indexOf("hr");
	if (hPos > 0)
	{
		int start = hPos - 1;
		while (start >= 0 && isdigit((unsigned char)s[start]))
			start--;
		total += s.substring(start + 1, hPos).toInt() * 60;
	}

	int mPos = s.indexOf("min");
	if (mPos > 0)
	{
		int start = mPos - 1;
		while (start >= 0 && isdigit((unsigned char)s[start]))
			start--;
		total += s.substring(start + 1, mPos).toInt();
	}

	return total;
}

struct TideEvent
{
	String state;
	int minutesOfDay;
	String heightM;
};

static bool parseTideEvents(const String &html, TideEvent *events, int maxEvents, int &count)
{
	count = 0;
	if (!events || maxEvents <= 0)
		return false;

	int todayStart = html.indexOf("<section id=\"tide-today-section\"");
	if (todayStart < 0)
		return false;

	int nextTides = html.indexOf("<div class=\"pure-g\" id=\"next-tides\"", todayStart);
	if (nextTides < 0)
		return false;

	String section = html.substring(todayStart, nextTides);
	int pos = 0;

	while (count < maxEvents)
	{
		String stateRaw;
		if (!extractBetween(section, "<td class=\"tidal-state\">", "</td>", stateRaw, pos))
			break;

		int stateIdx = section.indexOf("<td class=\"tidal-state\">", pos);
		if (stateIdx < 0)
			break;
		pos = stateIdx + 1;

		String timeRaw;
		String heightRaw;
		if (!extractBetween(section, "<td class=\"tidal-time\">", "</td>", timeRaw, stateIdx))
			continue;
		if (!extractBetween(section, "<span class=\"m\">", "</span>", heightRaw, stateIdx))
			continue;

		String state = stripHtml(stateRaw);
		String timeText = stripHtml(timeRaw);
		String height = stripHtml(heightRaw);

		int mins = -1;
		if (!parse12hMinutes(timeText, mins))
			continue;

		state.toLowerCase();
		events[count].state = state;
		events[count].minutesOfDay = mins;
		events[count].heightM = height;
		count++;
	}

	return count > 0;
}

static String findHeightForNext(const TideEvent *events, int count, const String &state, int minutesOfDay)
{
	if (!events || count <= 0)
		return "";

	String st = state;
	st.toLowerCase();

	for (int i = 0; i < count; i++)
	{
		if (events[i].state == st && events[i].minutesOfDay == minutesOfDay)
			return events[i].heightM;
	}

	for (int i = 0; i < count; i++)
	{
		if (events[i].state == st)
			return events[i].heightM;
	}

	return "";
}

static bool parseTideTimePage(const String &html)
{
	String nextHighText;
	String nextLowText;
	String nextHighIn;
	String nextLowIn;
	String tideStateText;

	if (!readMarkerText(html, "next-high", nextHighText) ||
		!readMarkerText(html, "next-low", nextLowText) ||
		!readMarkerText(html, "next-high-in", nextHighIn) ||
		!readMarkerText(html, "next-low-in", nextLowIn) ||
		!readMarkerText(html, "tide-state", tideStateText))
		return false;

	int nextHighMins = -1;
	int nextLowMins = -1;
	if (!parse24hMinutes(nextHighText, nextHighMins) || !parse24hMinutes(nextLowText, nextLowMins))
		return false;

	TideEvent events[12];
	int eventCount = 0;
	parseTideEvents(html, events, 12, eventCount);

	int highInMins = parseCountdownMinutes(nextHighIn);
	int lowInMins = parseCountdownMinutes(nextLowIn);

	String level;
	if (highInMins > 0 && lowInMins > 0)
	{
		level = (highInMins <= lowInMins) ? findHeightForNext(events, eventCount, "high", nextHighMins)
									 : findHeightForNext(events, eventCount, "low", nextLowMins);
	}
	else
	{
		level = findHeightForNext(events, eventCount, "high", nextHighMins);
		if (level.length() == 0)
			level = findHeightForNext(events, eventCount, "low", nextLowMins);
	}

	if (level.length() > 0)
	{
		if (!level.endsWith("m"))
			level += "m";
		snprintf(tideLevel, sizeof(tideLevel), "%s", level.c_str());
	}
	else
	{
		setText(tideLevel, sizeof(tideLevel), "--");
	}

	String trend = tideStateText;
	trend.toLowerCase();
	if (trend.indexOf("coming in") >= 0 || trend.indexOf("rising") >= 0)
		setText(tideTrend, sizeof(tideTrend), "Rising");
	else if (trend.indexOf("going out") >= 0 || trend.indexOf("falling") >= 0)
		setText(tideTrend, sizeof(tideTrend), "Falling");
	else
		setText(tideTrend, sizeof(tideTrend), "Steady");

	snprintf(tideNextHigh, sizeof(tideNextHigh), "%s", nextHighText.c_str());
	snprintf(tideNextLow, sizeof(tideNextLow), "%s", nextLowText.c_str());

	setText(tideStatus, sizeof(tideStatus), "Updated");
	return true;
}

static bool parseTideTimePageFallback(const String &html)
{
	String highChunk;
	String lowChunk;
	String tideStateText;
	String nextHighText;
	String nextLowText;

	if (!extractBetween(html, "Next high tide:", "</", highChunk))
		return false;
	if (!extractBetween(html, "Next low tide:", "</", lowChunk))
		return false;

	highChunk = stripHtml(highChunk);
	lowChunk = stripHtml(lowChunk);

	if (!extractFirstTime24(highChunk, nextHighText) || !extractFirstTime24(lowChunk, nextLowText))
		return false;

	if (readMarkerText(html, "tide-state", tideStateText))
	{
		String trend = tideStateText;
		trend.toLowerCase();
		if (trend.indexOf("coming in") >= 0 || trend.indexOf("rising") >= 0)
			setText(tideTrend, sizeof(tideTrend), "Rising");
		else if (trend.indexOf("going out") >= 0 || trend.indexOf("falling") >= 0)
			setText(tideTrend, sizeof(tideTrend), "Falling");
		else
			setText(tideTrend, sizeof(tideTrend), "Steady");
	}
	else
	{
		setText(tideTrend, sizeof(tideTrend), "--");
	}

	setText(tideLevel, sizeof(tideLevel), "--");
	snprintf(tideNextHigh, sizeof(tideNextHigh), "%s", nextHighText.c_str());
	snprintf(tideNextLow, sizeof(tideNextLow), "%s", nextLowText.c_str());
	setText(tideStatus, sizeof(tideStatus), "Updated*");
	return true;
}

static String formatMinutes24(int mins)
{
	if (mins < 0)
		return "--:--";
	mins %= 1440;
	if (mins < 0)
		mins += 1440;

	char buf[8];
	snprintf(buf, sizeof(buf), "%02d:%02d", mins / 60, mins % 60);
	return String(buf);
}

static String rssUrlFromSource(const String &sourceUrl)
{
	if (sourceUrl.endsWith(".rss"))
		return sourceUrl;

	int htmPos = sourceUrl.indexOf(".htm");
	if (htmPos >= 0)
		return sourceUrl.substring(0, htmPos) + ".rss";

	if (sourceUrl.endsWith("/"))
		return sourceUrl + "index.rss";

	return sourceUrl + ".rss";
}

static bool fetchTextUrl(const String &url, String &payload, int &httpCode)
{
	HTTPClient http;
	http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
	http.setTimeout(15000);
	http.setReuse(false);
	http.setUserAgent("Mozilla/5.0 (ESP32-Radio; Tides)");
	http.addHeader("Accept-Encoding", "identity");

	bool beginOk = false;
	WiFiClient plainClient;
	WiFiClientSecure secureClient;

	if (url.startsWith("https://"))
	{
		secureClient.setInsecure();
		beginOk = http.begin(secureClient, url);
	}
	else
	{
		beginOk = http.begin(plainClient, url);
	}

	if (!beginOk)
	{
		httpCode = -1;
		return false;
	}

	httpCode = http.GET();
	if (httpCode == HTTP_CODE_OK)
		payload = http.getString();

	http.end();
	return true;
}

static bool parseRssDescription(const String &rss)
{
	String desc;
	int searchPos = 0;

	// RSS has a channel-level <description> before item descriptions.
	// We need the first item description that actually contains tide rows.
	while (true)
	{
		int a = rss.indexOf("<description>", searchPos);
		if (a < 0)
			break;
		a += 13;
		int b = rss.indexOf("</description>", a);
		if (b < 0)
			break;

		String cand = rss.substring(a, b);
		if (cand.indexOf("High - ") >= 0 || cand.indexOf("Low - ") >= 0)
		{
			desc = cand;
			break;
		}

		searchPos = b + 14;
	}

	if (desc.length() == 0)
		return false;

	desc.replace("&lt;", "<");
	desc.replace("&gt;", ">");
	desc.replace("&amp;", "&");
	desc.replace("&#39;", "'");
	desc.replace("&quot;", "\"");

	struct Event
	{
		bool isHigh;
		int minutes;
		String height;
	};

	Event events[8];
	int n = 0;
	int pos = 0;

	while (n < 8)
	{
		int pHigh = desc.indexOf("High - ", pos);
		int pLow = desc.indexOf("Low - ", pos);
		if (pHigh < 0 && pLow < 0)
			break;

		bool isHigh = false;
		int p = -1;
		if (pHigh >= 0 && (pLow < 0 || pHigh < pLow))
		{
			isHigh = true;
			p = pHigh;
		}
		else
		{
			isHigh = false;
			p = pLow;
		}

		int tStart = p + (isHigh ? 7 : 6);
		while (tStart < (int)desc.length() && isspace((unsigned char)desc[tStart]))
			tStart++;

		int tEnd = desc.indexOf(' ', tStart);
		if (tEnd < 0)
			break;

		String timeText = trimCopy(desc.substring(tStart, tEnd));
		int mins = -1;
		if (!parse12hMinutes(timeText, mins))
		{
			pos = tEnd + 1;
			continue;
		}

		int hStart = tEnd + 1;
		while (hStart < (int)desc.length() && isspace((unsigned char)desc[hStart]))
			hStart++;
		int hEnd = desc.indexOf('<', hStart);
		if (hEnd < 0)
			hEnd = desc.length();

		String h = trimCopy(desc.substring(hStart, hEnd));
		events[n].isHigh = isHigh;
		events[n].minutes = mins;
		events[n].height = h;
		n++;
		pos = hEnd;
	}

	if (n == 0)
		return false;

	time_t nowTs = time(nullptr);
	struct tm tmNow;
	int nowMinutes = -1;
	if (localtime_r(&nowTs, &tmNow))
		nowMinutes = tmNow.tm_hour * 60 + tmNow.tm_min;

	int chosenHigh = -1;
	int chosenLow = -1;
	int bestHighDelta = 2000;
	int bestLowDelta = 2000;

	for (int i = 0; i < n; i++)
	{
		int delta = (nowMinutes >= 0) ? (events[i].minutes - nowMinutes) : 0;
		if (delta < 0)
			delta += 1440;

		if (events[i].isHigh)
		{
			if (chosenHigh < 0 || delta < bestHighDelta)
			{
				chosenHigh = i;
				bestHighDelta = delta;
			}
		}
		else
		{
			if (chosenLow < 0 || delta < bestLowDelta)
			{
				chosenLow = i;
				bestLowDelta = delta;
			}
		}
	}

	if (chosenHigh >= 0)
		setText(tideNextHigh, sizeof(tideNextHigh), formatMinutes24(events[chosenHigh].minutes).c_str());
	if (chosenLow >= 0)
		setText(tideNextLow, sizeof(tideNextLow), formatMinutes24(events[chosenLow].minutes).c_str());

	if (chosenHigh >= 0 && chosenLow >= 0)
	{
		if (bestHighDelta <= bestLowDelta)
		{
			setText(tideTrend, sizeof(tideTrend), "Rising");
			String lvl = events[chosenHigh].height;
			if (lvl.length() > 0 && !lvl.endsWith("m"))
				lvl += "m";
			setText(tideLevel, sizeof(tideLevel), lvl.c_str());
		}
		else
		{
			setText(tideTrend, sizeof(tideTrend), "Falling");
			String lvl = events[chosenLow].height;
			if (lvl.length() > 0 && !lvl.endsWith("m"))
				lvl += "m";
			setText(tideLevel, sizeof(tideLevel), lvl.c_str());
		}
	}
	else
	{
		setText(tideTrend, sizeof(tideTrend), "--");
		setText(tideLevel, sizeof(tideLevel), "--");
	}

	setText(tideStatus, sizeof(tideStatus), "Updated RSS");
	return true;
}

static bool fetchTides()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		setText(tideStatus, sizeof(tideStatus), "No WiFi");
		Serial.println("[TIDES] skipped: no WiFi");
		return false;
	}

	HTTPClient http;
	String sourceUrl = tidesSourceUrl();
	sourceUrl = trimCopy(sourceUrl);
	if (sourceUrl.length() == 0)
	{
		setText(tideStatus, sizeof(tideStatus), "No source URL");
		Serial.println("[TIDES] missing source URL");
		return false;
	}

	Serial.println();
	Serial.println("[TIDES] Request:");
	Serial.println(sourceUrl);

	String payload;
	int httpCode = 0;
	if (!fetchTextUrl(sourceUrl, payload, httpCode))
	{
		setText(tideStatus, sizeof(tideStatus), "Begin failed");
		Serial.println("[TIDES] begin failed");
		return false;
	}
	Serial.print("[TIDES] HTTP Response: ");
	Serial.println(httpCode);

	if (httpCode <= 0)
	{
		setText(tideStatus, sizeof(tideStatus), "Conn failed");
		Serial.println("[TIDES] connection failed");
		return false;
	}

	if (httpCode != HTTP_CODE_OK)
	{
		setHttpStatus(httpCode);
		Serial.println("[TIDES] non-200 response");
		return false;
	}

	if (payload.length() >= 2 && (uint8_t)payload[0] == 0x1F && (uint8_t)payload[1] == 0x8B)
	{
		setText(tideStatus, sizeof(tideStatus), "Gzip payload");
		Serial.println("[TIDES] gzip payload received");
		return false;
	}

	if (payload.length() < 200)
	{
		setText(tideStatus, sizeof(tideStatus), "Empty payload");
		Serial.println("[TIDES] payload too small");
		return false;
	}

	if (!parseTideTimePage(payload) && !parseTideTimePageFallback(payload))
	{
		String rssUrl = rssUrlFromSource(sourceUrl);
		String rssPayload;
		int rssCode = 0;

		Serial.println("[TIDES] trying RSS fallback");
		Serial.println(rssUrl);
		if (fetchTextUrl(rssUrl, rssPayload, rssCode) && rssCode == HTTP_CODE_OK && parseRssDescription(rssPayload))
		{
			Serial.println("[TIDES] RSS parsed OK");
			return true;
		}

		Serial.print("[TIDES] RSS HTTP Response: ");
		Serial.println(rssCode);

		String lower = payload;
		lower.toLowerCase();
		if (lower.indexOf("consent") >= 0 || lower.indexOf("manage options") >= 0)
			setText(tideStatus, sizeof(tideStatus), "Blocked page");
		else
			setText(tideStatus, sizeof(tideStatus), "Parse failed");

		Serial.print("[TIDES] marker next-high: ");
		Serial.println(payload.indexOf("next-high") >= 0 ? "yes" : "no");
		Serial.print("[TIDES] marker next-low: ");
		Serial.println(payload.indexOf("next-low") >= 0 ? "yes" : "no");
		Serial.print("[TIDES] marker tide-state: ");
		Serial.println(payload.indexOf("tide-state") >= 0 ? "yes" : "no");
		Serial.println("[TIDES] parse failed");
		return false;
	}

	Serial.println("[TIDES] parsed OK");

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
	unsigned long now = millis();
	if (lastFetchMs == 0 || (now - lastFetchMs >= tidesRefreshMs()))
	{
		fetchTides();
		lastFetchMs = now;
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
