# ESP32 Astro Focuser

Firmware for a motorised **ASCOM Alpaca focuser** for my Sky-Watcher P200 Newton
telescope (but should work in principle for others as well), built around an ESP32-WROOM-32, a 28BYJ-48 stepper, a ULN2003 driver
board, and an SBC-OLED01 128×64 I²C display.

The focuser is fully controllable from **N.I.N.A.** (Windows) and
**KStars/Ekos** (Astroberry / Linux) via the ASCOM Alpaca protocol, or manually
via three physical buttons and a small web dashboard.

---

## Features

- **ASCOM Alpaca Focuser V3** – compatible with N.I.N.A., KStars/Ekos, any Alpaca client
- **INDI compatible** via `indi_alpaca` bridge (see [docs/ASTROBERRY.md](docs/ASTROBERRY.md))
- **Absolute position tracking** – manual zeroing at fully-retracted position
- **Backlash compensation** – configurable overshoot + return on direction reversal
- **WiFi** – station mode with automatic captive-portal fallback for first-time setup
- **mDNS** – reachable at `http://esp32-focuser.local/` without knowing the IP
- **Web UI** – live status, manual step controls, full configuration page
- **OLED display** – real-time position, WiFi status, Alpaca connection state
- **3 physical buttons** – `−`, `ZERO`, `+` with short press / hold / long-press semantics
- **Non-blocking** – FreeRTOS stepper task on Core 1; network never stalls the motor
- **OTA updates** – ElegantOTA endpoint at `/update`

---

## Hardware

| Part | Notes |
|------|-------|
| ESP32-WROOM-32 Dev Board | Any generic 30-pin DevKit |
| 28BYJ-48 5 V stepper | Standard geared version (64:1 gearbox) |
| ULN2003APG driver module | Common blue PCB |
| SBC-OLED01 128×64 I²C | SSD1306 or compatible controller |
| GT2 belt 6 mm | Motor 16T → Focuser 60T pulley (3.75× reduction) |

---

## Wiring

### Power

Use the **same 5 V supply** for both the ESP32 (via USB) and the ULN2003 motor
driver (via its VCC terminal). The GND lines **must be shared** — without a
common GND the ULN2003 will not respond to ESP32 signals.

> ⚠️ Do NOT power the 28BYJ-48 from the ESP32's 5 V or 3.3 V pins. The motor
> draws 200–250 mA and will reset or damage the ESP32.

### ULN2003 Stepper Driver

| ESP32 GPIO | ULN2003 pin | Motor wire |
|------------|-------------|------------|
| GPIO **26** | IN1 | Blue (coil A+) |
| GPIO **25** | IN2 | Pink (coil A−) |
| GPIO **33** | IN3 | Yellow (coil B+) |
| GPIO **32** | IN4 | Orange (coil B−) |
| GND | GND | — (shared ground) |
| 5 V supply | VCC | Red (centre tap) |

### OLED Display (SBC-OLED01, I²C)

| ESP32 | OLED |
|-------|------|
| GPIO **21** (SDA) | SDA |
| GPIO **22** (SCL) | SCL |
| 3.3 V | VCC |
| GND | GND |

Default I²C address: `0x3C`. If the display stays blank, your module may be
soldered to `0x3D` (or labelled `0x78` / `0x7A` in 8-bit notation — divide by 2
for the 7-bit address the firmware uses). Override in `platformio.ini`:
```ini
build_flags = ... -D OLED_I2C_ADDR=0x3D
```
For SH1106-based modules add `-D OLED_SH1106`.

### DS18B20 Temperature Sensor (1-Wire)

| ESP32 | DS18B20 |
|-------|---------|
| GPIO **4** (DATA) | DATA (yellow/white) |
| 3.3 V | VCC (red) |
| GND | GND (black) |

A **4.7 kΩ pull-up resistor** between DATA and 3.3 V is required by the 1-Wire protocol. Without it the sensor will not respond.

