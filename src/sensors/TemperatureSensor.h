#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"
#include "Pins.h"

/**
 * TemperatureSensor – non-blocking DS18B20 driver.
 *
 * Wiring
 * ------
 *  DS18B20 DATA  → GPIO PIN_TEMP_DATA (see Pins.h)
 *  DS18B20 VCC   → 3.3 V
 *  DS18B20 GND   → GND
 *  4.7 kΩ pull-up between DATA and 3.3 V  (required by 1-Wire protocol)
 *
 * Call begin() once in setup(), then loop() every main-loop iteration.
 * Read the result with getTemperatureCelsius() (returns NAN when unavailable).
 *
 * Non-blocking design: conversion is requested every TEMP_POLL_MS ms;
 * the firmware continues running during the ≤750 ms conversion window.
 * The result is read and cached after TEMP_CONVERSION_MS ms.
 */
class TemperatureSensor {
public:
    void  begin();
    void  loop();

    /**
     * Latest temperature in °C.
     * Returns NAN if no sensor was found or the last reading was invalid.
     */
    float getTemperatureCelsius() const { return _temperature; }

    /** True if a DS18B20 was detected on the bus during begin(). */
    bool  isAvailable() const { return _available; }

private:
    OneWire          _oneWire{PIN_TEMP_DATA};
    DallasTemperature _sensors{&_oneWire};

    float    _temperature      = NAN;
    bool     _available        = false;
    bool     _requestPending   = false;
    uint32_t _lastRequestMs    = 0;
};
