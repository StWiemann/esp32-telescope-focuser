# Hardware Guide

## Components

| Component | Part | Notes |
|-----------|------|-------|
| Controller | ESP32-WROOM-32 Dev Board (30-pin) | Any standard DevKit v1 compatible |
| Stepper | 28BYJ-48 5V | Standard geared version |
| Driver | ULN2003APG module | Common 5-pin blue PCB with LEDs |
| Display | SBC-OLED01 | 128×64 I²C; SSD1306 or compatible |
| Buttons | Momentary NO | 3× normally-open, against GND |
| Power | 5V 1A USB or wall adapter | See notes below |

---

## GPIO Pinout

All pins are defined in `include/Pins.h` and can be changed without modifying source code.

### ULN2003 Stepper Driver

```
ESP32 GPIO 26  →  ULN2003 IN1   (motor: Blue  / coil A+)
ESP32 GPIO 25  →  ULN2003 IN2   (motor: Pink  / coil A-)
ESP32 GPIO 33  →  ULN2003 IN3   (motor: Yellow/ coil B+)
ESP32 GPIO 32  →  ULN2003 IN4   (motor: Orange/ coil B-)

ESP32 GND      →  ULN2003 GND   (shared ground – REQUIRED)
5V supply      →  ULN2003 VCC   (motor supply)
```

> **IMPORTANT:** The 5 V motor supply must share GND with the ESP32 3.3 V supply.
> Do NOT power the motor from the ESP32's 3.3 V or 5 V pins – the 28BYJ-48
> draws 200–250 mA which will damage or reset the ESP32.

### OLED Display (I²C)

```
ESP32 GPIO 21 (SDA)  →  OLED SDA
ESP32 GPIO 22 (SCL)  →  OLED SCL
ESP32 3.3 V          →  OLED VCC
ESP32 GND            →  OLED GND
```

Default I²C address: `0x3C` (configurable via `-D OLED_I2C_ADDR=0x3D`).
If the display does not show anything, try 0x3D.

To use an SH1106 controller instead of SSD1306, add `-D OLED_SH1106` to `build_flags`.

### Buttons (3×)

All buttons are wired normally-open, against GND, with `INPUT_PULLUP` enabled internally.

```
ESP32 GPIO 14  →  Button MINUS (focus inward)  →  GND
ESP32 GPIO 27  →  Button ZERO  (set zero)       →  GND
ESP32 GPIO 13  →  Button PLUS  (focus outward)  →  GND
```

---

## Power Supply Considerations

| Rail | Source | Current |
|------|--------|---------|
| ESP32 3.3 V | USB / onboard LDO | ~200 mA |
| 28BYJ-48 5 V | External 5 V supply | 200–250 mA active |

Recommended: use the same USB power bank or 5 V adapter for both the ESP32
(via USB) and the ULN2003 (via screw terminal).

---

## ULN2003 and 3.3 V Logic

The ULN2003A input threshold voltage is typically 1 V. ESP32 GPIOs output
3.3 V high which reliably switches the ULN2003 on most modules.

If your specific ULN2003 module does NOT respond to 3.3 V inputs:
- Verify the GND connection between ESP32 and motor supply.
- Add a 4.7 kΩ pull-up resistor from each IN pin to 5 V.
- As a last resort, use a level-shifter or replace the module.

---

## Strapping Pin Warnings

The following ESP32 GPIOs have special boot-time functions and are deliberately
**not** used for stepper/button outputs:

| GPIO | Boot function | Risk |
|------|--------------|------|
| 0 | BOOT mode select | LOW at boot forces download mode |
| 2 | Boot log output | Must be low during flashing |
| 5 | SPI CS / boot log | Affects log output at boot |
| 12 | MTDI / FLASH voltage | HIGH at boot selects 1.8 V flash |
| 15 | MTDO / boot log | Affects boot log output |

GPIO 2 (onboard LED) is used as LED only, always after boot.

---

## Wiring Diagram (ASCII)

```
                  5V PSU
                    │
                   VCC──── ULN2003 VCC
                            IN1 ────── GPIO 26
                            IN2 ────── GPIO 25
  ESP32-WROOM-32    IN3 ────── GPIO 33
  ┌──────────┐      IN4 ────── GPIO 32
  │ GPIO 26  ├──────────────── IN1
  │ GPIO 25  ├──────────────── IN2
  │ GPIO 33  ├──────────────── IN3
  │ GPIO 32  ├──────────────── IN4
  │          │      GND ─────── GND (shared)
  │ GPIO 21  ├──────────── OLED SDA
  │ GPIO 22  ├──────────── OLED SCL
  │ 3.3V     ├──────────── OLED VCC
  │          │
  │ GPIO 14  ├──────────── BTN- ── GND
  │ GPIO 27  ├──────────── BTNZ ── GND
  │ GPIO 13  ├──────────── BTN+ ── GND
  │ GND      ├──────────────────── GND (common)
  └──────────┘
```
