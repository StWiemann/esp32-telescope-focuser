#pragma once
#include <Arduino.h>
#include <AccelStepper.h>
#include "Pins.h"
#include "Config.h"

/**
 * StepperController – non-blocking 28BYJ-48 driver via AccelStepper.
 *
 * AccelStepper runs exclusively inside a dedicated FreeRTOS task on Core 1.
 * Communication with the rest of the firmware uses volatile 32-bit variables
 * (atomic on ESP32's 32-bit Xtensa architecture) and a portMUX spinlock for
 * the stop() operation which must be transactionally consistent.
 *
 * Step mode: HALF4WIRE (8 half-steps per electrical cycle).
 * AccelStepper pin order for 28BYJ-48:  IN1, IN3, IN2, IN4
 * See Pins.h for the rationale.
 *
 * NOTE: Do NOT call any AccelStepper method from outside this class.
 *       All AccelStepper interactions happen only in tick() / the task.
 */
class StepperController {
public:
    StepperController() = default;
    ~StepperController() = default;

    void begin(float maxSpeed, float accel, bool reversed);

    // Thread-safe accessors called from loop() / HTTP callbacks ─────────────
    void    setTarget(int32_t absoluteStep);
    int32_t getCurrentStep() const;
    bool    isMoving()        const;
    void    stop();              // immediately halt and sync target to current
    void    enable(bool en);     // power motor coils on/off

    void    setMaxSpeed(float stepsPerSec);
    void    setAcceleration(float stepsPerSec2);
    bool    isReversed() const { return _reversed; }

    /**
     * Reset the AccelStepper internal counter so the current physical position
     * is declared as `newPos`.  Safe to call from loop() task while the
     * stepper task is running because the flag + value are set atomically
     * (32-bit aligned volatile writes on ESP32 Xtensa are single-instruction).
     * The motor must be stopped before calling this.
     */
    void resetPosition(int32_t newPos);

    // Called only from the FreeRTOS stepper task
    void tick();
    static void taskFunc(void* param);

private:
    AccelStepper* _stepper    = nullptr;
    TaskHandle_t  _taskHandle = nullptr;

    volatile int32_t _targetStep    = 0;
    volatile int32_t _currentStep   = 0;
    volatile bool    _moving        = false;
    volatile bool    _enabled       = true;
    bool             _reversed      = false;

    // For resetPosition() – set by loop task, consumed by stepper task
    volatile bool    _resetPending  = false;
    volatile int32_t _resetValue    = 0;

    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};
