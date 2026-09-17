#include "WiFiManager.h"
#include "CaptivePortal.h"
#include "util/Log.h"

WiFiManager* WiFiManager::_instance = nullptr;

WiFiManager::WiFiManager(PreferencesManager& prefs) : _prefs(prefs) {
    _instance = this;
}

void WiFiManager::begin() {
    WiFi.onEvent(_wifiEventHandler);
    WiFi.setHostname(_prefs.getHostname().c_str());

    if (_prefs.hasWifiCredentials()) {
        _startStation();
    } else {
        LOG_INFO("No WiFi credentials – starting AP mode");
        _startAP();
    }
}

void WiFiManager::loop() {
    if (_rebootAt != 0 && millis() >= _rebootAt) {
        ESP.restart();
    }

    uint32_t now = millis();

    switch (_state) {
        case State::CONNECTING:
            if (_reconnectAttempts == 0) {
                if (now - _actionSince >= WIFI_CONNECT_TIMEOUT_MS) {
                    LOG_WARN("WiFi initial connect timed out – starting AP for reconfiguration");
                    WiFi.disconnect(true);
                    _startAP();
                }
            } else if (now - _actionSince >= WIFI_RECONNECT_INTERVAL_MS) {
                // Auto-reconnect did not restore the link – one explicit begin() kick.
                LOG_WARN("WiFi auto-reconnect timed out – calling begin() again");
                _startStation();
            }
            break;

        case State::CONNECTED:
            break;

        case State::AP_MODE:
            if (_portal) _portal->loop();
            break;

        default: break;
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

void WiFiManager::_startStation() {
    _state       = State::CONNECTING;
    _actionSince = millis();

    WiFi.mode(WIFI_STA);
    // Disable modem sleep – many consumer APs disconnect ESP32 clients that
    // sleep between beacons. Costs ~30 mA extra but massively improves link stability.
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(_prefs.getWifiSsid().c_str(), _prefs.getWifiPass().c_str());
    LOG_INFO("Connecting to SSID '%s' … (attempt %d)",
             _prefs.getWifiSsid().c_str(), _reconnectAttempts + 1);
}

void WiFiManager::_startAP() {
    if (_portal) {
        delete _portal;
        _portal = nullptr;
    }

    _state = State::AP_MODE;

    WiFi.mode(WIFI_AP);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apSsid[32];
    snprintf(apSsid, sizeof(apSsid), "%s%02X%02X",
             DEFAULT_AP_SSID_PREFIX, mac[4], mac[5]);

    WiFi.softAP(apSsid, DEFAULT_AP_PASSWORD);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    LOG_INFO("AP started: SSID='%s'  IP=%s  Pass='%s'",
             apSsid, apIP.toString().c_str(), DEFAULT_AP_PASSWORD);

    _portal = new CaptivePortal(_prefs);
    _portal->begin(apIP, [this]() {
        delay(1500);
        ESP.restart();
    });
}

void WiFiManager::_onConnected() {
    _reconnectAttempts = 0;
    _state = State::CONNECTED;

    if (_portal) {
        _portal->stop();
        delete _portal;
        _portal = nullptr;
    }

    LOG_INFO("WiFi connected: IP=%s  SSID=%s  BSSID=%s  RSSI=%d dBm",
             WiFi.localIP().toString().c_str(),
             WiFi.SSID().c_str(),
             WiFi.BSSIDstr().c_str(),
             (int)WiFi.RSSI());
}

void WiFiManager::_onDisconnected(uint8_t reason) {
    _lastDisconnectReason = reason;
    LOG_WARN("WiFi disconnected (reason %u %s)", reason, _reasonName(reason));

    if (_state == State::CONNECTED) {
        _reconnectAttempts++;
        _state       = State::CONNECTING;
        _actionSince = millis();
        // Do not call WiFi.begin() here. setAutoReconnect(true) already retries;
        // a second begin() races the supplicant and makes the link look flaky.
    }
}

const char* WiFiManager::_reasonName(uint8_t reason) {
    switch (reason) {
        case 1:   return "UNSPECIFIED";
        case 2:   return "AUTH_EXPIRE";
        case 3:   return "AUTH_LEAVE";
        case 4:   return "ASSOC_EXPIRE";
        case 8:   return "ASSOC_LEAVE";
        case 15:  return "4WAY_HANDSHAKE_TIMEOUT";
        case 200: return "BEACON_TIMEOUT";
        case 201: return "NO_AP_FOUND";
        case 202: return "AUTH_FAIL";
        case 203: return "ASSOC_FAIL";
        case 204: return "HANDSHAKE_TIMEOUT";
        case 205: return "CONNECTION_FAIL";
        default:  return "OTHER";
    }
}

const char* WiFiManager::getLastDisconnectReasonName() const {
    return _reasonName(_lastDisconnectReason);
}

void WiFiManager::_wifiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!_instance) return;
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _instance->_onConnected();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            _instance->_onDisconnected(info.wifi_sta_disconnected.reason);
            break;
        default:
            break;
    }
}

// ── Accessors ─────────────────────────────────────────────────────────────────

String WiFiManager::getIP() const {
    if (_state == State::CONNECTED) return WiFi.localIP().toString();
    if (_state == State::AP_MODE)   return WiFi.softAPIP().toString();
    return "0.0.0.0";
}

String WiFiManager::getSSID() const {
    if (_state == State::CONNECTED) return WiFi.SSID();
    return _prefs.getWifiSsid();
}

String WiFiManager::getBSSID() const {
    if (_state == State::CONNECTED) return WiFi.BSSIDstr();
    return "";
}

int32_t WiFiManager::getRSSI() const {
    if (_state == State::CONNECTED) return WiFi.RSSI();
    return 0;
}

void WiFiManager::onCredentialsSaved() {
    delay(1500);
    ESP.restart();
}

void WiFiManager::clearCredentialsAndReboot() {
    _prefs.clearWifiCredentials();
    _rebootAt = millis() + 1500;
    LOG_WARN("WiFi credentials cleared – rebooting into AP mode");
}
