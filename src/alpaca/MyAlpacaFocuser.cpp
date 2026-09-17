#include "MyAlpacaFocuser.h"
#include "util/Log.h"
#include "Config.h"

MyAlpacaFocuser::MyAlpacaFocuser(FocuserController& focuser, PreferencesManager& prefs,
                                 TemperatureSensor& tempSensor)
    : _focuser(focuser), _prefs(prefs), _tempSensor(tempSensor) {
    // Set initial Alpaca device name from preferences
    strlcpy(_device_name, _prefs.getFocuserName().c_str(), sizeof(_device_name));
    strlcpy(_device_description,
            "ESP32 P200 Crayford Focuser (28BYJ-48 / ULN2003)",
            sizeof(_device_description));
}

void MyAlpacaFocuser::Begin() {
    AlpacaFocuser::Begin();
    LOG_INFO("Alpaca Focuser device started (name='%s', maxStep=%d)",
             _device_name, _prefs.getMaxPosition());
}

void MyAlpacaFocuser::Loop() {
    // AlpacaServer::Loop() already calls CheckClientConnectionTimeout() for
    // all registered devices. Nothing extra needed here.
}

void MyAlpacaFocuser::AlpacaReadJson(JsonObject& root) {
    // Delegate base-class settings (clientIDs, name, description)
    AlpacaDevice::AlpacaReadJson(root);

    // Restore our settings from the JSON (saved by AlpacaWriteJson below)
    if (root["maxPosition"].is<int>())
        _prefs.setMaxPosition(root["maxPosition"].as<int32_t>());
    if (root["backlashSteps"].is<int>())
        _prefs.setBacklashSteps(root["backlashSteps"].as<int32_t>());
    if (root["motorReversed"].is<bool>())
        _prefs.setMotorReversed(root["motorReversed"].as<bool>());
}

void MyAlpacaFocuser::AlpacaWriteJson(JsonObject& root) {
    AlpacaDevice::AlpacaWriteJson(root);

    // Persist focuser-specific settings alongside the Alpaca base settings
    root["maxPosition"]   = _prefs.getMaxPosition();
    root["backlashSteps"] = _prefs.getBacklashSteps();
    root["motorReversed"] = _prefs.getMotorReversed();
}
