#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPIFFS.h>

#include "webui.h"
#include "favourites.h"
#include "station_manager.h"
#include "adsb_config.h"
#include "saved_networks.h"
#include "tides_config.h"
#include "audio_buffer_config.h"

static WebServer *serverPtr = nullptr;
#define server (*serverPtr)
static char networkStatus[160] = {0};

static void setNetworkStatus(const String &msg)
{
  msg.toCharArray(networkStatus, sizeof(networkStatus));
}

static String renderNav()
{
    return R"nav(
<nav style="margin-bottom:16px;padding-bottom:8px;border-bottom:1px solid #ddd">
  <a href="/" style="margin-right:12px">Home</a>
  <a href="/favourites" style="margin-right:12px">Favourites</a>
  <a href="/adsb" style="margin-right:12px">ADS-B Config</a>
  <a href="/tides" style="margin-right:12px">Tides Config</a>
  <a href="/manual" style="margin-right:12px">Manual</a>
  <a href="/network">Network</a>
</nav>
)nav";
}

static String htmlEscape(const String &in)
{
  String out = in;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'", "&#39;");
  return out;
}

static bool connectToWifi(const String &ssid, const String &password)
{
  if (ssid.length() == 0)
    return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
  {
    delay(250);
  }

  return WiFi.status() == WL_CONNECTED;
}

static void handleNetworkConnect()
{
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  ssid.trim();
  password.trim();

  if (ssid.length() == 0)
  {
    setNetworkStatus("Missing SSID");
    server.sendHeader("Location", "/network");
    server.send(303, "text/plain", "Missing SSID");
    return;
  }

  savedNetworksAdd(ssid, password);

  if (connectToWifi(ssid, password))
  {
    setNetworkStatus("Connected to " + ssid + " (" + WiFi.localIP().toString() + ")");
  }
  else
  {
    setNetworkStatus("Failed to connect to " + ssid);
  }

  server.sendHeader("Location", "/network");
  server.send(303, "text/plain", "OK");
}

  static void handleSavedNetworkList()
  {
    server.send(200, "application/json", savedNetworksListJson());
  }

  static void handleSavedNetworkAdd()
  {
    String ssid;
    String password;

    if (server.hasArg("plain"))
    {
      String body = server.arg("plain");
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, body);
      if (!err)
      {
        ssid = doc["ssid"].as<const char *>() ? String(doc["ssid"].as<const char *>()) : String("");
        password = doc["password"].as<const char *>() ? String(doc["password"].as<const char *>()) : String("");
      }
    }

    if (ssid.length() == 0 && server.hasArg("ssid"))
      ssid = server.arg("ssid");
    if (password.length() == 0 && server.hasArg("password"))
      password = server.arg("password");

    ssid.trim();
    password.trim();

    if (!savedNetworksAdd(ssid, password))
    {
      server.send(400, "text/plain", "missing ssid");
      return;
    }

    server.send(201, "application/json", savedNetworksListJson());
  }

  static void handleSavedNetworkDelete()
  {
    String idxs = server.arg("index");
    if (idxs.length() == 0)
    {
      server.send(400, "text/plain", "missing index");
      return;
    }

    int idx = idxs.toInt();
    if (!savedNetworksRemove(idx))
    {
      server.send(400, "text/plain", "invalid index");
      return;
    }

    server.send(200, "application/json", savedNetworksListJson());
  }

  static void handleSavedNetworkConnect()
  {
    String idxs = server.arg("index");
    if (idxs.length() == 0)
    {
      server.send(400, "text/plain", "missing index");
      return;
    }

    int idx = idxs.toInt();
    SavedNetwork n = savedNetworksGet(idx);
    if (!n.ssid.length())
    {
      server.send(400, "text/plain", "invalid index");
      return;
    }

    if (connectToWifi(n.ssid, n.password))
    {
      setNetworkStatus("Connected to " + n.ssid + " (" + WiFi.localIP().toString() + ")");
    }
    else
    {
      setNetworkStatus("Failed to connect to " + n.ssid);
    }

    server.sendHeader("Location", "/network");
    server.send(303, "text/plain", "OK");
  }

