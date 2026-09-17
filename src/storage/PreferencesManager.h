#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

/**
 * PreferencesManager – typed NVS (non-volatile storage) access.
 *
 * All settings are loaded once at begin() and cached in RAM.
 * Call save() to persist the current RAM copy back to NVS.
 *
 * Key naming: NVS keys must be ≤ 15 characters.
 * Namespace: "focuser"
 *
 * Position is deliberately NOT stored (always starts unzeroed after reset).
 */
class PreferencesManager {
public:
    void begin();
    void save();
    void resetToDefaults();

    // ── WiFi ──────────────────────────────────────────────────────────────
    const String& getWifiSsid()     const { return _wifiSsid; }
    const String& getWifiPass()     const { return _wifiPass; }
    void setWifiCredentials(const String& ssid, const String& pass);
    bool hasWifiCredentials() const { return _wifiSsid.length() > 0; }
    void clearWifiCredentials();

    // Arduino wifi_power_t value (78 = 19.5 dBm, 34 = 8.5 dBm, …)
    int getWifiTxPower() const { return _wifiTxPower; }
    void setWifiTxPower(int v);

    // ── Network identity ──────────────────────────────────────────────────
    const String& getHostname()     const { return _hostname; }
    void setHostname(const String& h);

    // ── Focuser general ───────────────────────────────────────────────────
    const String& getFocuserName()  const { return _focuserName; }
    void setFocuserName(const String& n);

    int32_t getMaxPosition()        const { return _maxPosition; }
    void setMaxPosition(int32_t v);

    int32_t getBacklashSteps()      const { return _backlashSteps; }
    void setBacklashSteps(int32_t v);

    int8_t getBacklashApproachDir() const { return _backlashApproachDir; }
    void setBacklashApproachDir(int8_t d);    // +1 = outward, -1 = inward

    bool getMotorReversed()         const { return _motorReversed; }
    void setMotorReversed(bool v);

    // ── Motor / mechanics ─────────────────────────────────────────────────
    int32_t getStepsPerRev()        const { return _stepsPerRev; }
    void setStepsPerRev(int32_t v);

    int32_t getMotorPulleyTeeth()   const { return _motorPulleyTeeth; }
    void setMotorPulleyTeeth(int32_t v);

    int32_t getFocuserPulleyTeeth() const { return _focuserPulleyTeeth; }
    void setFocuserPulleyTeeth(int32_t v);

    float getBeltRatio()            const {
        if (_motorPulleyTeeth == 0) return 1.0f;
        return (float)_focuserPulleyTeeth / (float)_motorPulleyTeeth;
    }

    // ── Manual movement ───────────────────────────────────────────────────
    // _manualStepSize = small step (used by physical buttons + web "<N<" button)
    int32_t getManualStepSize()     const { return _manualStepSize; }
    void setManualStepSize(int32_t v);

    int32_t getStepMedium()         const { return _stepMedium; }
    void setStepMedium(int32_t v);

    int32_t getStepLarge()          const { return _stepLarge; }
    void setStepLarge(int32_t v);

    float getManualSpeed()          const { return _manualSpeed; }
    void setManualSpeed(float v);

    float getMotorMaxSpeed()        const { return _motorMaxSpeed; }
    void setMotorMaxSpeed(float v);

private:
    Preferences _prefs;

    String  _wifiSsid;
    String  _wifiPass;
    String  _hostname;
    String  _focuserName;
    int32_t _maxPosition        = 0;
    int32_t _backlashSteps      = 0;
    int8_t  _backlashApproachDir = 1;
    bool    _motorReversed      = false;
    int32_t _stepsPerRev        = 0;
    int32_t _motorPulleyTeeth   = 0;
    int32_t _focuserPulleyTeeth = 0;
    int32_t _manualStepSize     = 0;
    int32_t _stepMedium         = 0;
    int32_t _stepLarge          = 0;
    float   _manualSpeed        = 0.0f;
    float   _motorMaxSpeed      = 0.0f;
    int     _wifiTxPower        = DEFAULT_WIFI_TX_POWER;

    void _load();
};
