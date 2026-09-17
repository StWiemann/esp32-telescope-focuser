#pragma once

/**
 * Log.h – Thin wrapper around SLog (provided by ESP32AlpacaDevices2 dependency).
 *
 * Each line is written to Serial (via SLog) and to a small RAM ring
 * (LogBuffer) so the web Debug tab can show the same history without USB.
 *
 * Usage:
 *   LOG_INFO("WiFi connected: %s", ip.c_str());
 *   LOG_WARN("WLAN disconnected");
 *   LOG_ERROR("Move rejected: position %d out of range", pos);
 *   LOG_DEBUG("Stepper tick, target=%d", target);
 */

#include <SLog.h>
#include "util/LogBuffer.h"

#define LOG_INFO(fmt, ...)  do { \
        logCapture("INFO",  fmt, ##__VA_ARGS__); \
        SLOG_PRINTF(SLOG_INFO,    "[INFO]  " fmt "\n", ##__VA_ARGS__); \
    } while (0)
#define LOG_WARN(fmt, ...)  do { \
        logCapture("WARN",  fmt, ##__VA_ARGS__); \
        SLOG_PRINTF(SLOG_WARNING, "[WARN]  " fmt "\n", ##__VA_ARGS__); \
    } while (0)
#define LOG_ERROR(fmt, ...) do { \
        logCapture("ERROR", fmt, ##__VA_ARGS__); \
        SLOG_PRINTF(SLOG_ERROR,   "[ERROR] " fmt "\n", ##__VA_ARGS__); \
    } while (0)
#define LOG_DEBUG(fmt, ...) do { \
        logCapture("DEBUG", fmt, ##__VA_ARGS__); \
        SLOG_PRINTF(SLOG_DEBUG,   "[DEBUG] " fmt "\n", ##__VA_ARGS__); \
    } while (0)
