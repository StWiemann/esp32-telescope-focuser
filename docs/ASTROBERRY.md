# Astroberry / INDI Integration Guide

## Architecture

The ESP32 provides an ASCOM Alpaca HTTP API. INDI does not natively speak
Alpaca, but an Alpaca-to-INDI bridge is available:

```
Astroberry / KStars+Ekos
        │
   INDI server (indi_alpaca or alpaca2indi bridge)
        │
   HTTP / ASCOM Alpaca (port 80)
        │
   ESP32 Focuser
```

---

## Option A: indi_alpaca (Recommended)

`indi_alpaca` is a generic INDI driver that wraps any Alpaca device.

### Installation on Astroberry / Raspberry Pi OS

```bash
sudo apt update
sudo apt install indi-alpaca
```

If not available in the package manager, build from source:
```bash
git clone https://github.com/indilib/indi-3rdparty
cd indi-3rdparty/indi-alpaca
mkdir build && cd build
cmake .. && make -j4
sudo make install
```

### Configuration in KStars / Ekos

1. Open **KStars → Ekos**.
2. In the INDI Control Panel, click **Add Driver**.
3. Select: **Focuser → Alpaca Focuser**.
4. Start the driver.
5. In the driver's **Options** tab, enter the Alpaca base URL:
   `http://esp32-focuser.local` (or the IP address)
6. Click **Connect**.

---

## Option B: indi_alpaca via indi_web_manager

If you use `indi_web_manager` (Astroberry default):

1. Navigate to `http://astroberry.local:8624/`
2. Add driver: **Alpaca → Focuser**.
3. Set base URL to `http://esp32-focuser.local`.
4. Start driver.

---

## Simultaneous NINA and Astroberry Use

The ESP32 can serve multiple Alpaca clients concurrently
(`ALPACA_MAX_CLIENTS = 8` in the library defaults).

However, two clients issuing `Move` commands simultaneously could produce
unpredictable results. It is recommended to use **only one client at a time**
for controlling movement.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Driver cannot connect | Ping `esp32-focuser.local` from Astroberry; check mDNS |
| Focuser not found in Alpaca chooser | Add URL manually |
| INDI shows wrong position | Zero the focuser (buttons or web UI) |

---

## Notes on INDI Alpaca Bridge Versions

Different versions of `indi_alpaca` have different feature coverage.
The firmware targets Focuser V3 (Interface Version 4 per `AlpacaConfig.h`).
Ensure your bridge version supports Focuser V3 / Interface Version 4.
