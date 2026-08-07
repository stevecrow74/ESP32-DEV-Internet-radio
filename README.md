# ESP32 Smart Radio V2 - User Manual
![ESP32 interner radio](img_3597.jpg)
# Hardware
This project uses:
-ESP32 Dev board
- 2 rotary encoders
- GMT020-02 ST7789 2" LCD
- GY-PCM5102 I2S Stereo Board


# User settings
This build uses my own local settings, these can be changed in the Webui.



## 1. Overview
ESP32 Smart Radio V2 is a Wi-Fi internet radio with a TFT display, rotary encoder controls, audio streaming, and a built-in browser-based Web UI for management.

Main features:
- Internet radio streaming
- On-device widgets (Clock, ADS-B, Weather, Tides, Audio, Favourites)
- Favourites management on-device and via Web UI
- ADS-B source/config management
- Tides config management
- Network scan/connect, saved Wi-Fi credentials, and AP mode

## 2. Hardware Controls
The device uses two rotary encoders.

### Encoder 1 (Volume/Mute)
- Rotate: change volume (0 to 21)
- Press: mute/unmute audio

### Encoder 2 (Station/Widget/Favourites)
Normal (most widgets):
- Rotate: next/previous favourite station (main radio browsing now uses your favourites list)
- Press: next widget (manual widget selection)

When on Favourites widget:
- Rotate: move selection in favourites list
- Short press: play selected favourite
- Long press (hold about 5 seconds): delete selected favourite

## 3. Widget Behavior
### Auto-cycle
- Widgets auto-cycle every ~10 seconds.
- Favourites widget is excluded from automatic cycle.
- You can still access Favourites manually using widget button presses.

### Manual hold
- After manual widget selection, current widget is held for ~20 seconds before normal auto behavior resumes.

## 4. Default Location and Time Settings
Compile-time defaults:
- Weather location label: Clarinbridge
- Weather coordinates: latitude 53.277, longitude -8.886
- ADS-B reference coordinates: latitude 53.277, longitude -8.886
- Tide location label: Galway
- Tide source URL: https://www.tidetime.org/europe/ireland/galway.htm
- Timezone: Irish time with DST (GMT0IST,M3.5.0/1,M10.5.0)

Notes:
- ADS-B and Tides values can be changed in Web UI and saved to SPIFFS.
- Saved values override compile-time defaults after reboot.

## 5. First Boot and Startup
On startup the system initializes:
1. SPIFFS storage
2. Display
3. Wi-Fi (WiFiManager auto-connect)
4. Audio engine
5. Station manager and current station connection
6. Widgets
7. Web UI server
8. Weather engine update

If no saved Wi-Fi is available, WiFiManager may open a captive portal depending on your environment.

### Startup station behavior
- Main radio station browsing is sourced from favourites.
- Default startup station is Rewind when present in favourites.
- If favourites are empty, the system falls back to Rewind URL so playback still works.

## 6. Web UI
Open your radio IP in a browser (shown in serial logs and status bar when connected).

Top navigation pages:
- /
- /favourites
- /adsb
- /tides
- /network

### Home (/)
Landing page with links to all management pages.

### Favourites (/favourites)
- View favourites list
- Add a station (name + URL)
- Play a favourite
- Delete a favourite

API endpoints:
- GET /api/favourites
- POST /api/favourites
- DELETE /api/favourites?index=N
- POST /api/favourites/play?index=N

### ADS-B Config (/adsb)
Configure and save:
- Range (km)
- Local SSID match (optional)
- Local feed URL
- Public feed URL

Saved to:
- /adsb_config.json

### Tides Config (/tides)
Configure and save:
- Location name
- Latitude
- Longitude
- Refresh interval (minutes)
- Source URL (default TideTime Galway page)

Current behavior:
- Tide widget fetches and parses high/low/trend data from the configured source URL.
- Default parser is built for the TideTime Galway page structure.

Saved to:
- /tides_config.json

### Network (/network)
Functions:
- View current SSID and IP
- Scan and join nearby networks
- Save Wi-Fi credentials
- Connect using saved credentials
- Delete saved credentials
- Enable AP mode

AP mode:
- SSID: ESP32-Radio-AP
- IP: 192.168.2.1

Saved networks file:
- /saved_networks.json

## 7. Persistence Files (SPIFFS)
The following files are created/used on SPIFFS:
- /favourites.json
- /adsb_config.json
- /tides_config.json
- /saved_networks.json

## 8. Build and Upload
Project is configured for PlatformIO.

Typical command:
- platformio run --target upload

Important:
- Partition scheme is set to huge_app.csv to fit current firmware size.

## 9. Troubleshooting
### Device boots but no audio
- Check station URL validity.
- Confirm Wi-Fi connected.
- Confirm not muted and volume > 0.

### Web UI unreachable
- Confirm device IP from serial logs/status bar.
- Ensure phone/PC is on same network.
- If AP mode enabled, connect to ESP32-Radio-AP and browse to 192.168.2.1.

### Favourites not updating
- Ensure both Name and URL are entered.
- Use full stream URLs including http:// or https://.

### Reset loops after major changes
- Re-upload latest stable build.
- Check monitor output at 115200.
- Reduce feature changes per build when diagnosing.

## 10. Operational Tips
- Use favourites for reliable station recall.
- Keep local ADS-B feed URL for home network and public URL for remote access.
- Tune tides refresh to balance data freshness and network usage.
- Keep saved Wi-Fi entries updated if passwords change.

## 11. Safety and Maintenance
- Power from a stable USB supply.
- Avoid abrupt power cuts during active writes where possible.
- Back up important SPIFFS config values if deploying multiple units.
