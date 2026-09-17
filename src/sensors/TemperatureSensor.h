#pragma once
#include <Arduino.h>
// OneWireNg auto-selects the best platform driver. On ESP32 it uses per-core
// interrupt masking (portSET_INTERRUPT_MASK_FROM_ISR) instead of the global
// noInterrupts() used by the standard OneWire library, so WiFi high-priority
// interrupts are not blocked during 1-Wire communication.
#include <OneWireNg_CurrentPlatform.h>
#include <drivers/DSTherm.h>
#include <utils/Placeholder.h>
#include "Config.h"
#include "Pins.h"

/**
 * TemperatureSensor – non-blocking DS18B20 driver.
 *
 * Wiring
 * ------
 *  DS18B20 DATA  → GPIO PIN_TEMP_DATA (see Pins.h, default GPIO 4)
 *  DS18B20 VCC   → 3.3 V
 *  DS18B20 GND   → GND
 *  4.7 kΩ pull-up between DATA and 3.3 V  (required by 1-Wire protocol)
 *
 * Call begin() once in setup(), then loop() every main-loop iteration.
 * The sensor is optional: if absent, getTemperatureCelsius() returns NAN
 * and the firmware continues normally.
 */
class TemperatureSensor {
public:
    void  begin();
    void  loop();

    /** Latest temperature in °C, or NAN if no sensor / bad read. */
    float getTemperatureCelsius() const { return _temperature; }

    /** True if a DS thermometer was detected on the bus during begin(). */
    bool  isAvailable() const { return _available; }

private:
    // false = no internal pull-up (we have an external 4.7 kΩ resistor)
    OneWireNg_CurrentPlatform _ow{PIN_TEMP_DATA, false};
    DSTherm                   _drv{_ow};

    OneWireNg::Id _addr{};
    // Scratchpad has no default constructor; use a Placeholder for in-place creation
    Placeholder<DSTherm::Scratchpad> _scratchpad{};

    float    _temperature    = NAN;
    bool     _available      = false;
    bool     _requestPending = false;
    uint32_t _lastRequestMs  = 0;
};
