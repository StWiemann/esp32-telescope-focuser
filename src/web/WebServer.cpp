#include "WebServer.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "util/Log.h"

void WebServer::begin(AsyncWebServer* server,
                      FocuserController& focuser,
                      PreferencesManager& prefs,
                      WiFiManager& wifi,
                      TemperatureSensor& tempSensor) {
    _focuser    = &focuser;
    _prefs      = &prefs;
    _wifi       = &wifi;
    _tempSensor = &tempSensor;
    _registerRoutes(server);
    LOG_INFO("Custom web routes registered");
}

void WebServer::_registerRoutes(AsyncWebServer* server) {

    // ── Static files: serve all /www/* directly from LittleFS ─────────────
    // AlpacaServer does not add a generic static file handler; we add one here
    // for our web UI assets (style.css, app.js, …).
    server->serveStatic("/www/", LittleFS, "/www/");

    // ── Status page (root) ─────────────────────────────────────────────────
    server->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (LittleFS.exists("/www/index.html")) {
            req->send(LittleFS, "/www/index.html", "text/html");
        } else {
            req->send(200, "text/plain",
                      "ESP32 Focuser running. LittleFS not flashed yet.");
        }
    });

    // ── API: status ────────────────────────────────────────────────────────
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "application/json", _buildStatusJson());
    });

    // ── API: halt ──────────────────────────────────────────────────────────
    server->on("/api/halt", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _focuser->enqueueHalt();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── API: zero ──────────────────────────────────────────────────────────
    server->on("/api/zero", HTTP_POST, [this](AsyncWebServerRequest* req) {
        if (_focuser->isMoving()) {
            req->send(409, "application/json",
                      "{\"ok\":false,\"error\":\"Motor is moving\"}");
            return;
        }
        _focuser->enqueueZero();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── API: move (absolute, JSON body) ───────────────────────────────────
    AsyncCallbackJsonWebHandler* moveHandler =
        new AsyncCallbackJsonWebHandler("/api/move",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!json.is<JsonObject>() || !json["position"].is<int>()) {
                    req->send(400, "application/json",
                              "{\"ok\":false,\"error\":\"Missing position\"}");
                    return;
                }
                int32_t pos = json["position"].as<int32_t>();
                if (pos < 0 || pos > _focuser->getMaxPosition()) {
                    req->send(400, "application/json",
                              "{\"ok\":false,\"error\":\"Position out of range\"}");
                    return;
                }
                _focuser->enqueueMove(pos);
                req->send(200, "application/json", "{\"ok\":true}");
            });
    server->addHandler(moveHandler);

    // ── API: moveby (relative, JSON body) ─────────────────────────────────
    AsyncCallbackJsonWebHandler* moveByHandler =
        new AsyncCallbackJsonWebHandler("/api/moveby",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!json.is<JsonObject>() || !json["delta"].is<int>()) {
                    req->send(400, "application/json",
                              "{\"ok\":false,\"error\":\"Missing delta\"}");
                    return;
                }
                int32_t delta = json["delta"].as<int32_t>();
                _focuser->moveRelative(delta);
                req->send(200, "application/json", "{\"ok\":true}");
            });
    server->addHandler(moveByHandler);

    // ── API: get config ────────────────────────────────────────────────────
    server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "application/json", _buildConfigJson());
    });

    // ── API: set config (JSON body) ────────────────────────────────────────
    AsyncCallbackJsonWebHandler* configHandler =
        new AsyncCallbackJsonWebHandler("/api/config",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!json.is<JsonObject>()) {
                    req->send(400, "application/json",
                              "{\"ok\":false,\"error\":\"Expected JSON object\"}");
                    return;
                }
                JsonObject obj = json.as<JsonObject>();

                if (obj["hostname"].is<const char*>())
                    _prefs->setHostname(obj["hostname"].as<const char*>());
                if (obj["focuserName"].is<const char*>())
                    _prefs->setFocuserName(obj["focuserName"].as<const char*>());
                if (obj["maxPosition"].is<int>())
                    _prefs->setMaxPosition(obj["maxPosition"].as<int32_t>());
                if (obj["backlashSteps"].is<int>())
                    _prefs->setBacklashSteps(obj["backlashSteps"].as<int32_t>());
                if (obj["backlashApproachDir"].is<int>())
                    _prefs->setBacklashApproachDir((int8_t)obj["backlashApproachDir"].as<int>());
                if (obj["motorReversed"].is<bool>())
                    _prefs->setMotorReversed(obj["motorReversed"].as<bool>());
                if (obj["stepsPerRev"].is<int>())
                    _prefs->setStepsPerRev(obj["stepsPerRev"].as<int32_t>());
                if (obj["motorPulleyTeeth"].is<int>())
                    _prefs->setMotorPulleyTeeth(obj["motorPulleyTeeth"].as<int32_t>());
                if (obj["focuserPulleyTeeth"].is<int>())
                    _prefs->setFocuserPulleyTeeth(obj["focuserPulleyTeeth"].as<int32_t>());
                if (obj["manualStepSize"].is<int>())
                    _prefs->setManualStepSize(obj["manualStepSize"].as<int32_t>());
                if (obj["stepMedium"].is<int>())
                    _prefs->setStepMedium(obj["stepMedium"].as<int32_t>());
                if (obj["stepLarge"].is<int>())
                    _prefs->setStepLarge(obj["stepLarge"].as<int32_t>());
                if (obj["manualSpeed"].is<float>())
                    _prefs->setManualSpeed(obj["manualSpeed"].as<float>());
                if (obj["motorMaxSpeed"].is<float>())
                    _prefs->setMotorMaxSpeed(obj["motorMaxSpeed"].as<float>());

                _focuser->applyConfig();

                LOG_INFO("Config updated via web API");
                req->send(200, "application/json", "{\"ok\":true}");
            });
    server->addHandler(configHandler);
}

