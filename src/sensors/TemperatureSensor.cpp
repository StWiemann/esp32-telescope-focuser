#include "sensors/TemperatureSensor.h"
#include "util/Log.h"

void TemperatureSensor::begin() {
    // Scan the bus for the first supported DS thermometer.
    // search() returns EC_SUCCESS for every device found, then EC_NO_DEVS.
    OneWireNg::Id id;
    _ow.searchReset();

    while (_ow.search(id) == OneWireNg::EC_SUCCESS) {
        // getFamilyName() returns a non-null string for any supported DS sensor
        if (DSTherm::getFamilyName(id) != nullptr) {
            memcpy(_addr, id, sizeof(OneWireNg::Id));
            _available = true;

            // Set 12-bit resolution (750 ms conversion time, 0.0625 °C steps).
            // Th/Tl alarm thresholds are unused, set to 0.
            _drv.writeScratchpad(_addr, 0, 0, DSTherm::RES_12_BIT);

            LOG_INFO("TemperatureSensor: %s found (family 0x%02X)",
                     DSTherm::getFamilyName(id), id[0]);
            break;
        }
    }

    if (!_available) {
        LOG_INFO("TemperatureSensor: no DS thermometer found – disabled");
    }
}

void TemperatureSensor::loop() {
    if (!_available) return;

    uint32_t now = millis();

    if (!_requestPending) {
        if (now - _lastRequestMs >= TEMP_POLL_MS) {
            // convTime=0: send conversion command and return immediately (non-blocking)
            _drv.convertTemp(_addr, 0, false);
            _lastRequestMs  = now;
            _requestPending = true;
        }
        return;
    }

    // Wait until the conversion is complete before reading
    if (now - _lastRequestMs < TEMP_CONVERSION_MS) return;

    _requestPending = false;

    // readScratchpad() constructs a Scratchpad in-place inside the Placeholder
    if (_drv.readScratchpad(_addr, _scratchpad) == OneWireNg::EC_SUCCESS) {
        const DSTherm::Scratchpad& sp =
            static_cast<DSTherm::Scratchpad&>(_scratchpad);

        // getTemp() returns milli-degrees Celsius (e.g. 20125 = 20.125 °C)
        float t = sp.getTemp() / 1000.0f;

        // 85.0 °C is the power-on reset value – discard it
        if (t != 85.0f) {
            _temperature = t;
        }
    }
}
