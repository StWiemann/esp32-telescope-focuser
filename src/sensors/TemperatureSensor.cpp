#include "TemperatureSensor.h"
#include "util/Log.h"
#include <cmath>

void TemperatureSensor::begin() {
    _sensors.begin();
    int count = _sensors.getDeviceCount();
    _available = (count > 0);

    if (_available) {
        // Use 12-bit resolution (0.0625 °C precision); conversion takes ≤750 ms.
        _sensors.setResolution(12);
        // Do not wait for conversion in-line; we drive the bus asynchronously.
        _sensors.setWaitForConversion(false);
        LOG_INFO("DS18B20 found (%d device(s)) on GPIO %d", count, PIN_TEMP_DATA);
    } else {
        LOG_WARN("No DS18B20 found on GPIO %d (check wiring / pull-up)", PIN_TEMP_DATA);
    }
}

void TemperatureSensor::loop() {
    if (!_available) return;

    uint32_t now = millis();

    if (!_requestPending) {
        // Time to start a new conversion
        if (now - _lastRequestMs >= TEMP_POLL_MS) {
            _sensors.requestTemperaturesByIndex(0);
            _lastRequestMs  = now;
            _requestPending = true;
        }
        return;
    }

    // Wait for the conversion to complete before reading
    if (now - _lastRequestMs < TEMP_CONVERSION_MS) return;

    _requestPending = false;
    float t = _sensors.getTempCByIndex(0);

    if (t == DEVICE_DISCONNECTED_C || t == 85.0f) {
        // 85 °C is the power-on default and indicates a bad read
        LOG_WARN("TemperatureSensor: bad reading (%.1f °C) – ignored", t);
        return;
    }

    _temperature = t;
    LOG_DEBUG("Temperature: %.2f °C", _temperature);
}
