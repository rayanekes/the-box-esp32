#pragma once
// ============================================================
// sensors/SonarReader.h — HC-SR04 non-bloquant
// Mesure par pulseIn avec timeout, filtrage médian
// ============================================================
#include <Arduino.h>
#include "config.h"

class SonarReader {
public:
    static void init() {
        pinMode(PIN_SONAR_TRIG, OUTPUT);
        pinMode(PIN_SONAR_ECHO, INPUT);
        digitalWrite(PIN_SONAR_TRIG, LOW);
    }

    // Mesure instantanée en cm (timeout 25ms → max ~4m)
    static float readCm() {
        digitalWrite(PIN_SONAR_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(PIN_SONAR_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_SONAR_TRIG, LOW);
        long dur = pulseIn(PIN_SONAR_ECHO, HIGH, 25000UL);
        if (dur == 0) return 400.0f;  // Hors portée
        return dur * 0.01716f;        // cm = µs × (34300/2)/1e6
    }

    // Médiane de 3 mesures pour filtrer les glitches
    static float readFiltered() {
        float a = readCm(); delayMicroseconds(500);
        float b = readCm(); delayMicroseconds(500);
        float c = readCm();
        // Tri bulle sur 3 éléments
        if (a > b) { float t = a; a = b; b = t; }
        if (b > c) { float t = b; b = c; c = t; }
        if (a > b) { float t = a; a = b; b = t; }
        return b;  // Médiane
    }

    // Détecte une "vague" rapide (approche + recul < 400ms)
    static bool detectWave(float triggerDist = 20.0f) {
        static enum { IDLE, NEAR, DONE } state = IDLE;
        static uint32_t nearTime = 0;
        float d = readCm();
        switch (state) {
            case IDLE:
                if (d < triggerDist) { state = NEAR; nearTime = millis(); }
                break;
            case NEAR:
                if (d >= triggerDist) {
                    state = IDLE;
                    if ((millis() - nearTime) < 400) return true;
                }
                if ((millis() - nearTime) > 1500) state = IDLE;
                break;
            default: break;
        }
        return false;
    }
};
