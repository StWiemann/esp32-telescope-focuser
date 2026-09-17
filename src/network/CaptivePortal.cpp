#include "CaptivePortal.h"
#include <WiFi.h>
#include "util/Log.h"

// All captive portal HTML/JS is inline to avoid LittleFS dependency in AP mode.
// Intentionally minimal: no frameworks, ~3 KB total.

static const char PORTAL_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Focuser – WiFi Setup</title>
<style>
body{font-family:sans-serif;background:#1a1a2e;color:#eee;display:flex;
  justify-content:center;align-items:center;min-height:100vh;margin:0}
.card{background:#16213e;padding:2rem;border-radius:12px;max-width:380px;width:90%}
h1{margin:0 0 1.5rem;font-size:1.3rem;color:#e94560}
label{display:block;margin:.8rem 0 .2rem;font-size:.85rem;color:#aaa}
input,select{width:100%;box-sizing:border-box;padding:.6rem;border:1px solid #444;
  background:#0f3460;color:#eee;border-radius:6px;font-size:1rem}
button{width:100%;margin-top:1.5rem;padding:.8rem;background:#e94560;color:#fff;
  border:none;border-radius:6px;font-size:1rem;cursor:pointer}
button:hover{background:#c73652}
#status{margin-top:1rem;font-size:.85rem;color:#aaa;text-align:center;min-height:1.2em}
</style>
</head>
<body>
<div class="card">
  <h1>&#9899; ESP32 Focuser<br>WiFi-Konfiguration</h1>
  <label for="ssid">WLAN-Netzwerk (SSID)</label>
  <select id="ssid" name="ssid" onchange="
    var custom=document.getElementById('ssid_custom');
    custom.style.display=(this.value==='__manual__')?'block':'none';
    if(this.value!=='__manual__')document.getElementById('ssid_input').value=this.value;
  ">
    <option value="">-- Lade Netzwerke... --</option>
    <option value="__manual__">Manuell eingeben…</option>
  </select>
  <input id="ssid_custom" name="ssid_custom" type="text" placeholder="SSID manuell" style="display:none;margin-top:.4rem">
  <input id="ssid_input" type="hidden">
  <label for="pass">Passwort</label>
  <input id="pass" type="password" placeholder="WLAN-Passwort">
  <button onclick="doSave()">Speichern &amp; Verbinden</button>
  <div id="status"></div>
</div>
<script>
function status(msg){document.getElementById('status').textContent=msg;}
function doSave(){
  var sel=document.getElementById('ssid');
  var ssid=(sel.value==='__manual__')
    ?document.getElementById('ssid_custom').value.trim()
    :document.getElementById('ssid_input').value||sel.value;
  var pass=document.getElementById('pass').value;
  if(!ssid){status('Bitte SSID eingeben.');return;}
  status('Wird gespeichert…');
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)})
  .then(r=>r.text()).then(t=>status(t))
  .catch(()=>status('Fehler beim Speichern.'));
}
fetch('/scan').then(r=>r.json()).then(nets=>{
  var sel=document.getElementById('ssid');
  sel.innerHTML='';
  nets.forEach(n=>{
    var o=document.createElement('option');
    o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+' dBm)';
    sel.appendChild(o);
  });
  var manual=document.createElement('option');
  manual.value='__manual__';manual.textContent='Manuell eingeben…';
  sel.appendChild(manual);
  if(nets.length>0)document.getElementById('ssid_input').value=nets[0].ssid;
}).catch(()=>{
  document.getElementById('ssid').innerHTML='<option value="__manual__">Manuell eingeben…</option>';
});
</script>
</body>
</html>
)rawhtml";

CaptivePortal::CaptivePortal(PreferencesManager& prefs) : _prefs(prefs) {}

void CaptivePortal::begin(const IPAddress& apIP, OnSavedCallback cb) {
    _onSaved = cb;
    _server  = new AsyncWebServer(80);
    _registerRoutes();
    _server->begin();

    _dns.start(53, "*", apIP);
    _running = true;

    LOG_INFO("Captive portal started (HTTP port 80, DNS redirecting all to %s)",
             apIP.toString().c_str());
}

void CaptivePortal::loop() {
    if (_running) _dns.processNextRequest();
}

void CaptivePortal::stop() {
    if (!_running) return;
    _dns.stop();
    _server->end();
    _running = false;
    LOG_INFO("Captive portal stopped");
}

void CaptivePortal::_registerRoutes() {
    // Serve portal page on every URL (captive-portal catch-all)
    _server->onNotFound([](AsyncWebServerRequest* req) {
        // Captive-portal redirect: any unknown host → portal root
        req->send_P(200, "text/html", PORTAL_HTML);
    });

    _server->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", PORTAL_HTML);
    });

    _server->on("/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", CaptivePortal::_scanNetworks());
    });

    _server->on("/save", HTTP_POST, [this](AsyncWebServerRequest* req) {
        if (!req->hasParam("ssid", true) || !req->hasParam("pass", true)) {
            req->send(400, "text/plain", "Fehlende Parameter");
            return;
        }
        String ssid = req->getParam("ssid", true)->value();
        String pass = req->getParam("pass", true)->value();

        if (ssid.isEmpty()) {
            req->send(400, "text/plain", "SSID darf nicht leer sein.");
            return;
        }

        _prefs.setWifiCredentials(ssid, pass);
        req->send(200, "text/plain",
                  "Gespeichert! ESP32 verbindet sich mit \"" + ssid +
                  "\" und startet neu...");

        LOG_INFO("Portal: credentials saved for SSID '%s', restarting", ssid.c_str());

        if (_onSaved) _onSaved();
    });
}

String CaptivePortal::_scanNetworks() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"";
        // Escape quotes in SSID
        String s = WiFi.SSID(i);
        s.replace("\"", "\\\"");
        json += s;
        json += "\",\"rssi\":";
        json += WiFi.RSSI(i);
        json += "}";
    }
    json += "]";
    WiFi.scanDelete();
    return json;
}
