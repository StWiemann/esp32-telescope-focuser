#include "StepperController.h"
#include "util/Log.h"

void StepperController::begin(float maxSpeed, float accel, bool reversed) {
    _reversed = reversed;

    // AccelStepper pin order for 28BYJ-48 HALF4WIRE: IN1, IN3, IN2, IN4
    // This crossed ordering matches AccelStepper's 8-step half-step sequence
    // to the motor's bipolar coil pairs (A+/A−/B+/B−).
    _stepper = new AccelStepper(
        AccelStepper::HALF4WIRE,
        PIN_STEP_IN1,
        PIN_STEP_IN3,
        PIN_STEP_IN2,
        PIN_STEP_IN4
    );

    float clampedSpeed = min(maxSpeed, MOTOR_MAX_SPEED_LIMIT);
    _stepper->setMaxSpeed(clampedSpeed);
    _stepper->setAcceleration(accel);
    _stepper->setCurrentPosition(0);

    // Disable coils initially (saves power, reduces heat)
    _stepper->disableOutputs();

    xTaskCreatePinnedToCore(
        taskFunc,
        "StepperTask",
        STEPPER_TASK_STACK_SIZE,
        this,
        STEPPER_TASK_PRIORITY,
        &_taskHandle,
        STEPPER_TASK_CORE
    );

    LOG_INFO("Stepper initialized (speed=%.0f, accel=%.0f, reversed=%s)",
             clampedSpeed, accel, reversed ? "yes" : "no");
}

void StepperController::taskFunc(void* param) {
    StepperController* ctrl = static_cast<StepperController*>(param);
    for (;;) {
        ctrl->tick();
        vTaskDelay(pdMS_TO_TICKS(STEPPER_TICK_MS));
    }
}

void StepperController::tick() {
    if (!_enabled) return;

    // Handle a pending position reset (e.g. ZERO command)
    if (_resetPending) {
        _resetPending = false;
        int32_t val   = _resetValue;
        int32_t phys  = _reversed ? -val : val;
        _stepper->setCurrentPosition(phys);
        _targetStep  = val;   // prevent immediate re-move
        _currentStep = val;
        _moving      = false;
        _stepper->disableOutputs();
        return;
    }

    int32_t target = _targetStep;  // volatile atomic read

    // Map logical target to physical position (handle reversal)
    int32_t physTarget = _reversed ? -target : target;
    _stepper->moveTo(physTarget);
    _stepper->run();

    int32_t physPos = _stepper->currentPosition();
    _currentStep = _reversed ? -physPos : physPos;  // volatile atomic write
    _moving      = _stepper->isRunning();            // volatile atomic write

    // Disable coils when motor stops (reduces heat on 28BYJ-48)
    if (!_moving && _stepper->currentPosition() == _stepper->targetPosition()) {
        _stepper->disableOutputs();
    }
}

// ── Thread-safe public interface ──────────────────────────────────────────────

void StepperController::setTarget(int32_t absoluteStep) {
    // ESP32 32-bit volatile write is atomic; no spinlock needed for single int32_t
    _targetStep = absoluteStep;
    if (_enabled) {
        _stepper->enableOutputs();
    }
}

int32_t StepperController::getCurrentStep() const {
    return _currentStep;  // volatile atomic read
}

bool StepperController::isMoving() const {
    return _moving;       // volatile atomic read
}

void StepperController::stop() {
    portENTER_CRITICAL(&_mux);
    _stepper->stop();
    // AccelStepper::stop() computes a deceleration curve and sets its internal
    // target to the position where the motor will naturally come to rest.
    // We must read targetPosition() (not currentPosition()) so that tick()'s
    // subsequent moveTo() call agrees with that stopping point — otherwise the
    // motor overshoots during deceleration and then reverses back.
    int32_t physStop = _stepper->targetPosition();
    _targetStep = _reversed ? -physStop : physStop;
    portEXIT_CRITICAL(&_mux);
}

void StepperController::resetPosition(int32_t newPos) {
    // Write value first, then set flag (both are volatile 32-bit writes → atomic on ESP32)
    _resetValue   = newPos;
    _resetPending = true;
    // _currentStep will be updated on next tick()
}

void StepperController::enable(bool en) {
    _enabled = en;
    if (en) {
        _stepper->enableOutputs();
    } else {
        _stepper->stop();
        _stepper->disableOutputs();
        _moving = false;
    }
}

void StepperController::setMaxSpeed(float stepsPerSec) {
    float clamped = min(stepsPerSec, MOTOR_MAX_SPEED_LIMIT);
    _stepper->setMaxSpeed(clamped);
}

void StepperController::setAcceleration(float stepsPerSec2) {
    _stepper->setAcceleration(stepsPerSec2);
}
