#pragma once
// ============================================================
// engine/RGBEngine.h — Reacteur d'ambiance RGB LED
// Inclus APRES LovyanGFX pour eviter les conflits de noms
// ============================================================
#include <Arduino.h>
#include "config.h"

enum class RGBMode : uint8_t {
    OFF = 0,
    SOLID,
    BREATHING,
    PULSE,
    RAINBOW,
    ALERT_RED,
    ALERT_GREEN,
    ALERT_VIOLET,
    HEARTBEAT
};

// Renomme en EBoxRGBColor pour eviter le conflit avec lgfx::bgr888_t
// qui est typedef'd comme RGBColor dans LovyanGFX
struct EBoxRGBColor { uint8_t r, g, b; };

class RGBEngine {
public:
    static void init();
    static void setMode(RGBMode mode);
    static void setColor(uint8_t r, uint8_t g, uint8_t b);
    static void setColorHSV(float h, float s, float v);
    static void taskRunner(void* pvParams);

private:
    static RGBMode       _mode;
    static EBoxRGBColor  _target;
    static uint32_t      _tick;

    static void _write(uint8_t r, uint8_t g, uint8_t b);
    static EBoxRGBColor _hsv2rgb(float h, float s, float v);
};
