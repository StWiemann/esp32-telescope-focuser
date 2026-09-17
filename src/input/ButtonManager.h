#pragma once
#include <Arduino.h>
#include "Config.h"

class FocuserController;

/**
 * ButtonManager – debounced button handling for MINUS, ZERO, PLUS.
 *
 * Buttons are wired normally-open, against GND, with INPUT_PULLUP.
 *
 * Behaviour per button
 * --------------------
 * PLUS / MINUS:
 *   Short press  → single step (manualStepSize)
 *   Hold         → continuous movement starts after BUTTON_HOLD_DELAY_MS
 *   Longer hold  → speed multiplied after BUTTON_ACCEL_DELAY_MS
 *
 * ZERO:
 *   Short press  → ignored (prevents accidental zeroing)
 *   Long press ≥ ZERO_LONG_PRESS_MS → sets current position to 0
 *   Motor must NOT be moving; ZERO during movement is silently ignored.
 */
class ButtonManager {
public:
    void begin();
    void loop(FocuserController& focuser);

private:
    enum class BtnId { MINUS = 0, ZERO = 1, PLUS = 2 };

    struct ButtonState {
        uint8_t  pin;
        bool     lastRaw      = true;   // true = released (INPUT_PULLUP)
        bool     pressed      = false;  // stable pressed state
        uint32_t pressedSince = 0;      // millis() when press was confirmed
        bool     holdFired    = false;  // continuous move has started
        bool     accelFired   = false;  // accelerated speed has started
        bool     stepIssued   = false;  // single step has been issued
    };

    ButtonState _btns[3];
    uint32_t    _lastDebounce[3] = {0, 0, 0};
    bool        _rawLast[3]      = {true, true, true};

    void _processDirectional(FocuserController& focuser, BtnId id, int8_t dir);
    void _processZero(FocuserController& focuser);
};
