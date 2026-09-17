# WiFi Setup Guide

## First Boot (No Credentials Stored)

When the ESP32 boots with no WiFi credentials, it automatically starts an
**Access Point (AP)** for configuration:

1. On your PC or phone, scan for WiFi networks.
2. Connect to `ESP32-Focuser-XXXX` (last 4 digits are from the MAC address).
   Default AP password: `focuser1`
3. A captive portal opens automatically (if not, navigate to `http://192.168.4.1/`).
4. Select your home network SSID from the dropdown (or enter manually).
5. Enter the WiFi password.
6. Press **Speichern & Verbinden**.
7. The ESP32 saves the credentials and restarts.
8. Within ~10 seconds the focuser connects to your WiFi.

OLED display shows `WiFi: OK` after successful connection.

---

## Normal Station Mode

After credentials are saved the ESP32 always tries to connect at boot.

- Connect timeout: 20 seconds.
- If connection fails (out of range, wrong password): falls back to AP mode.
- Automatic reconnect: every 30 seconds if the link drops during operation.

---

## Changing WiFi Credentials

**Option A – AP mode reset:**
1. Hold the ZERO button while powering on (not yet implemented; use Option B).

**Option B – Web interface:**
1. Navigate to `http://esp32-focuser.local/setup/v1/focuser/0/setup` (Alpaca setup page).
2. Use the "Server" tab → reset or clear credentials.

**Option C – Clear NVS via serial:**
```
pio run -t monitor
(type reset command if implemented)
```

---

## Finding the Focuser on Your Network

After connecting:

| Method | URL |
|--------|-----|
| mDNS hostname | `http://esp32-focuser.local/` |
| IP address | Check your router's DHCP table or OLED display |
| Alpaca discovery | NINA → Telescopes & Equipment → Focuser → ASCOM Alpaca |

---

## Multiple Focusers on the Same Network

Each focuser gets a unique AP SSID suffix (last 4 hex of MAC). mDNS hostnames
must be unique – change `hostname` in the configuration page before adding a
second unit.