Change the pin in [`include/Pins.h`](include/Pins.h) (`PIN_TEMP_DATA`) if GPIO 4 is occupied. The temperature is reported via the Alpaca `Temperature` property (visible in NINA's focuser panel), the web status page, and the OLED display.

### Buttons (3×, normally-open, connect between GPIO and GND)

| ESP32 GPIO | Button | Action |
|------------|--------|--------|
| GPIO **14** | **−** | Focus inward — short press = 1 step, hold = continuous |
| GPIO **27** | **ZERO** | Hold 2 s to set current position = 0 |
| GPIO **13** | **+** | Focus outward |

No external resistors needed — internal pull-ups (`INPUT_PULLUP`) are enabled.

### Wiring Diagram

```
  5 V supply ─────────────────────────── ULN2003 VCC
                                         ULN2003 IN1 ──── ESP32 GPIO 26
                                         ULN2003 IN2 ──── ESP32 GPIO 25
                                         ULN2003 IN3 ──── ESP32 GPIO 33
                                         ULN2003 IN4 ──── ESP32 GPIO 32
                                         ULN2003 GND ─┐
  ESP32-WROOM-32 DevKit                               │
  ┌────────────────────┐                              │
  │ GPIO 26 ───────────┼── ULN2003 IN1               │
  │ GPIO 25 ───────────┼── ULN2003 IN2               │
  │ GPIO 33 ───────────┼── ULN2003 IN3               │
  │ GPIO 32 ───────────┼── ULN2003 IN4               │
  │                    │                              │
  │ GPIO 21 (SDA) ─────┼── OLED SDA                  │
  │ GPIO 22 (SCL) ─────┼── OLED SCL                  │
  │ 3.3 V ─────────────┼── OLED VCC                  │
  │                    │                              │
  │ GPIO 14 ───────────┼── BTN [−] ── GND             │
  │ GPIO 27 ───────────┼── BTN [Z] ── GND             │
  │ GPIO 13 ───────────┼── BTN [+] ── GND             │
  │                    │                              │
  │ GND ───────────────┼──────────────────────────────┘
  └────────────────────┘   (common ground)
```

All GPIO assignments live in [`include/Pins.h`](include/Pins.h) and can be
changed without touching any other source file. See
[docs/HARDWARE.md](docs/HARDWARE.md) for strapping-pin warnings and 3.3 V /
ULN2003 compatibility notes.

---

## Build & Flash

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- ESP32 board drivers (CP210x or CH340 depending on your DevKit)

### First-time build

```bash
git clone <this-repo>
cd esp32-focusser

# Install dependencies and compile
pio run

# Flash firmware
pio run -t upload

# Build and upload the filesystem image (REQUIRED first time and after web UI changes)
pio run -t buildfs
pio run -t uploadfs
```

> **The filesystem flash is mandatory.** Without it, the Alpaca setup page and
> the web UI will not be served.

### Subsequent firmware updates

```bash
pio run -t upload
# Only re-flash filesystem if data/ files changed:
pio run -t buildfs && pio run -t uploadfs
```

OTA updates are also available at `http://esp32-focuser.local/update` once the
device is on WiFi.

---

## WiFi Setup

1. On first boot the ESP32 starts an AP named `ESP32-Focuser-XXXX`.
2. Connect to it (password: `focuser1`).
3. Open `http://192.168.4.1/` and enter your home network credentials.
4. The device restarts and connects. The OLED shows the assigned IP.

See [docs/WIFI_SETUP.md](docs/WIFI_SETUP.md) for changing credentials later and
for network discovery (mDNS, Alpaca UDP).

---

## Zeroing the Position

The firmware has no homing sensor. Before each session:

1. Press and hold **−** until the focuser is fully retracted (at mechanical stop).
2. Hold **ZERO** for 2 seconds → position is set to 0.
3. OLED shows `STATUS: READY`.

NINA and Ekos will now read a meaningful absolute position. If you skip this
step the position counter starts at an arbitrary offset.

---

## Web Interface

Navigate to `http://esp32-focuser.local/` (or the IP shown on the OLED).

- **Status** – current position, WiFi SSID, Alpaca connection state
- **Manual Control** – step buttons `<<<` `<<` `<` STOP `>` `>>` `>>>`
- **Configuration** – all parameters, saved to flash (NVS) instantly

---

## Connecting to N.I.N.A.

### 1. Verify the device is ready

Power on the focuser and check the OLED shows:
```
WiFi: OK   Alp: OK
```
Both must be green before proceeding. If `Alp:` is missing, wait a few seconds
for the Alpaca server to initialise.

### 2. Open the Alpaca Focuser chooser in N.I.N.A.

1. N.I.N.A. → **Equipment** tab → **Focuser** panel.
2. In the focuser dropdown select **ASCOM Alpaca Focuser**.
3. Click the **wrench / gear icon** (⚙) next to the dropdown — this opens the
   *ASCOM Alpaca Focuser Setup* dialog.

### 3. Run discovery

The dialog shows three discovery settings — leave them at their defaults:

| Setting | Default | Meaning |
|---------|---------|---------|
| Discovery port | `32227` | UDP port the Alpaca discovery broadcast uses |
| # of broadcasts | `1` | How many UDP packets to send |
| Use IPv4 | ✓ | Required; the ESP32 is IPv4-only |

Click **Discover**. After a moment the focuser appears as **ESP32 P200 Focuser**
in the device list. Select it and close the dialog.

### 4. If auto-discovery fails

Some routers block UDP broadcasts between subnets or across VLANs.

**Option A – type the IP directly:**
In the same Setup dialog, enter the IP address from the OLED in the *Host*
field (e.g. `192.168.1.42`) and click **Discover** again. N.I.N.A. will query
that specific host on port 80 and should find the device immediately.

**Option B – add the device manually:**
Some N.I.N.A. versions have an **Add** button in the chooser. Click it and enter:
- Host: `192.168.1.42` (or `esp32-focuser.local`)
- Port: `80`
- Device type: `Focuser`
- Device number: `0`

**Windows Firewall note:** If discovery never works even with a direct IP,
add an inbound rule in *Windows Defender Firewall* allowing **UDP port 32227**.
N.I.N.A. must be able to receive the reply packet.

### 5. Connect and use

Back in the main Focuser panel, click **Connect** (plug icon). N.I.N.A. reads
`Position`, `MaxStep`, `IsMoving`, and `Absolute = true`. For autofocus, a
starting step size of 200–500 half-steps is a reasonable baseline — calibrate
with your camera/scope combination.

See [docs/NINA.md](docs/NINA.md) for zeroing procedure, autofocus tips, and
troubleshooting.

---

## Connecting to KStars / Ekos (Astroberry / INDI)

INDI does not natively speak Alpaca. You need the `indi-alpaca` bridge driver
installed on your Raspberry Pi / Astroberry.

### 1. Install indi-alpaca

```bash
sudo apt update
sudo apt install indi-alpaca
```

If the package is not available for your distribution, build it from source:

```bash
git clone https://github.com/indilib/indi-3rdparty
cd indi-3rdparty/indi-alpaca
mkdir build && cd build
cmake .. && make -j4
sudo make install
```

### 2. Add the driver in KStars / Ekos

1. Open **KStars → Ekos**.
2. In the INDI Control Panel click **Add Driver**.
3. Select **Focuser → Alpaca Focuser**.
4. Start the driver.
5. In the driver's **Options** tab set the Alpaca base URL to:
   `http://esp32-focuser.local` (or use the IP from the OLED if mDNS is
   unavailable: `http://192.168.1.42`).
6. Click **Connect**.

### 3. Using indi_web_manager (Astroberry default)

If you manage INDI drivers through the Astroberry web interface:

1. Navigate to `http://astroberry.local:8624/`.
2. Under *Drivers*, find **Alpaca → Focuser** and add it.
3. Set the Base URL to `http://esp32-focuser.local`.
4. Click **Start** then **Connect**.

See [docs/ASTROBERRY.md](docs/ASTROBERRY.md) for version-specific notes and
simultaneous NINA + Ekos use.

---

## Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `maxPosition` | 100 000 | Upper travel limit (motor half-steps) |
| `backlashSteps` | 0 | Compensation steps on direction change |
| `backlashApproachDir` | +1 | +1 = outward approach, −1 = inward |
| `motorReversed` | false | Flip motor direction |
| `stepsPerRev` | 4076 | Half-steps per motor output shaft revolution |
| `motorPulleyTeeth` | 16 | GT2 motor pulley tooth count |
| `focuserPulleyTeeth` | 60 | GT2 focuser pulley tooth count |
| `manualStepSize` | 50 | Steps per short button press |
| `manualSpeed` | 400 | Steps/s for manual and button moves |
| `motorMaxSpeed` | 500 | Hard speed cap (steps/s) |

Calibrate `stepsPerRev`, `maxPosition`, and `backlashSteps` after assembly.
See [docs/CALIBRATION.md](docs/CALIBRATION.md).

---

## Libraries Used

| Library | License | Role |
|---------|---------|------|
| [ESP32AlpacaDevices2](https://github.com/npeter/ESP32AlpacaDevices2) by **npeter** | MIT | Alpaca server, OTA, SLog |
| [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) by **ESP32Async** | LGPL-3.0 | Async HTTP server |
| [ArduinoJson](https://arduinojson.org/) by **Benoît Blanchon** | MIT | JSON serialisation |
| [U8g2](https://github.com/olikraus/u8g2) by **olikraus** | BSD-2-Clause | OLED driver |
| [AccelStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/) by **Mike McCauley** | GPL-3.0 | Stepper motion profile |

---

## Directory Structure

```
esp32-focusser/
  platformio.ini         Build system config
  include/
    Pins.h               GPIO definitions (change pins here)
    Config.h             Compile-time defaults
  src/
    main.cpp             Entry point and task init
    util/Log.h           Logging macros (wraps SLog)
    storage/             NVS persistence (PreferencesManager)
    display/             OLED via U8g2 (DisplayManager)
    input/               Button debounce (ButtonManager)
    focuser/             Stepper, position, backlash logic
    network/             WiFi, captive portal, mDNS
    web/                 Custom HTTP routes and JSON API
    alpaca/              ASCOM Alpaca Focuser V3 implementation
  data/www/              LittleFS web assets (HTML/CSS/JS)
  docs/                  Extended documentation
  scripts/               PlatformIO build scripts
```

---

## Docs

- [Hardware & Wiring](docs/HARDWARE.md)
- [WiFi Setup](docs/WIFI_SETUP.md)
- [N.I.N.A. Integration](docs/NINA.md)
- [Astroberry / INDI](docs/ASTROBERRY.md)
- [Calibration](docs/CALIBRATION.md)
- [Mechanics & Belt Drive](docs/MECHANICS.md)

---

## Attribution & Credits

This project was designed and vibe-coded by **StWiemann** for personal use
with a Sky-Watcher P200/200P Newton telescope.

The firmware is built on top of:

- **[ESP32AlpacaDevices2](https://github.com/npeter/ESP32AlpacaDevices2)** by
  **npeter** — provides the ASCOM Alpaca server framework, ElegantOTA, and SLog.
  Without this library the Alpaca integration would have required implementing
  the entire protocol from scratch.
- **[AccelStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/)** by
  **Mike McCauley** — smooth acceleration/deceleration profiles for the stepper.
- **[U8g2](https://github.com/olikraus/u8g2)** by **olikraus** — universal
  display library covering the SSD1306 and SH1106 OLED controllers.
- **[ArduinoJson](https://arduinojson.org/)** by **Benoît Blanchon** and
  **[ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)** by
  the ESP32Async contributors — web API and async HTTP serving.


---

## License

This project is released under the **MIT License** — see [LICENSE](LICENSE) for
the full text.

Note that **AccelStepper** is GPL-3.0 licensed. If you distribute a compiled
binary that includes AccelStepper, the GPL-3.0 terms apply to that binary.
The source code of this project (excluding vendored libraries) remains MIT.
