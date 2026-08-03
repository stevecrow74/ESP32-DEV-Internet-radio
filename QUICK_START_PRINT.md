# ESP32 Smart Radio V2
## One-Page Quick Guide (Print Version)

---

## Start Up
1. Power on the radio.
2. Wait for startup.
3. Check top status bar for Wi-Fi and time.

## Controls
### Left Encoder
- Rotate: Volume up/down
- Press: Mute/unmute

### Right Encoder
- Rotate: Next/previous favourite station (normal widgets)
- Press: Next widget

Main station browsing uses favourites. Startup defaults to Rewind when present.

## Favourites Widget
- Manual access only (not in auto-cycle).
- Press right encoder button to move through widgets until Favourites appears.
- In Favourites:
  - Rotate: Move highlight
  - Short press: Play selected favourite
  - Long press (~5 sec): Delete selected favourite

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
2. Set location + coordinates + refresh
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
- Timezone: Irish time (with DST)

## If Something Fails
- No audio: check mute, volume, and station URL.
- Web UI not loading: verify IP and same network.
- No Wi-Fi: use /network or AP mode.

## Full Documentation
- USER_MANUAL.md
- QUICK_START.md
