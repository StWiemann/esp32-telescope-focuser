#include "DisplayManager.h"
#include "focuser/FocuserController.h"
#include "util/Log.h"
#include <cmath>

// ── Compile-time display driver selection ────────────────────────────────────
// U8g2 full-buffer mode (F_HW_I2C) fits 128×64 = 1 KB – fine for ESP32.
// Using HW I2C with the default ESP32 pins (SDA=21, SCL=22).

#ifdef OLED_SH1106
  // SH1106 – common on some "1.3 inch" OLED modules
  static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
      U8G2_R0, U8X8_PIN_NONE, PIN_OLED_SCL, PIN_OLED_SDA);
#else
  // SSD1306 – default; SBC-OLED01 uses SSD1306 or compatible
  static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
      U8G2_R0, U8X8_PIN_NONE, PIN_OLED_SCL, PIN_OLED_SDA);
#endif

void DisplayManager::begin() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontMode(0);        // opaque background
    u8g2.setDrawColor(1);
    _initialized = true;
    LOG_INFO("Display initialized (I2C addr 0x%02X)", OLED_I2C_ADDR);
}

void DisplayManager::showBootScreen() {
    if (!_initialized) return;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tf);
    u8g2.drawStr(16, 22, "ESP32 FOCUSER");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(28, 38, "Booting...");
    u8g2.drawStr(20, 52, "v" FIRMWARE_VERSION);
    u8g2.sendBuffer();
    _lastRefreshMs = millis();
}

void DisplayManager::loop(const FocuserController& focuser, bool wifiOk, bool alpacaOk,
                          float tempCelsius) {
    if (!_initialized) return;

    uint32_t now = millis();
    uint32_t interval = focuser.isMoving()
                        ? DISPLAY_REFRESH_MOVING_MS
                        : DISPLAY_REFRESH_IDLE_MS;

    if (now - _lastRefreshMs >= interval) {
        _draw(focuser, wifiOk, alpacaOk, tempCelsius);
        _lastRefreshMs = now;
    }
}

void DisplayManager::_draw(const FocuserController& focuser, bool wifiOk, bool alpacaOk,
                            float tempCelsius) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    char buf[32];

    // ── Row 0 (y= 9): header ──────────────────────────────────────────────
    u8g2.drawStr(0, 9, "ESP32 FOCUSER");

    // ── Row 1 (y=19): Position ────────────────────────────────────────────
    u8g2.drawStr(0, 19, "Pos:");
    if (focuser.isZeroed()) {
        snprintf(buf, sizeof(buf), "%-8ld", (long)focuser.getCurrentPosition());
    } else {
        snprintf(buf, sizeof(buf), "----    ");
    }
    u8g2.drawStr(26, 19, buf);

    // ── Row 2 (y=29): Target ──────────────────────────────────────────────
    u8g2.drawStr(0, 29, "Tgt:");
    if (focuser.isZeroed()) {
        snprintf(buf, sizeof(buf), "%-8ld", (long)focuser.getTargetPosition());
    } else {
        snprintf(buf, sizeof(buf), "----    ");
    }
    u8g2.drawStr(26, 29, buf);

    // ── Row 3 (y=39): Status / movement indicator ─────────────────────────
    if (focuser.isMoving()) {
        int32_t cur = focuser.getCurrentPosition();
        int32_t tgt = focuser.getTargetPosition();
        u8g2.drawStr(0, 39, cur < tgt ? "MOVING >>>" : "MOVING <<<");
    } else if (!focuser.isZeroed()) {
        u8g2.drawStr(0, 39, "STATUS: UNZEROED");
    } else {
        u8g2.drawStr(0, 39, "STATUS: READY   ");
    }

    // ── Row 4 (y=50): Temperature ─────────────────────────────────────────
    if (!std::isnan(tempCelsius)) {
        snprintf(buf, sizeof(buf), "Tmp: %+.1f" "\xb0" "C", tempCelsius);
    } else {
        snprintf(buf, sizeof(buf), "Tmp: ---");
    }
    u8g2.drawStr(0, 50, buf);

    // ── Row 5 (y=62): WiFi + Alpaca ───────────────────────────────────────
    snprintf(buf, sizeof(buf), "WiFi:%-4s Alp:%-3s",
             wifiOk ? "OK" : "LOST",
             alpacaOk ? "OK" : "OFF");
    u8g2.drawStr(0, 62, buf);

    u8g2.sendBuffer();
}
