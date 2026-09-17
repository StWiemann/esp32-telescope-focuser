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
 *  CONNECTING  → CONNECTED  (WiFi.status() == WL_CONNECTED within timeout)
 *              → AP_MODE    (timeout)
 *  CONNECTED   → CONNECTING (WiFi dropped, auto-reconnect after interval)
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
    int32_t getRSSI()      const;

    // Called by CaptivePortal after credentials are saved
    void onCredentialsSaved();

private:
    PreferencesManager& _prefs;
    State    _state            = State::INIT;
    uint32_t _actionSince      = 0;   // ms: when CONNECTING state started
    int      _reconnectAttempts = 0;  // 0 = first connect attempt

    CaptivePortal* _portal = nullptr;

    void _startAP();
    void _startStation();
    void _onConnected();
    void _onDisconnected();

    static void _wifiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    static WiFiManager* _instance;  // for event callback
};
