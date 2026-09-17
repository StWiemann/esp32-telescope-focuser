#include "ButtonManager.h"
#include "focuser/FocuserController.h"
#include "Pins.h"
#include "util/Log.h"

void ButtonManager::begin() {
    _btns[0].pin = PIN_BTN_MINUS;
    _btns[1].pin = PIN_BTN_ZERO;
    _btns[2].pin = PIN_BTN_PLUS;

    for (auto& b : _btns) {
        pinMode(b.pin, INPUT_PULLUP);
    }
    LOG_INFO("Buttons initialized (MINUS=%d, ZERO=%d, PLUS=%d)",
             PIN_BTN_MINUS, PIN_BTN_ZERO, PIN_BTN_PLUS);
}

void ButtonManager::loop(FocuserController& focuser) {
    uint32_t now = millis();

    for (int i = 0; i < 3; i++) {
        auto& b  = _btns[i];
        bool raw = digitalRead(b.pin);  // HIGH = released (pull-up)

        // Debounce: record transition time
        if (raw != _rawLast[i]) {
            _lastDebounce[i] = now;
            _rawLast[i] = raw;
        }

        bool stable = (now - _lastDebounce[i]) >= BUTTON_DEBOUNCE_MS;
        bool nowPressed = stable && (raw == LOW);

        if (nowPressed && !b.pressed) {
            // Falling edge – button just confirmed pressed
            b.pressed      = true;
            b.pressedSince = now;
            b.holdFired    = false;
            b.accelFired   = false;
            b.stepIssued   = false;
        } else if (!nowPressed && b.pressed) {
            // Rising edge – button released
            if ((BtnId)i == BtnId::ZERO) {
                // Handled in _processZero on release
            } else {
                if (!b.holdFired && !b.stepIssued) {
                    // Short tap: issue one step
                    _processDirectional(focuser, (BtnId)i, (i == 2) ? 1 : -1);
                } else {
                    // Release during hold: stop motor
                    focuser.halt();
                    LOG_DEBUG("Button hold released – halt");
                }
            }
            b.pressed = false;
        }

        if (b.pressed) {
            uint32_t held = now - b.pressedSince;

            if ((BtnId)i == BtnId::ZERO) {
                _processZero(focuser);
            } else {
                int8_t dir = (i == 2) ? 1 : -1;

                if (held >= BUTTON_ACCEL_DELAY_MS && !b.accelFired) {
                    b.accelFired = true;
                    focuser.setManualSpeedMultiplier(BUTTON_SPEED_MULTIPLIER);
                    LOG_DEBUG("Button accel: speed x%d", BUTTON_SPEED_MULTIPLIER);
                }

                if (held >= BUTTON_HOLD_DELAY_MS && !b.holdFired) {
                    b.holdFired = true;
                    b.stepIssued = true;
                    focuser.startContinuousMove(dir);
                    LOG_DEBUG("Button hold: continuous move dir=%d", dir);
                }
            }
        } else if (!b.pressed && b.accelFired) {
            focuser.setManualSpeedMultiplier(1);
            b.accelFired = false;
        }
    }
}

void ButtonManager::_processDirectional(FocuserController& focuser, BtnId id, int8_t dir) {
    focuser.moveRelative(dir * focuser.getManualStepSize());
    _btns[(int)id].stepIssued = true;
    LOG_DEBUG("Button step: dir=%d, size=%d", dir, focuser.getManualStepSize());
}

void ButtonManager::_processZero(FocuserController& focuser) {
    auto& b = _btns[(int)BtnId::ZERO];
    if (!b.pressed) return;

    uint32_t held = millis() - b.pressedSince;
    if (held >= ZERO_LONG_PRESS_MS && !b.holdFired) {
        b.holdFired = true;
        if (focuser.isMoving()) {
            LOG_WARN("ZERO ignored: motor is moving");
            return;
        }
        focuser.zeroHere();
    }
}
