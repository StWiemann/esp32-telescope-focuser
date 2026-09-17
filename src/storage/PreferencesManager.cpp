#include "PreferencesManager.h"
#include "util/Log.h"
#include "Config.h"

static const char* NS = "focuser";  // NVS namespace

void PreferencesManager::begin() {
    _load();
    LOG_INFO("Preferences loaded (SSID=%s, hostname=%s, maxPos=%d, backlash=%d)",
             _wifiSsid.isEmpty() ? "<none>" : _wifiSsid.c_str(),
             _hostname.c_str(), _maxPosition, _backlashSteps);
}

void PreferencesManager::_load() {
    _prefs.begin(NS, true);  // read-only

    _wifiSsid            = _prefs.getString("wifiSsid",    "");
    _wifiPass            = _prefs.getString("wifiPass",    "");
    _hostname            = _prefs.getString("hostname",    DEFAULT_HOSTNAME);
    _focuserName         = _prefs.getString("focName",     DEFAULT_FOCUSER_NAME);
    _maxPosition         = _prefs.getInt   ("maxPos",      DEFAULT_MAX_POSITION);
    _backlashSteps       = _prefs.getInt   ("backlash",    DEFAULT_BACKLASH_STEPS);
    _backlashApproachDir = (int8_t)_prefs.getInt("backlDir", DEFAULT_BACKLASH_APPROACH_DIR);
    _motorReversed       = _prefs.getBool  ("reverse",     DEFAULT_MOTOR_REVERSED);
    _stepsPerRev         = _prefs.getInt   ("stepsRev",    DEFAULT_STEPS_PER_REV);
    _motorPulleyTeeth    = _prefs.getInt   ("motorTeeth",  DEFAULT_MOTOR_PULLEY_TEETH);
    _focuserPulleyTeeth  = _prefs.getInt   ("focTeeth",    DEFAULT_FOCUSER_PULLEY_TEETH);
    _manualStepSize      = _prefs.getInt   ("manualStep",  DEFAULT_MANUAL_STEP_SIZE);
    _stepMedium          = _prefs.getInt   ("stepMd",      DEFAULT_STEP_MEDIUM);
    _stepLarge           = _prefs.getInt   ("stepLg",      DEFAULT_STEP_LARGE);
    _manualSpeed         = _prefs.getFloat ("manualSpeed", DEFAULT_MANUAL_SPEED);
    _motorMaxSpeed       = _prefs.getFloat ("maxSpeed",    DEFAULT_MOTOR_MAX_SPEED);

    _prefs.end();
}

void PreferencesManager::save() {
    _prefs.begin(NS, false);  // read-write

    _prefs.putString("wifiSsid",   _wifiSsid);
    _prefs.putString("wifiPass",   _wifiPass);
    _prefs.putString("hostname",   _hostname);
    _prefs.putString("focName",    _focuserName);
    _prefs.putInt   ("maxPos",     _maxPosition);
    _prefs.putInt   ("backlash",   _backlashSteps);
    _prefs.putInt   ("backlDir",   (int)_backlashApproachDir);
    _prefs.putBool  ("reverse",    _motorReversed);
    _prefs.putInt   ("stepsRev",   _stepsPerRev);
    _prefs.putInt   ("motorTeeth", _motorPulleyTeeth);
    _prefs.putInt   ("focTeeth",   _focuserPulleyTeeth);
    _prefs.putInt   ("manualStep", _manualStepSize);
    _prefs.putInt   ("stepMd",     _stepMedium);
    _prefs.putInt   ("stepLg",     _stepLarge);
    _prefs.putFloat ("manualSpeed",_manualSpeed);
    _prefs.putFloat ("maxSpeed",   _motorMaxSpeed);

    _prefs.end();
    LOG_INFO("Preferences saved");
}

void PreferencesManager::resetToDefaults() {
    _prefs.begin(NS, false);
    _prefs.clear();
    _prefs.end();
    _load();  // reload defaults
    LOG_WARN("Preferences reset to defaults");
}

// ── WiFi ──────────────────────────────────────────────────────────────────────

void PreferencesManager::setWifiCredentials(const String& ssid, const String& pass) {
    _wifiSsid = ssid;
    _wifiPass = pass;
    _prefs.begin(NS, false);
    _prefs.putString("wifiSsid", _wifiSsid);
    _prefs.putString("wifiPass", _wifiPass);
    _prefs.end();
    LOG_INFO("WiFi credentials saved (SSID=%s)", _wifiSsid.c_str());
}

void PreferencesManager::clearWifiCredentials() {
    _wifiSsid = "";
    _wifiPass = "";
    _prefs.begin(NS, false);
    _prefs.putString("wifiSsid", "");
    _prefs.putString("wifiPass", "");
    _prefs.end();
    LOG_WARN("WiFi credentials cleared");
}

// ── Setters with immediate partial NVS save ───────────────────────────────────

#define SETTER(field, key, type, putFn) \
    _prefs.begin(NS, false); \
    field = v; \
    _prefs.putFn(key, v); \
    _prefs.end();

void PreferencesManager::setHostname(const String& h) {
    _hostname = h;
    _prefs.begin(NS, false);
    _prefs.putString("hostname", _hostname);
    _prefs.end();
}

void PreferencesManager::setFocuserName(const String& n) {
    _focuserName = n;
    _prefs.begin(NS, false);
    _prefs.putString("focName", _focuserName);
    _prefs.end();
}

void PreferencesManager::setMaxPosition(int32_t v) {
    SETTER(_maxPosition, "maxPos", int32_t, putInt);
}

void PreferencesManager::setBacklashSteps(int32_t v) {
    SETTER(_backlashSteps, "backlash", int32_t, putInt);
}

void PreferencesManager::setBacklashApproachDir(int8_t d) {
    _backlashApproachDir = d;
    _prefs.begin(NS, false);
    _prefs.putInt("backlDir", (int)d);
    _prefs.end();
}

void PreferencesManager::setMotorReversed(bool v) {
    SETTER(_motorReversed, "reverse", bool, putBool);
}

void PreferencesManager::setStepsPerRev(int32_t v) {
    SETTER(_stepsPerRev, "stepsRev", int32_t, putInt);
}

void PreferencesManager::setMotorPulleyTeeth(int32_t v) {
    SETTER(_motorPulleyTeeth, "motorTeeth", int32_t, putInt);
}

void PreferencesManager::setFocuserPulleyTeeth(int32_t v) {
    SETTER(_focuserPulleyTeeth, "focTeeth", int32_t, putInt);
}

void PreferencesManager::setManualStepSize(int32_t v) {
    SETTER(_manualStepSize, "manualStep", int32_t, putInt);
}

void PreferencesManager::setStepMedium(int32_t v) {
    SETTER(_stepMedium, "stepMd", int32_t, putInt);
}

void PreferencesManager::setStepLarge(int32_t v) {
    SETTER(_stepLarge, "stepLg", int32_t, putInt);
}

void PreferencesManager::setManualSpeed(float v) {
    SETTER(_manualSpeed, "manualSpeed", float, putFloat);
}

void PreferencesManager::setMotorMaxSpeed(float v) {
    SETTER(_motorMaxSpeed, "maxSpeed", float, putFloat);
}

#undef SETTER
