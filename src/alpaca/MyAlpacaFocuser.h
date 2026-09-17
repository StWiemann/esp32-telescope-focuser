#pragma once
#include <limits>
#include <AlpacaFocuser.h>
#include "focuser/FocuserController.h"
#include "storage/PreferencesManager.h"
#include "sensors/TemperatureSensor.h"
#include "Config.h"

/**
 * MyAlpacaFocuser – concrete Alpaca Focuser V3 implementation.
 *
 * Inherits AlpacaFocuser (from ESP32AlpacaDevices2) and implements all pure
 * virtual methods by delegating to FocuserController.
 *
 * Temperature reporting:
 *   A DS18B20 sensor is read via TemperatureSensor.  The Alpaca Temperature
 *   property returns the real value when the sensor is connected, or NaN when
 *   it is absent.  TempCompAvailable = false (we read temperature but do not
 *   drive the motor from it automatically).
 *
 * Settings persistence:
 *   AlpacaDevice settings (name, description, clientIDs) are persisted by
 *   the base class in LittleFS /settings.json via AlpacaReadJson/WriteJson.
 *   Focuser-specific settings (maxPos, backlash, …) are in NVS via
 *   PreferencesManager.
 */
class MyAlpacaFocuser : public AlpacaFocuser {
public:
    MyAlpacaFocuser(FocuserController& focuser, PreferencesManager& prefs,
                    TemperatureSensor& tempSensor);

    void Begin();
    void Loop();

    // Settings serialisation (called by AlpacaServer when saving /settings.json)
    void AlpacaReadJson(JsonObject& root) override;
    void AlpacaWriteJson(JsonObject& root) override;

private:
    FocuserController& _focuser;
    PreferencesManager& _prefs;
    TemperatureSensor& _tempSensor;

    // ── AlpacaFocuser pure virtual implementations ─────────────────────────
    const char* const _getFirmwareVersion() override { return FIRMWARE_VERSION; }

    const bool _getAbsolut()        override { return true; }
    const bool _getIsMoving()       override { return _focuser.isMoving(); }
    const int32_t _getMaxIncrement()override { return _prefs.getMaxPosition(); }
    const int32_t _getMaxStep()     override { return _prefs.getMaxPosition(); }
    const int32_t _getPosition()    override { return _focuser.getCurrentPosition(); }

    // Step size in microns per step.
    // Set to 0.0 until the user calibrates (see docs/CALIBRATION.md).
    const double _getStepSize()     override { return 0.0; }

    // Temperature: read from DS18B20 sensor; compensation not implemented
    const bool   _getTempComp()         override { return false; }
    const bool   _getTempCompAvailable()override { return false; }
    const double _getTemperature()      override {
        float t = _tempSensor.getTemperatureCelsius();
        if (std::isnan(t)) return std::numeric_limits<double>::quiet_NaN();
        return static_cast<double>(t);
    }

    const bool _putTempComp(bool) override {
        // TempCompAvailable=false, so this should never be called by a compliant
        // client; return false to indicate not-implemented.
        return false;
    }

    const bool _putHalt() override {
        _focuser.enqueueHalt();
        return true;
    }

    const bool _putMove(int32_t position) override {
        if (position < 0 || position > _prefs.getMaxPosition()) {
            return false;  // out-of-range; Alpaca returns generic error
        }
        _focuser.enqueueMove(position);
        return true;
    }

    // Optional Alpaca extensions – not used
    const bool _putAction(const char*, const char*, char*, size_t) override { return false; }
    const bool _putCommandBlind(const char*, const char*, bool&)   override { return false; }
    const bool _putCommandBool(const char*, const char*, bool&)    override { return false; }
    const bool _putCommandString(const char*, const char*, char*, size_t) override { return false; }
};
