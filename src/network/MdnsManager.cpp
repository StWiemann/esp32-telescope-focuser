#include "MdnsManager.h"
#include <ESPmDNS.h>
#include "util/Log.h"

void MdnsManager::begin(const String& hostname) {
    if (!MDNS.begin(hostname.c_str())) {
        LOG_ERROR("mDNS failed to start for hostname '%s'", hostname.c_str());
        return;
    }

    // Announce HTTP service for browsers
    MDNS.addService("http", "tcp", 80);

    // Announce Alpaca service so ASCOM discovery tools can find it
    // ASCOM Alpaca uses UDP discovery on port 32227, but advertising via mDNS
    // also helps manual discovery in N.I.N.A.
    MDNS.addService("alpaca", "tcp", 80);

    _started = true;
    LOG_INFO("mDNS started: http://%s.local/", hostname.c_str());
}

void MdnsManager::loop() {
    // ESPmDNS on Arduino/ESP32 is fully async; no explicit loop call required.
}
