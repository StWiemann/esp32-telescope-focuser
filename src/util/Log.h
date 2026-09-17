#pragma once

/**
 * Log.h – Thin wrapper around SLog (provided by ESP32AlpacaDevices2 dependency).
 *
 * SLog outputs to Serial and optionally to a syslog server.  We expose
 * simple macros so the rest of the code never calls SLOG_PRINTF directly,
 * giving us a single place to change the logging backend if needed.
 *
 * Usage:
 *   LOG_INFO("WiFi connected: %s", ip.c_str());
 *   LOG_WARN("WLAN disconnected");
 *   LOG_ERROR("Move rejected: position %d out of range", pos);
 *   LOG_DEBUG("Stepper tick, target=%d", target);
 */

#include <SLog.h>

// Severity-tagged macros matching the log output format expected in the spec
#define LOG_INFO(fmt, ...)   SLOG_PRINTF(SLOG_INFO,    "[INFO]  " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   SLOG_PRINTF(SLOG_WARNING, "[WARN]  " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)  SLOG_PRINTF(SLOG_ERROR,   "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  SLOG_PRINTF(SLOG_DEBUG,   "[DEBUG] " fmt "\n", ##__VA_ARGS__)
