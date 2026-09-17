#pragma once

/**
 * Pins.h – Central GPIO configuration for ESP32-WROOM-32 Dev Board (30-pin)
 *
 * Selection rationale
 * -------------------
 *  - Strapping pins (0, 2, 5, 12, 15) are avoided for outputs.
 *    GPIO 0 (BOOT), GPIO 2 (LED), GPIO 5 (SPI CS), GPIO 12 (MTDI / FLASH voltage),
 *    GPIO 15 (MTDO / log output at boot) all influence boot behaviour.
 *  - GPIO 34–39 are INPUT-ONLY and cannot be used for outputs.
 *  - GPIO 21/22 are the default I²C pins on most dev boards; leave them for I²C.
 *  - GPIOs 26, 25, 33, 32 are safe for ULN2003 outputs (no boot constraints,
 *    fully bidirectional, can sink the small current needed).
 *  - GPIO 13 and 14 are safe for button inputs (internal pull-ups, not strapping).
 *    GPIO 27 is likewise safe.
 *
 * HARDWARE NOTE – ULN2003 and 3.3 V logic
 * -----------------------------------------
 *  The ULN2003A input threshold is typically 1 V. ESP32 GPIOs output 3.3 V high
 *  which easily drives the ULN2003 reliably in most modules.
 *  If your specific ULN2003 module does NOT respond correctly:
 *    - Check that GND is shared between the ESP32 and the 5 V supply.
 *    - Consider a 2K–10K pull-up on the ULN2003 input side.
 *
 * Modify any pin below to suit your actual wiring, then re-compile.
 */

// ── ULN2003 stepper driver outputs ──────────────────────────────────────────
// AccelStepper HALF4WIRE constructor order for 28BYJ-48:
//   AccelStepper(HALF4WIRE, IN1, IN3, IN2, IN4)
// The crossed order (IN1, IN3, IN2, IN4) matches AccelStepper's internal
// half-step sequence for bipolar winding pairs.

#define PIN_STEP_IN1  26   // ULN2003 IN1 → motor Blue  (coil A+)
#define PIN_STEP_IN2  25   // ULN2003 IN2 → motor Pink  (coil A-)
#define PIN_STEP_IN3  33   // ULN2003 IN3 → motor Yellow(coil B+)
#define PIN_STEP_IN4  32   // ULN2003 IN4 → motor Orange(coil B-)

// ── Taster (gegen GND, INPUT_PULLUP) ────────────────────────────────────────
#define PIN_BTN_MINUS  14   // Button "–"    (focus inward)
#define PIN_BTN_ZERO   27   // Button "ZERO" (set position = 0)
#define PIN_BTN_PLUS   13   // Button "+"    (focus outward)

// ── OLED I²C ────────────────────────────────────────────────────────────────
// SBC-OLED01 (SSD1306 128×64, I²C)
// Default I²C pins on ESP32; change only if your board differs.
#define PIN_OLED_SDA   21
#define PIN_OLED_SCL   22

// ── DS18B20 temperature sensor (1-Wire) ─────────────────────────────────────
// Connect DS18B20 DATA to this pin.  Add a 4.7 kΩ pull-up from DATA to 3.3 V.
// Change this pin to any free GPIO if yours is already occupied.
#define PIN_TEMP_DATA    4

// ── Built-in LED (optional; present on most DevKit boards) ──────────────────
#define PIN_LED_BUILTIN  2   // strapping pin – used as output AFTER boot only
