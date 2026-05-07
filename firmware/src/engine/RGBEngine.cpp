// ============================================================
// engine/RGBEngine.cpp — API LEDC v5 corrigee
// ============================================================
#include "RGBEngine.h"
#include <math.h>

RGBMode        RGBEngine::_mode   = RGBMode::OFF;
EBoxRGBColor   RGBEngine::_target = {0, 0, 0};
uint32_t       RGBEngine::_tick   = 0;

void RGBEngine::init() {
    // API ESP-IDF v5 : ledcAttach remplace ledcSetup + ledcAttachPin
    ledcAttach(PIN_RGB_R, RGB_PWM_FREQ, RGB_PWM_RES);
    ledcAttach(PIN_RGB_G, RGB_PWM_FREQ, RGB_PWM_RES);
    ledcAttach(PIN_RGB_B, RGB_PWM_FREQ, RGB_PWM_RES);
    _write(0, 0, 0);

    xTaskCreatePinnedToCore(
        taskRunner, "rgb_task", 2048,
        nullptr, 1, nullptr, 0  // Core 0
    );
}

void RGBEngine::_write(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(PIN_RGB_R, r);
    ledcWrite(PIN_RGB_G, g);
    ledcWrite(PIN_RGB_B, b);
}

EBoxRGBColor RGBEngine::_hsv2rgb(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    int   i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        case 5: r=v; g=p; b=q; break;
    }
    return {(uint8_t)(r*255), (uint8_t)(g*255), (uint8_t)(b*255)};
}

void RGBEngine::setMode(RGBMode mode) { _mode = mode; _tick = 0; }

void RGBEngine::setColor(uint8_t r, uint8_t g, uint8_t b) {
    _target = {r, g, b};
    _mode   = RGBMode::SOLID;
    _write(r, g, b);
}

void RGBEngine::setColorHSV(float h, float s, float v) {
    auto c = _hsv2rgb(h, s, v);
    setColor(c.r, c.g, c.b);
}

void RGBEngine::taskRunner(void* pvParams) {
    while (true) {
        _tick++;
        float t = (float)_tick;

        switch (_mode) {
            case RGBMode::OFF:
                _write(0, 0, 0); break;

            case RGBMode::SOLID:
                _write(_target.r, _target.g, _target.b); break;

            case RGBMode::BREATHING: {
                float br = (sinf(t * 0.04f) + 1.0f) * 0.5f;
                _write((uint8_t)(_target.r * br),
                       (uint8_t)(_target.g * br),
                       (uint8_t)(_target.b * br));
                break;
            }
            case RGBMode::PULSE: {
                float br = (sinf(t * 0.12f) + 1.0f) * 0.5f;
                _write((uint8_t)(_target.r * br),
                       (uint8_t)(_target.g * br),
                       (uint8_t)(_target.b * br));
                break;
            }
            case RGBMode::RAINBOW: {
                float h   = fmod(t * 0.002f, 1.0f);
                auto  col = _hsv2rgb(h, 1.0f, 1.0f);
                _write(col.r, col.g, col.b);
                break;
            }
            case RGBMode::ALERT_RED: {
                uint8_t v = (_tick % 20 < 10) ? 255 : 0;
                _write(v, 0, 0); break;
            }
            case RGBMode::ALERT_GREEN: {
                uint8_t v = (_tick % 20 < 10) ? 255 : 0;
                _write(0, v, 0); break;
            }
            case RGBMode::ALERT_VIOLET: {
                uint8_t v = (_tick % 30 < 15) ? 200 : 0;
                _write(v, 0, v); break;
            }
            case RGBMode::HEARTBEAT: {
                uint32_t p = _tick % 60;
                uint8_t  v = 0;
                if      (p < 8)  v = (uint8_t)(p * 31);
                else if (p < 16) v = (uint8_t)((16 - p) * 31);
                else if (p < 24) v = (uint8_t)((p - 16) * 20);
                else if (p < 32) v = (uint8_t)((32 - p) * 20);
                _write(v, 0, 0); break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));  // 50 Hz
    }
}
