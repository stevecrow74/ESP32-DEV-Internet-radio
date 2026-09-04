# ESP32 Smart Radio V2
## One-Page Quick Guide (Print Version)

---

## Start Up
1. Power on the radio.
2. Wait for startup.
3. Check top status bar for Wi-Fi and time.

## Controls
### GPIO 1 Button
- Short press: Next widget
- Long press: Unused

### Touch Pads
- GPIO 2: Previous station
- GPIO 6: Next station
- GPIO 4: Volume down
- GPIO 5: Volume up

Main station browsing uses favourites. Startup defaults to Rewind when present.

## Favourites Widget
- Press GPIO 1 to move through widgets until Favourites appears.
- In Favourites:
   - GPIO 2/GPIO 6: Move highlight
   - After three seconds: Play the selected favourite

---

## Web UI Access
1. Find the radio IP (status bar or serial monitor).
2. On same Wi-Fi network, open:
   - http://DEVICE_IP/

## Main Pages
- /favourites
- /adsb
- /tides
- /network

## Common Tasks
### Add Favourite Station
1. Open /favourites
2. Enter Name + Stream URL
3. Press Add

### Configure ADS-B
1. Open /adsb
2. Set range and feed URLs
3. Save

### Configure Tides
1. Open /tides
2. Set location + coordinates + refresh + source URL
3. The Tides widget shows all daily high/low events in High/Low, Time, and Height columns.
3. Save

### Network Management
1. Open /network
2. Join Wi-Fi and/or save credentials
3. Use Connect/Delete on saved entries
4. AP mode (fallback):
   - SSID: ESP32-Radio-AP
   - IP: 192.168.2.1

---

## Defaults
- Weather area: Clarinbridge
- ADS-B reference area: Clarinbridge
- Tides label: Galway
- Tides source URL: https://www.tidetime.org/europe/ireland/galway.htm
- Timezone: Irish time (with DST)

## If Something Fails
- No audio: check mute, volume, and station URL.
- Web UI not loading: verify IP and same network.
- No Wi-Fi: use /network or AP mode.

## Full Documentation
- USER_MANUAL.md
- QUICK_START.md
