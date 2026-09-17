#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "storage/PreferencesManager.h"
#include "Config.h"

// Forward declaration
class CaptivePortal;

/**
 * WiFiManager – station connect + AP fallback.
 *
 * State machine
 * -------------
 *  INIT        → CONNECTING (credentials available)
 *              → AP_MODE    (no credentials)
 *  CONNECTING  → CONNECTED  (GOT_IP within timeout)
 *              → AP_MODE    (first-boot timeout)
 *  CONNECTED   → CONNECTING (link dropped; ESP32 auto-reconnect, no WiFi.begin())
 *  AP_MODE     → (stays until credentials saved + restart)
 *
 * After the captive portal saves credentials, the ESP32 restarts automatically.
 */
class WiFiManager {
public:
    enum class State {
        INIT,
        CONNECTING,
        CONNECTED,
        AP_MODE,
    };

    explicit WiFiManager(PreferencesManager& prefs);

    void begin();
    void loop();

    bool    isConnected()  const { return _state == State::CONNECTED; }
    bool    isAPMode()     const { return _state == State::AP_MODE; }
    State   getState()     const { return _state; }
    String  getIP()        const;
    String  getSSID()      const;
    String  getBSSID()     const;
    int32_t getRSSI()      const;
    uint8_t getLastDisconnectReason() const { return _lastDisconnectReason; }
    const char* getLastDisconnectReasonName() const;

    // Called by CaptivePortal after credentials are saved
    void onCredentialsSaved();

    // Wipe stored SSID/password and reboot into AP mode (deferred so HTTP can finish).
    void clearCredentialsAndReboot();

private:
    PreferencesManager& _prefs;
    State    _state             = State::INIT;
    uint32_t _actionSince       = 0;   // ms: when CONNECTING state started
    int      _reconnectAttempts = 0;   // 0 = first connect attempt this boot
    uint32_t _rebootAt          = 0;   // millis() deadline, 0 = none
    uint8_t  _lastDisconnectReason = 0;

    CaptivePortal* _portal = nullptr;

    void _startAP();
    void _startStation();
    void _onConnected();
    void _onDisconnected(uint8_t reason);

    static const char* _reasonName(uint8_t reason);
    static void _wifiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    static WiFiManager* _instance;  // for event callback
};