static void handleNetworkApMode()
{
  IPAddress apIp(192, 168, 2, 1);
  IPAddress apGateway(192, 168, 2, 1);
  IPAddress apSubnet(255, 255, 255, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAPdisconnect(true);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);
  WiFi.softAP("ESP32-Radio-AP");

  setNetworkStatus("AP mode enabled at 192.168.2.1 (SSID: ESP32-Radio-AP)");

  server.sendHeader("Location", "/network");
  server.send(303, "text/plain", "OK");
}

static void handleListFavs()
{
    server.send(200, "application/json", favsListJson());
}

static void handleAddFav()
{
    String name;
    String url;

    if (server.hasArg("plain"))
    {
        String body = server.arg("plain");
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (!err)
        {
            name = doc["name"].as<const char *>() ? String(doc["name"].as<const char *>()) : String("");
            url = doc["url"].as<const char *>() ? String(doc["url"].as<const char *>()) : String("");
        }
    }

    // Fallback for x-www-form-urlencoded clients.
    if (name.length() == 0 && server.hasArg("name"))
        name = server.arg("name");
    if (url.length() == 0 && server.hasArg("url"))
        url = server.arg("url");

    name.trim();
    url.trim();

    if (name.length() == 0 || url.length() == 0)
    {
        server.send(400, "text/plain", "missing name or url");
        return;
    }

  favsAdd(name, url);
    server.send(201, "application/json", favsListJson());
}

static void handleDeleteFav()
{
    String idxs = server.arg("index");
    if (idxs.length() == 0)
    {
        server.send(400, "text/plain", "missing index");
        return;
    }

    int idx = idxs.toInt();
    if (!favsRemove(idx))
    {
        server.send(400, "text/plain", "invalid index");
        return;
    }

    server.send(200, "application/json", favsListJson());
}

static void handlePlayFav()
{
    String idxs = server.arg("index");
    if (idxs.length() == 0)
    {
        server.send(400, "text/plain", "missing index");
        return;
    }

    int idx = idxs.toInt();
    if (idx < 0 || idx >= favsCount())
    {
        server.send(400, "text/plain", "invalid index");
        return;
    }

    FavStation s = favsGet(idx);
    stationPlayUrl(s.name.c_str(), s.url.c_str());
    server.send(200, "application/json", favsListJson());
}

static void handleAdsbSave()
{
    uint16_t rangeKm = server.arg("rangeKm").toInt();
    String localFeedUrl = server.arg("localFeedUrl");
    String publicFeedUrl = server.arg("publicFeedUrl");
    String localSsid = server.arg("localSsid");

    if (rangeKm == 0)
        rangeKm = adsbRangeKm();

    adsbConfigUpdate(rangeKm, localFeedUrl, publicFeedUrl, localSsid);
    server.sendHeader("Location", "/adsb");
    server.send(303, "text/plain", "Saved");
}

  static void handleTidesSave()
  {
    String location = server.arg("location");
    double latitude = server.arg("latitude").toDouble();
    double longitude = server.arg("longitude").toDouble();
    uint16_t refreshMinutes = (uint16_t)server.arg("refreshMinutes").toInt();

    if (refreshMinutes == 0)
      refreshMinutes = (uint16_t)(tidesRefreshMs() / 60000UL);

    tidesConfigUpdate(location, latitude, longitude, refreshMinutes);
    server.sendHeader("Location", "/tides");
    server.send(303, "text/plain", "Saved");
  }

static void handleAudioBufferSave()
{
    int ramBytes = server.arg("ramBytes").toInt();
    int psramBytes = server.arg("psramBytes").toInt();

    if (ramBytes < 0)
        ramBytes = -1;
    if (psramBytes < 0)
        psramBytes = -1;

    audioBufferConfigUpdate(ramBytes, psramBytes);
    server.sendHeader("Location", "/network");
    server.send(303, "text/plain", "Saved");
}

