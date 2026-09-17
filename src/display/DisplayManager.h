#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "Pins.h"
#include "Config.h"

// Forward declaration to avoid circular includes
class FocuserController;

/**
 * DisplayManager – SBC-OLED01 (SSD1306/SH1106, 128×64, I²C) via U8g2.
 *
 * The controller type (SSD1306 vs SH1106) is selectable at compile time:
 *   -D OLED_SH1106   → SH1106 controller
 *   (default)         → SSD1306 controller
 *
 * Refresh rate:
 *   DISPLAY_REFRESH_MOVING_MS while the motor is running (10 Hz)
 *   DISPLAY_REFRESH_IDLE_MS   while idle (1 Hz)
 */
class DisplayManager {
public:
    void begin();
    void showBootScreen();

    /**
     * Call from loop(). Redraws the display at the configured refresh rate.
     * @param focuser     Reference to the FocuserController (for status data).
     * @param wifiOk      true if station WiFi is connected.
     * @param alpacaOk    true if Alpaca server is running.
     * @param tempCelsius Current temperature in °C, or NAN if unavailable.
     */
    void loop(const FocuserController& focuser, bool wifiOk, bool alpacaOk,
              float tempCelsius);

    // Force an immediate redraw on the next loop() call
    void forceRefresh() { _lastRefreshMs = 0; }

private:
    void _draw(const FocuserController& focuser, bool wifiOk, bool alpacaOk,
               float tempCelsius);

    uint32_t _lastRefreshMs = 0;
    bool     _initialized   = false;
};
