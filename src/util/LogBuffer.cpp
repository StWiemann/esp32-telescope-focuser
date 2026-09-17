#include "util/LogBuffer.h"
#include "Config.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

static char          s_buf[LOG_RING_SIZE];
static size_t        s_start = 0;
static size_t        s_used  = 0;
static portMUX_TYPE  s_mux   = portMUX_INITIALIZER_UNLOCKED;

static void dropOldestLine() {
    if (s_used == 0) return;
    size_t i = 0;
    while (i < s_used) {
        char c = s_buf[(s_start + i) % LOG_RING_SIZE];
        i++;
        if (c == '\n') break;
    }
    s_start = (s_start + i) % LOG_RING_SIZE;
    s_used -= i;
}

static void appendBytes(const char* data, size_t n) {
    if (n >= LOG_RING_SIZE) {
        data += n - (LOG_RING_SIZE - 1);
        n = LOG_RING_SIZE - 1;
    }
    while (s_used + n > LOG_RING_SIZE) {
        dropOldestLine();
    }
    for (size_t i = 0; i < n; i++) {
        s_buf[(s_start + s_used) % LOG_RING_SIZE] = data[i];
        s_used++;
    }
}

void logBufferBegin() {
    s_start = 0;
    s_used  = 0;
}

void logCapture(const char* level, const char* fmt, ...) {
    char line[192];
    int prefix = snprintf(line, sizeof(line), "[%lu] [%s]  ",
                          (unsigned long)millis(), level);
    if (prefix < 0) prefix = 0;
    if (prefix >= (int)sizeof(line)) prefix = sizeof(line) - 1;

    va_list args;
    va_start(args, fmt);
    int body = vsnprintf(line + prefix, sizeof(line) - prefix, fmt, args);
    va_end(args);
    if (body < 0) body = 0;

    size_t total = (size_t)prefix + (size_t)body;
    if (total >= sizeof(line)) total = sizeof(line) - 1;
    if (total + 1 < sizeof(line)) {
        line[total++] = '\n';
        line[total] = '\0';
    } else {
        line[sizeof(line) - 2] = '\n';
        line[sizeof(line) - 1] = '\0';
        total = sizeof(line) - 1;
    }

    portENTER_CRITICAL(&s_mux);
    appendBytes(line, total);
    portEXIT_CRITICAL(&s_mux);
}

void logBufferClear() {
    portENTER_CRITICAL(&s_mux);
    s_start = 0;
    s_used  = 0;
    portEXIT_CRITICAL(&s_mux);
}

String logBufferSnapshot() {
    char* tmp = (char*)malloc(LOG_RING_SIZE + 1);
    if (!tmp) return String();

    portENTER_CRITICAL(&s_mux);
    const size_t n = s_used;
    for (size_t i = 0; i < n; i++) {
        tmp[i] = s_buf[(s_start + i) % LOG_RING_SIZE];
    }
    portEXIT_CRITICAL(&s_mux);
    tmp[n] = '\0';

    String out(tmp);
    free(tmp);
    return out;
}

size_t logBufferUsed() {
    portENTER_CRITICAL(&s_mux);
    size_t n = s_used;
    portEXIT_CRITICAL(&s_mux);
    return n;
}
