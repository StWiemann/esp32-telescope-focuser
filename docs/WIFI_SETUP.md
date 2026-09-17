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

**Option A – Web UI (preferred):**
1. Open `http://esp32-focuser.local/` (or the IP shown on the OLED).
2. Open the **Network** tab.
3. Press **Clear WiFi & reboot into setup**.
4. Join `ESP32-Focuser-XXXX` (password `focuser1`) and open `http://192.168.4.1/`.

**Option B – USB erase** (also wipes all other settings):
```
pio run -t erase
pio run -t upload
pio run -t buildfs && pio run -t uploadfs
```

The Network tab also shows the **BSSID** (MAC of the access point you are
actually associated with). On a Fritzbox mesh that tells you whether you
landed on the box or a repeater — picking the same SSID twice in the portal
does not lock the radio to one node. Pin the device in the Fritzbox Mesh
settings, or use a 2.4 GHz SSID that is not extended to the repeater.

---

## Mesh / Fritzbox notes

The firmware stores only SSID + password. The ESP32 will roam to whichever
mesh node is louder. `WiFi.setSleep(false)` is set to reduce idle disconnects.
If the link still drops, check the **Last disconnect** reason on the Network
tab (Serial also logs `reason N NAME`).

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
