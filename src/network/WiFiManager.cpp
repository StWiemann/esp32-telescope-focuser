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
    uint32_t now = millis();

    switch (_state) {
        case State::CONNECTING:
            if (now - _actionSince >= WIFI_CONNECT_TIMEOUT_MS) {
                if (_reconnectAttempts == 0) {
                    // First attempt failed – fall back to AP mode
                    LOG_WARN("WiFi initial connect timed out – starting AP for reconfiguration");
                    WiFi.disconnect(true);
                    _startAP();
                } else {
                    // Subsequent attempt failed – retry silently after interval
                    LOG_WARN("WiFi reconnect timed out – will retry");
                    _startStation();
                }
            }
            break;

        case State::CONNECTED:
            // WiFi is up; periodic health check via event callback
            break;

        case State::AP_MODE:
            if (_portal) _portal->loop();
            break;

        default: break;
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

void WiFiManager::_startStation() {
    _state         = State::CONNECTING;
    _actionSince   = millis();

    WiFi.mode(WIFI_STA);
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

    LOG_INFO("WiFi connected: IP=%s  SSID=%s  RSSI=%d dBm",
             WiFi.localIP().toString().c_str(),
             WiFi.SSID().c_str(),
             (int)WiFi.RSSI());
}

void WiFiManager::_onDisconnected() {
    if (_state == State::CONNECTED) {
        _reconnectAttempts++;
        LOG_WARN("WiFi disconnected (attempt %d) – reconnecting …", _reconnectAttempts);
        _startStation();
    }
}

void WiFiManager::_wifiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!_instance) return;
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _instance->_onConnected();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            _instance->_onDisconnected();
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
