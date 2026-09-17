/**
 * ESP32 Astro Focuser – main entry point
 *
 * Startup sequence
 * ────────────────
 * 1. Serial + SLog init
 * 2. Preferences (NVS settings)
 * 3. Display (OLED boot screen)
 * 4. Buttons
 * 5. Stepper + FocuserController
 * 6. WiFi (station or AP + captive portal)
 * 7. Once WiFi is connected:
 *    a. mDNS
 *    b. AlpacaServer (HTTP port 80, LittleFS, UDP discovery)
 *    c. Custom web routes (/api/status, /api/move, …)
 * 8. Main loop: buttons, focuser, display, alpaca
 *
 * The firmware works without WiFi (buttons + OLED) but NINA/Astroberry
 * connectivity requires a successful station connection.
 */

#include <Arduino.h>
#include <SLog.h>
#include <AlpacaServer.h>

#include "util/Log.h"
#include "storage/PreferencesManager.h"
#include "display/DisplayManager.h"
#include "input/ButtonManager.h"
#include "focuser/StepperController.h"
#include "focuser/FocuserController.h"
#include "sensors/TemperatureSensor.h"
#include "network/WiFiManager.h"
#include "network/MdnsManager.h"
#include "web/WebServer.h"
#include "alpaca/MyAlpacaFocuser.h"
#include "Config.h"

// ── Module instances ──────────────────────────────────────────────────────────

static PreferencesManager g_prefs;
static DisplayManager     g_display;
static ButtonManager      g_buttons;
static StepperController  g_stepper;
static FocuserController  g_focuser(g_stepper, g_prefs);
static TemperatureSensor  g_tempSensor;
static WiFiManager        g_wifi(g_prefs);
static MdnsManager        g_mdns;

// Network services are initialized lazily (after WiFi connects)
static AlpacaServer*    g_alpaca = nullptr;
static MyAlpacaFocuser* g_device = nullptr;
static WebServer*       g_webSrv = nullptr;

static bool g_networkReady = false;

// ── Forward declarations ──────────────────────────────────────────────────────
static void onNetworkReady();

// ── setup() ───────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);  // allow USB serial to enumerate

    logBufferBegin();

    // SLog is provided by ESP32AlpacaDevices2; output to Serial at DEBUG level
    g_Slog.Begin(Serial, 115200);
    g_Slog.SetLvlMsk(SLOG_DEBUG);
    g_Slog.SetEnableSerial(true);

    LOG_INFO("=== ESP32 Astro Focuser v%s ===", FIRMWARE_VERSION);
    LOG_INFO("Chip: %s  Rev: %d  Freq: %u MHz",
             ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
    LOG_INFO("Free heap: %u bytes", ESP.getFreeHeap());

    // ── Hardware init ─────────────────────────────────────────────────────
    g_prefs.begin();
    g_display.begin();
    g_display.showBootScreen();
    g_buttons.begin();
    g_focuser.begin();
    g_tempSensor.begin();

    LOG_INFO("Hardware initialized");

    // ── Network ───────────────────────────────────────────────────────────
    g_wifi.begin();

    // If WiFi is already connected synchronously (unlikely but possible on
    // reconnect scenarios), handle immediately.
    if (g_wifi.isConnected() && !g_networkReady) {
        onNetworkReady();
    }
}

// ── loop() ────────────────────────────────────────────────────────────────────

void loop() {
    // ── WiFi state machine ────────────────────────────────────────────────
    g_wifi.loop();

    // Detect transition to connected state
    if (!g_networkReady && g_wifi.isConnected()) {
        onNetworkReady();
    }

    // ── Physical input ────────────────────────────────────────────────────
    g_buttons.loop(g_focuser);

    // ── Focuser logic (command dispatch, backlash state machine) ─────────
    g_focuser.loop();

    // ── Temperature sensor (non-blocking poll) ────────────────────────────
    g_tempSensor.loop();

    // ── Alpaca server maintenance ─────────────────────────────────────────
    if (g_networkReady && g_alpaca != nullptr) {
        // AlpacaServer::Loop() calls CheckClientConnectionTimeout() on all
        // registered devices and handles ElegantOTA updates.
        g_alpaca->Loop();

        // Handle reset request from Alpaca setup page
        if (g_alpaca->GetResetRequest()) {
            LOG_WARN("Reset requested via Alpaca setup page");
            delay(500);
            ESP.restart();
        }
    }

    // ── Display refresh ───────────────────────────────────────────────────
    bool alpacaOk = (g_networkReady && g_alpaca != nullptr);
    g_display.loop(g_focuser, g_wifi.isConnected(), alpacaOk,
                   g_tempSensor.getTemperatureCelsius());

    // Main loop yield – 10 ms gives FreeRTOS time to run other tasks.
    // ESPAsyncWebServer is async so it does not need loop() calls.
    delay(10);
}

// ── Called once WiFi station connection is established ───────────────────────

static void onNetworkReady() {
    g_networkReady = true;

    LOG_INFO("Network ready – IP=%s", g_wifi.getIP().c_str());

    // mDNS
    g_mdns.begin(g_prefs.getHostname());

    // AlpacaServer: constructor takes server name, manufacturer, version, location
    g_alpaca = new AlpacaServer(
        g_prefs.getFocuserName(),
        "DIY",
        FIRMWARE_VERSION,
        "Observatory"
    );

    // Step 1: Begin() – mounts LittleFS, creates AsyncWebServer, starts UDP discovery
    g_alpaca->Begin();   // default ports: UDP 32227, TCP 80

    // Step 2: Create focuser device
    g_device = new MyAlpacaFocuser(g_focuser, g_prefs, g_tempSensor);

    // Step 3: AddDevice() – calls SetAlpacaServer(), SetDeviceNumber(),
    //         and RegisterCallbacks() on the device.  _alpaca_server is now set.
    g_alpaca->AddDevice(g_device);

    // Step 4: Begin() is now safe (requires _alpaca_server set by AddDevice)
    g_device->Begin();

    // Step 5: Register our custom web routes and static file handler.
    //         Must happen before AlpacaServer::RegisterCallbacks() adds the
    //         /settings.json static route so our routes take priority.
    g_webSrv = new WebServer();
    g_webSrv->begin(g_alpaca->getServerTCP(), g_focuser, g_prefs, g_wifi, g_tempSensor);

    // Step 6: Register server-level Alpaca management routes
    g_alpaca->RegisterCallbacks();

    // Step 7: Load persisted AlpacaDevice settings from /settings.json in LittleFS
    g_alpaca->LoadSettings();

    g_display.forceRefresh();

    LOG_INFO("Alpaca server running on http://%s.local/ (port 80)",
             g_prefs.getHostname().c_str());
    LOG_INFO("Setup page: http://%s.local/setup/v1/focuser/0/setup",
             g_prefs.getHostname().c_str());
}
