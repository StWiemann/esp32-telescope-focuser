#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "storage/PreferencesManager.h"

/**
 * CaptivePortal – AP-mode WiFi configuration page.
 *
 * Starts a DNS server that resolves all domains to the ESP32 AP IP,
 * and an HTTP server on port 80 serving a minimal config form.
 *
 * The config form presents:
 *   - Available networks (scan result)
 *   - SSID input field
 *   - Password input field
 *
 * On submit the credentials are saved to NVS and the device restarts.
 */
class CaptivePortal {
public:
    using OnSavedCallback = std::function<void()>;

    explicit CaptivePortal(PreferencesManager& prefs);

    void begin(const IPAddress& apIP, OnSavedCallback cb);
    void loop();
    void stop();

private:
    PreferencesManager& _prefs;
    DNSServer           _dns;
    AsyncWebServer*     _server  = nullptr;
    OnSavedCallback     _onSaved;
    bool                _running = false;

    void _registerRoutes();
    static String _buildPage(const String& scanJson);
    static String _scanNetworks();
};
