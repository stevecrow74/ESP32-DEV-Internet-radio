# ESP32 Smart Radio V2 - Quick Start

## 1. Power On
1. Power the ESP32 radio.
2. Wait for startup to complete.
3. Check the screen status bar for Wi-Fi and time.

## 2. Basic Controls
### Left encoder (Volume)
- Rotate: volume up/down
- Press: mute/unmute

### Right encoder (Stations and Widgets)
- Rotate: next/previous favourite station
- Press: next widget

Main station browsing uses your favourites list.
Startup default is Rewind when present in favourites.

## 3. Favourites Widget
- Favourites is manual access only (not in auto-cycle).
- Use right encoder button to switch widgets until Favourites appears.
- In Favourites:
  - Rotate: move selection
  - Short press: play selected favourite
  - Long press (~5s): delete selected favourite

## 4. Open Web UI
1. Find the device IP on serial monitor or status bar.
2. On a phone/laptop on the same network, open:
   - http://DEVICE_IP/

## 5. Most Common Web Tasks
### Favourites
- Go to /favourites
- Add station name + stream URL
- Play or delete entries

### Network
- Go to /network
- Join Wi-Fi network
- Save Wi-Fi credentials for quick reconnect
- Enable AP mode if needed
  - AP SSID: ESP32-Radio-AP
  - AP IP: 192.168.2.1

### ADS-B
- Go to /adsb
- Set range and feed URLs
- Save config

### Tides
- Go to /tides
- Set location, latitude, longitude, refresh interval, and source URL
- Save config

## 6. Default Locations and Time
- Weather default: Clarinbridge area
- ADS-B default reference: Clarinbridge area
- Tides default location label: Galway
- Tides default source URL: https://www.tidetime.org/europe/ireland/galway.htm
- Timezone: Irish time (with DST)

## 7. If Something Looks Wrong
- No audio: check mute/volume and station URL.
- Web UI not loading: verify IP and same Wi-Fi network.
- No Wi-Fi: use /network or AP mode.
- Bad data in ADS-B or tides: check config pages and save again.

## 8. Full Guide
For complete details, use:
- USER_MANUAL.md
