#pragma once
// ============================================================
// sensors/JoystickReader.h — Joystick analogique non-bloquant
// ADC 12-bit, zone morte, directions, vitesse angulaire
// ============================================================
#include <Arduino.h>
#include "config.h"

enum class JoyDir : uint8_t {
    NONE = 0,
    UP, DOWN, LEFT, RIGHT,
    UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT,
    PRESS   // Bouton SW enfoncé
};

struct JoyState {
    int     rawX, rawY;    // 0–4095
    float   normX, normY;  // -1.0 à +1.0
    JoyDir  dir;
    bool    pressed;
};

class JoystickReader {
public:
    static void init() {
        pinMode(PIN_JOY_SW, INPUT_PULLUP);
    }

    static JoyState read() {
        JoyState s;
        s.rawX = analogRead(PIN_JOY_X);
        s.rawY = analogRead(PIN_JOY_Y);

        // Normalisation -1.0 à +1.0 avec zone morte
        int cx = s.rawX - 2048;
        int cy = s.rawY - 2048;
        s.normX = (abs(cx) > JOY_DEADZONE) ? (float)cx / 2048.0f : 0.0f;
        s.normY = (abs(cy) > JOY_DEADZONE) ? (float)cy / 2048.0f : 0.0f;

        // Direction
        bool up    = s.normY < -0.5f;
        bool down  = s.normY >  0.5f;
        bool left  = s.normX < -0.5f;
        bool right = s.normX >  0.5f;

        if      (up && left)    s.dir = JoyDir::UP_LEFT;
        else if (up && right)   s.dir = JoyDir::UP_RIGHT;
        else if (down && left)  s.dir = JoyDir::DOWN_LEFT;
        else if (down && right) s.dir = JoyDir::DOWN_RIGHT;
        else if (up)            s.dir = JoyDir::UP;
        else if (down)          s.dir = JoyDir::DOWN;
        else if (left)          s.dir = JoyDir::LEFT;
        else if (right)         s.dir = JoyDir::RIGHT;
        else                    s.dir = JoyDir::NONE;

        s.pressed = (digitalRead(PIN_JOY_SW) == LOW);
        return s;
    }

    // Retourne true si la direction correspond (debounce interne)
    static bool isDir(JoyDir d) {
        static JoyDir lastDir = JoyDir::NONE;
        static uint32_t lastTime = 0;
        auto s = read();
        if (s.dir == d && (millis() - lastTime) > 150) {
            lastTime = millis();
            lastDir  = d;
            return true;
        }
        return false;
    }
};