String WebServer::_buildStatusJson() const {
    JsonDocument doc;

    doc["position"]     = _focuser->getCurrentPosition();
    doc["target"]       = _focuser->getTargetPosition();
    doc["isMoving"]     = _focuser->isMoving();
    doc["isZeroed"]     = _focuser->isZeroed();
    doc["maxPosition"]  = _focuser->getMaxPosition();
    doc["firmware"]     = FIRMWARE_VERSION;

    float t = _tempSensor->getTemperatureCelsius();
    if (!isnan(t)) {
        doc["temperature"] = t;
    } else {
        doc["temperature"] = nullptr;  // JSON null when sensor absent/not-read-yet
    }
    doc["tempAvailable"] = _tempSensor->isAvailable();

    doc["wifi"]["ok"]   = _wifi->isConnected();
    doc["wifi"]["ssid"] = _wifi->getSSID();
    doc["wifi"]["ip"]   = _wifi->getIP();
    doc["wifi"]["rssi"] = _wifi->getRSSI();

    String out;
    serializeJson(doc, out);
    return out;
}

String WebServer::_buildConfigJson() const {
    JsonDocument doc;

    doc["hostname"]           = _prefs->getHostname();
    doc["focuserName"]        = _prefs->getFocuserName();
    doc["maxPosition"]        = _prefs->getMaxPosition();
    doc["backlashSteps"]      = _prefs->getBacklashSteps();
    doc["backlashApproachDir"]= _prefs->getBacklashApproachDir();
    doc["motorReversed"]      = _prefs->getMotorReversed();
    doc["stepsPerRev"]        = _prefs->getStepsPerRev();
    doc["motorPulleyTeeth"]   = _prefs->getMotorPulleyTeeth();
    doc["focuserPulleyTeeth"] = _prefs->getFocuserPulleyTeeth();
    doc["beltRatio"]          = _prefs->getBeltRatio();
    doc["manualStepSize"]     = _prefs->getManualStepSize();
    doc["stepMedium"]         = _prefs->getStepMedium();
    doc["stepLarge"]          = _prefs->getStepLarge();
    doc["manualSpeed"]        = _prefs->getManualSpeed();
    doc["motorMaxSpeed"]      = _prefs->getMotorMaxSpeed();

    String out;
    serializeJson(doc, out);
    return out;
}
