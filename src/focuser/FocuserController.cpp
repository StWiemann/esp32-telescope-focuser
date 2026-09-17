#include "FocuserController.h"
#include "util/Log.h"
#include "Config.h"

static const int CMD_QUEUE_LENGTH = 8;

FocuserController::FocuserController(StepperController& stepper,
                                     PreferencesManager& prefs)
    : _stepper(stepper), _prefs(prefs) {}

void FocuserController::begin() {
    _cmdQueue    = xQueueCreate(CMD_QUEUE_LENGTH, sizeof(FocuserCommand));
    _maxPosition = _prefs.getMaxPosition();
    _zeroed      = false;
    _targetPosition = 0;

    _stepper.begin(
        _prefs.getMotorMaxSpeed(),
        DEFAULT_MOTOR_ACCEL,
        _prefs.getMotorReversed()
    );

    LOG_INFO("FocuserController ready (maxPos=%d, backlash=%d, approach=%+d)",
             _maxPosition, _prefs.getBacklashSteps(), _prefs.getBacklashApproachDir());
}

void FocuserController::loop() {
    // ── Drain command queue ───────────────────────────────────────────────
    FocuserCommand cmd;
    while (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
        _executeCommand(cmd);
    }

    // ── Two-phase (backlash) state machine ────────────────────────────────
    if (_inPhase1 && !_stepper.isMoving()) {
        _inPhase1 = false;
        LOG_INFO("Backlash phase1 done → phase2 target=%d", _phase2Target);
        _stepper.setTarget(_phase2Target);
    }
}

void FocuserController::_executeCommand(const FocuserCommand& cmd) {
    switch (cmd.type) {
        case FocuserCmd::MOVE_ABSOLUTE: _doMoveAbsolute(cmd.value); break;
        case FocuserCmd::HALT:          _doHalt();                   break;
        case FocuserCmd::ZERO:          _doZero();                   break;
        case FocuserCmd::CONT_MOVE:     _doStartContinuous((int8_t)(cmd.value > 0 ? 1 : -1)); break;
        default: break;
    }
}

void FocuserController::_doMoveAbsolute(int32_t absolutePos) {
    if (!_isInRange(absolutePos)) {
        LOG_ERROR("Move rejected: position %d out of range [0, %d]",
                  absolutePos, _maxPosition);
        return;
    }

    int32_t current = getCurrentPosition();
    LOG_INFO("Move %d -> %d", current, absolutePos);

    MoveSequence seq = BacklashPolicy::compute(
        current,
        absolutePos,
        _prefs.getBacklashSteps(),
        _prefs.getBacklashApproachDir(),
        0,
        _maxPosition
    );

    _targetPosition = absolutePos;  // logical target is always the final position

    if (seq.twoPhase) {
        LOG_INFO("Backlash: overshoot to %d, then %d", seq.intermediate, seq.target);
        _inPhase1    = true;
        _phase2Target = seq.target;
        _stepper.setTarget(seq.intermediate);
    } else {
        _stepper.setTarget(absolutePos);
    }
}

void FocuserController::_doHalt() {
    _inPhase1 = false;
    _stepper.stop();
    // Sync logical target to wherever the motor stopped
    _targetPosition = getCurrentPosition();
    LOG_INFO("Halt – position %d", _targetPosition);
}

void FocuserController::_doZero() {
    if (_stepper.isMoving()) {
        LOG_WARN("ZERO ignored: motor is moving");
        return;
    }
    // Reset AccelStepper's internal step counter to 0 via the thread-safe
    // resetPosition() flag mechanism in StepperController::tick().
    // The motor does not move; it just re-labels its current position as 0.
    _stepper.resetPosition(0);

    _zeroed         = true;
    _targetPosition = 0;
    _inPhase1       = false;

    LOG_INFO("Position zeroed");
}

void FocuserController::_doStartContinuous(int8_t dir) {
    // For continuous movement, move to max or min depending on direction
    int32_t target = (dir > 0) ? _maxPosition : 0;
    _stepper.setTarget(target);
    _targetPosition = target;
    LOG_DEBUG("Continuous move dir=%d, target=%d", dir, target);
}

// ── Thread-safe public interface ─────────────────────────────────────────────

void FocuserController::moveAbsolute(int32_t absolutePos) {
    FocuserCommand cmd{FocuserCmd::MOVE_ABSOLUTE, absolutePos};
    xQueueSend(_cmdQueue, &cmd, 0);
}

void FocuserController::moveRelative(int32_t delta) {
    int32_t newTarget = getCurrentPosition() + delta;
    newTarget = constrain(newTarget, 0, _maxPosition);
    moveAbsolute(newTarget);
}

void FocuserController::halt() {
    FocuserCommand cmd{FocuserCmd::HALT, 0};
    xQueueSend(_cmdQueue, &cmd, 0);
}

void FocuserController::zeroHere() {
    FocuserCommand cmd{FocuserCmd::ZERO, 0};
    xQueueSend(_cmdQueue, &cmd, 0);
}

void FocuserController::startContinuousMove(int8_t dir) {
    FocuserCommand cmd{FocuserCmd::CONT_MOVE, (int32_t)dir};
    xQueueSend(_cmdQueue, &cmd, 0);
}

void FocuserController::enqueueMove(int32_t absolutePos) { moveAbsolute(absolutePos); }
void FocuserController::enqueueHalt()                    { halt(); }
void FocuserController::enqueueZero()                    { zeroHere(); }

int32_t FocuserController::getCurrentPosition() const {
    return _stepper.getCurrentStep();  // volatile read
}

void FocuserController::setManualSpeedMultiplier(int multiplier) {
    float base  = _prefs.getManualSpeed();
    float speed = min(base * multiplier, MOTOR_MAX_SPEED_LIMIT);
    _stepper.setMaxSpeed(speed);
}

void FocuserController::applyConfig() {
    _maxPosition = _prefs.getMaxPosition();
    _stepper.setMaxSpeed(_prefs.getMotorMaxSpeed());
}
