#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "StepperController.h"
#include "BacklashPolicy.h"
#include "storage/PreferencesManager.h"

/**
 * FocuserController – single source of truth for focuser state.
 *
 * Thread-safety model
 * -------------------
 * Public mutating methods (moveAbsolute, moveRelative, halt, zeroHere,
 * startContinuousMove, enqueueMove, enqueueHalt, enqueueZero) may be called
 * from ANY context (loop task, async HTTP callback, button handler).
 * They enqueue a command to _cmdQueue; the actual state transitions happen
 * in loop() on the loop task.
 *
 * Read-only accessors (getCurrentPosition, getTargetPosition, isMoving,
 * isZeroed) may also be called from any context; they read volatile values.
 */

enum class FocuserCmd : uint8_t {
    MOVE_ABSOLUTE = 0,
    HALT          = 1,
    ZERO          = 2,
    CONT_MOVE     = 3,   // continuous move: value = direction (+1/-1) * 1000
    SET_SPEED     = 4,   // value = speed * 100 (scaled to avoid float in queue)
};

struct FocuserCommand {
    FocuserCmd type;
    int32_t    value;
};

class FocuserController {
public:
    explicit FocuserController(StepperController& stepper,
                               PreferencesManager& prefs);

    void begin();
    void loop();   // must be called from Arduino loop()

    // ── Commands (thread-safe, may be called from any task) ──────────────
    void moveAbsolute(int32_t absolutePos);
    void moveRelative(int32_t delta);
    void halt();
    void zeroHere();

    // Continuous movement for held-button operation
    void startContinuousMove(int8_t dir);  // dir: +1 or -1

    // Convenience wrappers that post to the command queue
    // (used by Alpaca callbacks running in async-TCP task context)
    void enqueueMove(int32_t absolutePos);
    void enqueueHalt();
    void enqueueZero();

    void setManualSpeedMultiplier(int multiplier);  // 1 = normal, N = faster

    // ── Read-only accessors (thread-safe) ─────────────────────────────────
    int32_t getCurrentPosition() const;
    int32_t getTargetPosition()  const { return _targetPosition; }
    bool    isMoving()           const { return _stepper.isMoving(); }
    bool    isZeroed()           const { return _zeroed; }
    int32_t getMaxPosition()     const { return _maxPosition; }
    // 0 once zeroed; −maxPosition before zero so the focuser can retract to the stop.
    int32_t getMinPosition()     const { return _minPosition(); }
    int32_t getManualStepSize()  const { return _prefs.getManualStepSize(); }
    float   getManualSpeed()     const { return _prefs.getManualSpeed(); }

    // Configuration update (also calls setters on StepperController)
    void applyConfig();

private:
    StepperController& _stepper;
    PreferencesManager& _prefs;

    QueueHandle_t _cmdQueue;

    volatile int32_t _targetPosition = 0;
    bool             _zeroed         = false;
    int32_t          _maxPosition    = DEFAULT_MAX_POSITION;

    // Two-phase move state (backlash compensation)
    bool    _inPhase1    = false;
    int32_t _phase2Target = 0;

    void _executeCommand(const FocuserCommand& cmd);
    void _doMoveAbsolute(int32_t absolutePos);
    void _doHalt();
    void _doZero();
    void _doStartContinuous(int8_t dir);

    int32_t _minPosition() const { return _zeroed ? 0 : -_maxPosition; }

    bool _isInRange(int32_t pos) const {
        return pos >= _minPosition() && pos <= _maxPosition;
    }
};
