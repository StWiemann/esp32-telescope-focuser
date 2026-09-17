#pragma once
#include <Arduino.h>

/**
 * MdnsManager – registers the ESP32 under a configurable .local hostname
 * and announces the Alpaca HTTP service for ASCOM auto-discovery.
 */
class MdnsManager {
public:
    void begin(const String& hostname);
    void loop();   // currently a no-op; reserved for future MDNS updates

private:
    bool _started = false;
};