void webuiInit()
{
    if (!serverPtr)
    {
        static WebServer serverInstance(80);
        serverPtr = &serverInstance;
    }

    favsInit();
    adsbConfigInit();
    tidesConfigInit();
    savedNetworksInit();

    server.on("/api/favourites", HTTP_GET, handleListFavs);
    server.on("/api/favourites", HTTP_POST, handleAddFav);
    server.on("/api/favourites", HTTP_DELETE, handleDeleteFav);
    server.on("/api/favourites/play", HTTP_POST, handlePlayFav);
    server.on("/adsb", HTTP_POST, handleAdsbSave);
    server.on("/tides", HTTP_POST, handleTidesSave);
    server.on("/audio-buffer", HTTP_POST, handleAudioBufferSave);
    server.on("/network/connect", HTTP_POST, handleNetworkConnect);
    server.on("/network/ap", HTTP_POST, handleNetworkApMode);
    server.on("/network/connect_saved", HTTP_POST, handleSavedNetworkConnect);
    server.on("/api/networksaved", HTTP_GET, handleSavedNetworkList);
    server.on("/api/networksaved", HTTP_POST, handleSavedNetworkAdd);
    server.on("/api/networksaved", HTTP_DELETE, handleSavedNetworkDelete);
    server.on("/manual", HTTP_GET, []() {
        if (!SPIFFS.exists("/manual.html"))
        {
            String html;
            html.reserve(3800);
            html += R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>ESP32 Radio - Manual</title>
    <style>
      body{font-family:Arial;margin:16px;max-width:900px}
      .card{padding:12px;border:1px solid #ddd;border-radius:8px;margin-bottom:16px}
    </style>
  </head>
  <body>
)rawliteral";
            html += renderNav();
            html += R"rawliteral(
    <div class="card">
      <h2>ESP32 Smart Radio V2 Manual</h2>
      <p><strong>Quick Start:</strong> Power on, wait for Wi-Fi, then open this Web UI from your device IP.</p>
      <p><strong>Note:</strong> Full manual file not found on SPIFFS (<code>/manual.html</code>), so this built-in quick manual is shown.</p>
    </div>
    <div class="card">
      <h3>Controls</h3>
      <ul>
        <li>Left encoder rotate: volume up/down</li>
        <li>Left encoder press: mute/unmute</li>
        <li>Right encoder rotate: next/previous station</li>
        <li>Right encoder press: next widget</li>
      </ul>
      <p>In Favourites widget: rotate to select, short press to play, long press (~5s) to delete.</p>
    </div>
    <div class="card">
      <h3>Web Pages</h3>
      <ul>
        <li><a href="/favourites">Favourites</a>: add, delete, play saved stations</li>
        <li><a href="/adsb">ADS-B Config</a>: set feed URLs and range</li>
        <li><a href="/tides">Tides Config</a>: set location, coordinates, refresh</li>
        <li><a href="/network">Network</a>: join Wi-Fi, save credentials, AP mode</li>
      </ul>
    </div>
    <div class="card">
      <h3>Defaults</h3>
      <ul>
        <li>Weather area: Clarinbridge</li>
        <li>ADS-B reference area: Clarinbridge coordinates</li>
        <li>Tides label: Galway</li>
        <li>Timezone: Irish time (with DST)</li>
      </ul>
    </div>
    <div class="card">
      <h3>Enable Full Manual</h3>
      <p>Upload filesystem data (including <code>data/manual.html</code>) to SPIFFS, then reload this page.</p>
    </div>
  </body>
</html>
)rawliteral";
            server.send(200, "text/html", html);
            return;
        }

        File f = SPIFFS.open("/manual.html", FILE_READ);
        if (!f)
        {
            server.send(500, "text/plain", "Failed to open manual file");
            return;
        }

        server.streamFile(f, "text/html");
        f.close();
    });

    server.on("/", HTTP_GET, []() {
        String html;
        html.reserve(2600);
        html += R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>ESP32 Radio</title>
    <style>
      body{font-family:Arial;margin:16px;max-width:900px}
      .card{padding:12px;border:1px solid #ddd;border-radius:8px;margin-bottom:16px}
      a{display:inline-block;margin:6px 12px 6px 0}
    </style>
  </head>
  <body>
)rawliteral";
        html += renderNav();
        html += R"rawliteral(
    <div class="card">
      <h2>ESP32 Radio</h2>
      <p>Use the links below to manage the radio.</p>
      <a href="/favourites">Open Favourites</a>
      <a href="/adsb">Open ADS-B Config</a>
      <a href="/tides">Open Tides Config</a>
      <a href="/manual">Open Manual</a>
      <a href="/network">Open Network</a>
    </div>
  </body>
</html>
)rawliteral";
        server.send(200, "text/html", html);
    });

    server.on("/favourites", HTTP_GET, []() {
        String html;
        html.reserve(3200);
        html += R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>ESP32 Radio - Favourites</title>
    <style>
      body{font-family:Arial;margin:16px;max-width:900px}
      .card{padding:12px;border:1px solid #ddd;border-radius:8px;margin-bottom:16px}
      input{padding:6px;margin:4px 0;width:100%;max-width:520px;box-sizing:border-box}
      button{padding:6px 10px;margin:4px 4px 4px 0}
      li{margin:6px 0}
    </style>
  </head>
  <body>
)rawliteral";
        html += renderNav();
        html += R"rawliteral(
    <div class="card">
      <h2>Favourites</h2>
      <ul id="list"></ul>
      <h3>Add Favourite</h3>
      <input id="name" placeholder="Name" />
      <input id="url" placeholder="Stream URL" />
      <button id="add">Add</button>
    </div>
    <script>
      async function load() {
        const res = await fetch('/api/favourites');
        const arr = await res.json();
        const list = document.getElementById('list');
        list.innerHTML = '';
        arr.forEach((it, idx) => {
          const li = document.createElement('li');
          li.innerHTML = `<strong>${it.name}</strong> — ${it.url} ` +
            `<button onclick="play(${idx})">Play</button> ` +
            `<button onclick="del(${idx})">Delete</button>`;
          list.appendChild(li);
        });
      }
      async function add() {
        const name = document.getElementById('name').value;
        const url = document.getElementById('url').value;
        const res = await fetch('/api/favourites', {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify({name, url})
        });
        if (!res.ok) {
          const txt = await res.text();
          alert('Add failed: ' + txt);
          return;
        }
        document.getElementById('name').value = '';
        document.getElementById('url').value = '';
        load();
      }
      async function del(idx) {
        await fetch('/api/favourites?index=' + idx, { method: 'DELETE' });
        load();
      }
      async function play(idx) {
        await fetch('/api/favourites/play?index=' + idx, { method: 'POST' });
      }
      document.getElementById('add').addEventListener('click', add);
      load();
    </script>
  </body>
</html>
)rawliteral";
        server.send(200, "text/html", html);
    });

    server.on("/adsb", HTTP_GET, []() {
        String html;
        html.reserve(2600);
        html += R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>ESP32 Radio - ADS-B Config</title>
    <style>
      body{font-family:Arial;margin:16px;max-width:900px}
      .card{padding:12px;border:1px solid #ddd;border-radius:8px;margin-bottom:16px}
      input{padding:6px;margin:4px 0;width:100%;max-width:650px;box-sizing:border-box}
      button{padding:6px 12px;margin-top:8px}
      label{font-weight:bold;display:block;margin-top:8px}
      .hint{color:#666;font-size:0.9em}
    </style>
  </head>
  <body>
)rawliteral";
        html += renderNav();
        html += "<div class=\"card\"><h2>ADS-B Config</h2><form method=\"post\" action=\"/adsb\">";
        html += "<label>Range (km)</label><input name=\"rangeKm\" type=\"number\" min=\"1\" max=\"500\" value=\"";
        html += String(adsbRangeKm());
        html += "\" />";
        html += "<label>Local SSID</label><input name=\"localSsid\" value=\"";
        html += adsbLocalSsid();
        html += "\" />";
        html += "<div class=\"hint\">Leave blank to use local feed on any 192.168.1.x network.</div>";
        html += "<label>Local feed URL</label><input name=\"localFeedUrl\" value=\"";
        html += adsbLocalFeedUrl();
        html += "\" />";
        html += "<label>Public feed URL</label><input name=\"publicFeedUrl\" value=\"";
        html += adsbPublicFeedUrl();
        html += "\" />";
        html += "<button type=\"submit\">Save ADS-B Config</button></form></div></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/tides", HTTP_GET, []() {
        String html;
        html.reserve(2400);
        html += R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>ESP32 Radio - Tides Config</title>
    <style>
      body{font-family:Arial;margin:16px;max-width:900px}
      .card{padding:12px;border:1px solid #ddd;border-radius:8px;margin-bottom:16px}
      input{padding:6px;margin:4px 0;width:100%;max-width:650px;box-sizing:border-box}
      button{padding:6px 12px;margin-top:8px}
      label{font-weight:bold;display:block;margin-top:8px}
      .hint{color:#666;font-size:0.9em}
    </style>
  </head>
  <body>
)rawliteral";
        html += renderNav();
        html += "<div class=\"card\"><h2>Tides Config</h2><form method=\"post\" action=\"/tides\">";
        html += "<label>Location name</label><input name=\"location\" value=\"";
        html += htmlEscape(tidesLocation());
        html += "\" />";
        html += "<label>Latitude</label><input name=\"latitude\" type=\"number\" step=\"0.0001\" value=\"";
        html += String(tidesLatitude(), 6);
        html += "\" />";
        html += "<label>Longitude</label><input name=\"longitude\" type=\"number\" step=\"0.0001\" value=\"";
        html += String(tidesLongitude(), 6);
        html += "\" />";
        html += "<label>Refresh (minutes)</label><input name=\"refreshMinutes\" type=\"number\" min=\"1\" max=\"720\" value=\"";
        html += String(tidesRefreshMs() / 60000UL);
        html += "\" />";
        html += "<div class=\"hint\">Used by the Tides widget to fetch sea level and estimate next high/low tide.</div>";
        html += "<button type=\"submit\">Save Tides Config</button></form></div></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/network", HTTP_GET, []() {
        String html;
        html.reserve(5200);

        String currentSsid = WiFi.SSID();
        String currentIp = WiFi.localIP().toString();
        if (currentIp == "0.0.0.0")
          currentIp = "not connected";

        int count = WiFi.scanNetworks();

        html += R"rawliteral(
    <!doctype html>
    <html>
      <head>
      <meta charset="utf-8" />
      <meta name="viewport" content="width=device-width,initial-scale=1" />
      <title>ESP32 Radio - Network</title>
      <style>
        body{font-family:Arial;margin:16px;max-width:900px}
        .card{padding:12px;border:1px solid #ddd;border-radius:8px;margin-bottom:16px}
        input,select{padding:6px;margin:4px 0;width:100%;max-width:520px;box-sizing:border-box}
        button{padding:6px 12px;margin-top:8px}
        .muted{color:#666;font-size:0.9em}
        .ok{color:#0a7a0a}
      </style>
      </head>
      <body>
    )rawliteral";
        html += renderNav();

        html += "<div class=\"card\"><h2>Current Network</h2>";
        html += "<p><strong>Connected SSID:</strong> " + htmlEscape(currentSsid.length() ? currentSsid : String("none")) + "</p>";
        html += "<p><strong>IP:</strong> " + htmlEscape(currentIp) + "</p>";
        if (networkStatus[0] != '\0')
          html += "<p class=\"ok\">" + htmlEscape(String(networkStatus)) + "</p>";
        html += "</div>";

        html += "<div class=\"card\"><h2>Join Network</h2>";
        html += "<form method=\"post\" action=\"/network/connect\">";
        html += "<label>SSID</label><select name=\"ssid\">";

        if (count <= 0)
        {
          html += "<option value=\"\">No networks found</option>";
        }
        else
        {
          for (int i = 0; i < count; i++)
          {
            String ssid = WiFi.SSID(i);
            long rssi = WiFi.RSSI(i);
            html += "<option value=\"" + htmlEscape(ssid) + "\">";
            html += htmlEscape(ssid) + " (" + String(rssi) + " dBm)";
            html += "</option>";
          }
        }

        html += "</select>";
        html += "<label>Password</label><input type=\"password\" name=\"password\" placeholder=\"Wi-Fi password\" />";
        html += "<button type=\"submit\">Connect</button></form>";
        html += "<p class=\"muted\">If this network is open, leave password blank.</p></div>";

        html += R"rawliteral(
<div class="card">
  <h2>Saved Networks</h2>
  <ul id="savedList"></ul>
  <h3>Add Saved Network</h3>
  <input id="savedSsid" placeholder="SSID" />
  <input id="savedPassword" type="password" placeholder="Password" />
  <button id="savedAdd">Save Network</button>
</div>
<script>
  async function loadSaved() {
    const res = await fetch('/api/networksaved');
    const arr = await res.json();
    const list = document.getElementById('savedList');
    list.innerHTML = '';

    arr.forEach((it, idx) => {
      const li = document.createElement('li');
      li.innerHTML = `<strong>${it.ssid}</strong> ` +
        `<button onclick="connectSaved(${idx})">Connect</button> ` +
        `<button onclick="delSaved(${idx})">Delete</button>`;
      list.appendChild(li);
    });
  }

  async function addSaved() {
    const ssid = document.getElementById('savedSsid').value;
    const password = document.getElementById('savedPassword').value;
    const body = new URLSearchParams({ssid, password});

    await fetch('/api/networksaved', {
      method: 'POST',
      headers: {'Content-Type':'application/x-www-form-urlencoded'},
      body
    });

    document.getElementById('savedSsid').value = '';
    document.getElementById('savedPassword').value = '';
    loadSaved();
  }

  async function delSaved(idx) {
    await fetch('/api/networksaved?index=' + idx, { method: 'DELETE' });
    loadSaved();
  }

  async function connectSaved(idx) {
    await fetch('/network/connect_saved?index=' + idx, { method: 'POST' });
    window.location.reload();
  }

  document.getElementById('savedAdd').addEventListener('click', addSaved);
  loadSaved();
</script>
)rawliteral";

        html += "<div class=\"card\"><h2>Audio Buffer</h2>";
        html += "<form method=\"post\" action=\"/audio-buffer\">";
        html += "<label>RAM buffer (bytes)</label><input name=\"ramBytes\" type=\"number\" min=\"-1\" value=\"" + String(audioBufferRamBytes()) + "\" />";
        html += "<p><label>PSRAM buffer (bytes)</label><input name=\"psramBytes\" type=\"number\" min=\"-1\" value=\"" + String(audioBufferPsramBytes()) + "\" />";
        html += "<p class=\"muted\">Use -1 to leave the library default unchanged. Changes take effect after reboot.</p>";
        html += "<button type=\"submit\">Save Audio Buffer</button></form></div>";

        html += "<div class=\"card\"><h2>AP Mode</h2>";
        html += "<p>Start AP mode with static IP <strong>192.168.2.1</strong>.</p>";
        html += "<form method=\"post\" action=\"/network/ap\">";
        html += "<button type=\"submit\">Enable AP Mode</button></form></div>";

        html += "</body></html>";
        server.send(200, "text/html", html);
    });

    server.begin();
    Serial.println("[WEBUI] server started");
}

void webuiLoop()
{
  if (!serverPtr)
    return;

    server.handleClient();
}
