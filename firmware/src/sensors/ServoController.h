#pragma once
// ============================================================
// sensors/ServoController.h — SG90 avec easing non-bloquant
// LEDC PWM 50Hz, animations fluides, séquences chorégraphiées
// ============================================================
#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

class ServoController {
public:
    static void init() {
        ESP32PWM::allocateTimer(3);
        _servo.setPeriodHertz(SERVO_FREQ);
        _servo.attach(PIN_SERVO, 500, 2400);
        _servo.write(SERVO_LOCKED);
        _currentAngle  = SERVO_LOCKED;
        _targetAngle   = SERVO_LOCKED;
        _animating     = false;
    }

    // Définir la cible avec easing — appeler update() chaque frame
    static void moveTo(int angleDeg, uint32_t durationMs = 600) {
        _startAngle  = _currentAngle;
        _targetAngle = constrain(angleDeg, 0, 180);
        _startTime   = millis();
        _duration    = durationMs;
        _animating   = true;
    }

    // Séquence dramatique : hésitation + ouverture avec rebond
    static void openDramatic() {
        // Pré-mouvement d'hésitation (5°), puis ouverture complète
        moveTo(SERVO_LOCKED + 5, 200);
        _pendingOpen = true;
    }

    // À appeler dans task_sensors ou loop (non-bloquant)
    static void update() {
        if (!_animating) return;
        uint32_t elapsed = millis() - _startTime;
        float    t       = constrain((float)elapsed / _duration, 0.0f, 1.0f);

        // Easing out-cubic + rebond léger
        float ease = 1.0f - pow(1.0f - t, 3.0f);

        _currentAngle = (float)_startAngle + ease * (_targetAngle - _startAngle);
        _servo.write((int)_currentAngle);

        if (t >= 1.0f) {
            _animating = false;
            if (_pendingOpen) {
                _pendingOpen = false;
                moveTo(SERVO_OPEN, 800);  // Ouverture principale
            }
        }
    }

    static bool isLocked()    { return _currentAngle < SERVO_LOCKED + 5; }
    static bool isAnimating() { return _animating; }
    static void lock()        { moveTo(SERVO_LOCKED, 400); }

private:
    static Servo    _servo;
    static float    _currentAngle;
    static int      _targetAngle, _startAngle;
    static uint32_t _startTime, _duration;
    static bool     _animating, _pendingOpen;
};

// Définitions statiques (dans ServoController.cpp si besoin)
inline Servo    ServoController::_servo;
inline float    ServoController::_currentAngle   = SERVO_LOCKED;
inline int      ServoController::_targetAngle    = SERVO_LOCKED;
inline int      ServoController::_startAngle     = SERVO_LOCKED;
inline uint32_t ServoController::_startTime      = 0;
inline uint32_t ServoController::_duration       = 600;
inline bool     ServoController::_animating      = false;
inline bool     ServoController::_pendingOpen    = false;
