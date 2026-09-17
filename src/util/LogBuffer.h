#pragma once
#include <Arduino.h>

/**
 * Fixed-size RAM ring of recent LOG_* lines for the web Debug tab.
 * Oldest lines are dropped when full. Not persisted across reboot.
 */
void   logBufferBegin();
void   logCapture(const char* level, const char* fmt, ...);
void   logBufferClear();
String logBufferSnapshot();
size_t logBufferUsed();
