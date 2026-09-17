#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "focuser/FocuserController.h"
#include "storage/PreferencesManager.h"
#include "network/WiFiManager.h"
#include "sensors/TemperatureSensor.h"

/**
 * WebServer – custom HTTP routes added to the AlpacaServer's AsyncWebServer.
 *
 * Routes registered
 * -----------------
 *  GET  /           → serves /www/index.html from LittleFS
 *  GET  /api/status → JSON status snapshot
 *  POST /api/halt   → stop motor
 *  POST /api/zero   → set zero
 *  POST /api/move   → { "position": <int> }   absolute move
 *  POST /api/moveby → { "delta": <int> }       relative move
 *  GET  /api/config → JSON of current config
 *  POST /api/config → update config fields (JSON body)
 *
 * All POST endpoints accept application/json.
 * The AsyncWebServer used here is the one owned by AlpacaServer so that all
 * HTTP traffic goes through a single server on port 80.
 */
class WebServer {
public:
    void begin(AsyncWebServer* server,
               FocuserController& focuser,
               PreferencesManager& prefs,
               WiFiManager& wifi,
               TemperatureSensor& tempSensor);

private:
    FocuserController* _focuser    = nullptr;
    PreferencesManager* _prefs     = nullptr;
    WiFiManager*        _wifi      = nullptr;
    TemperatureSensor*  _tempSensor = nullptr;

    void _registerRoutes(AsyncWebServer* server);
    String _buildStatusJson()  const;
    String _buildConfigJson()  const;
};
