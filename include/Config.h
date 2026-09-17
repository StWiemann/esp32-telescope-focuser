#pragma once

/**
 * Config.h – Compile-time defaults and limits
 *
 * Values here are used when no value is found in NVS (first boot),
 * and serve as reference documentation for all configurable parameters.
 * Nothing here contains credentials.
 */

// ── Firmware ─────────────────────────────────────────────────────────────────
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

// ── Alpaca / mDNS ────────────────────────────────────────────────────────────
#define DEFAULT_HOSTNAME          "esp32-focuser"
#define DEFAULT_FOCUSER_NAME      "ESP32 P200 Focuser"
#define DEFAULT_AP_SSID_PREFIX    "ESP32-Focuser-"   // suffix = last 4 hex of MAC
#define DEFAULT_AP_PASSWORD       "focuser1"          // min. 8 chars for WPA2

// ── OLED ─────────────────────────────────────────────────────────────────────
// Typical SBC-OLED01 address is 0x3C; some modules are 0x3D.
// Compile flag: -D OLED_I2C_ADDR=0x3D  to override without touching this file.
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR  0x3C
#endif

// Display refresh intervals (ms)
#define DISPLAY_REFRESH_MOVING_MS   100    // 10 Hz while motor moves
#define DISPLAY_REFRESH_IDLE_MS    1000    // 1 Hz when idle

// ── Stepper / Motor ───────────────────────────────────────────────────────────
// 28BYJ-48 in half-step mode (HALF4WIRE) with AccelStepper.
// The actual gear ratio yields ~4076 half-steps per output shaft revolution,
// NOT the rounded 4096.  Calibrate with a dial gauge; see docs/CALIBRATION.md.
#define DEFAULT_STEPS_PER_REV     4076    // motor output shaft, half-step mode

// Pulley teeth for belt ratio calculation
#define DEFAULT_MOTOR_PULLEY_TEETH   16
#define DEFAULT_FOCUSER_PULLEY_TEETH 60

// Speed / acceleration defaults (motor half-steps / second)
#define DEFAULT_MOTOR_MAX_SPEED    500.0f   // conservative; 28BYJ-48 can miss steps above ~600
// Higher acceleration shortens the deceleration tail when halt() is called,
// reducing coasting distance after button release. 600 steps/s² is safe for the
// 28BYJ-48's 64:1 gearbox. Raise further if overshoot on halt is still too long.
#define DEFAULT_MOTOR_ACCEL        600.0f
#define MOTOR_MAX_SPEED_LIMIT     1000.0f   // hard upper limit, never exceeded regardless of config

// ── Focuser position ──────────────────────────────────────────────────────────
#define DEFAULT_MAX_POSITION      100000   // motor half-steps; calibrate to your mechanism
#define DEFAULT_BACKLASH_STEPS        0    // half-steps; set after mechanical calibration
// Backlash approach direction: 1 = outward (+), -1 = inward (-)
#define DEFAULT_BACKLASH_APPROACH_DIR  1

// If true, motor direction is reversed (clockwise vs. counter-clockwise sense)
#define DEFAULT_MOTOR_REVERSED     false

// ── Manual movement (buttons / web UI) ───────────────────────────────────────
// Three step sizes used by the web UI buttons and physical button short-press.
// Physical button uses the SMALL step; all three are configurable via web UI.
#define DEFAULT_MANUAL_STEP_SIZE     10    // small – physical button + web "<N<"
#define DEFAULT_STEP_MEDIUM         100    // medium web button "<<N<<"
#define DEFAULT_STEP_LARGE         1000    // large  web button "<<<N<<<"
#define DEFAULT_MANUAL_SPEED        400.0f // half-steps/s for manual moves
#define BUTTON_HOLD_DELAY_MS        500    // ms before continuous movement starts
#define BUTTON_ACCEL_DELAY_MS      2000    // ms before speed multiplier kicks in
#define BUTTON_SPEED_MULTIPLIER      4     // speed factor during accelerated hold

// ── Button debounce ───────────────────────────────────────────────────────────
#define BUTTON_DEBOUNCE_MS          20     // ms; ignore noise shorter than this

// ZERO long-press confirmation threshold (ms)
#define ZERO_LONG_PRESS_MS        2000     // hold ZERO for 2 s to zero position

// ── Temperature sensor (DS18B20 on 1-Wire) ───────────────────────────────────
// Non-blocking: a conversion request is sent every TEMP_POLL_MS ms; the result
// is read TEMP_CONVERSION_MS ms later (12-bit conversion takes ≤750 ms).
#define TEMP_POLL_MS            5000    // ms between temperature readings
#define TEMP_CONVERSION_MS       800    // ms to wait after requesting conversion

// ── WiFi ─────────────────────────────────────────────────────────────────────
#define WIFI_CONNECT_TIMEOUT_MS   20000    // ms; give up station attempt after this
#define WIFI_RECONNECT_INTERVAL_MS 30000   // ms; retry station connection every 30 s
#define CAPTIVE_DNS_TTL_MS           60    // TTL for captive DNS responses (seconds)

// ── Stepper FreeRTOS task ─────────────────────────────────────────────────────
#define STEPPER_TASK_STACK_SIZE   2048     // words (8 KB)
#define STEPPER_TASK_PRIORITY        2     // higher than Arduino loop (1)
#define STEPPER_TASK_CORE            1     // Core 1; same as loop()
#define STEPPER_TICK_MS              1     // ms per step-engine tick
