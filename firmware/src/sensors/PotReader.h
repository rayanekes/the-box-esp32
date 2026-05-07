#pragma once
// ============================================================
// sensors/PotReader.h — Potentiomètre avec haptique et zones
// ADC 12-bit, moving average, zones cliquantes via buzzer
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "engine/BuzzerEngine.h"

class PotReader {
public:
    static void init() { /* ADC natif */ }

    // Lecture lissée (moving average 32 échantillons)
    static uint16_t readSmooth() {
        static uint32_t buf[POT_SMOOTH] = {};
        static uint8_t  idx = 0;
        buf[idx] = analogRead(PIN_POT);
        idx = (idx + 1) % POT_SMOOTH;
        uint32_t sum = 0;
        for (auto v : buf) sum += v;
        return (uint16_t)(sum / POT_SMOOTH);
    }

    // Valeur normalisée 0.0–1.0
    static float readNorm() {
        return readSmooth() / 4095.0f;
    }

    // Vérifie si la valeur est dans une zone cible (±tolérance)
    // Si on vient d'entrer dans la zone → joue le SFX haptique
    static bool inZone(uint16_t target, uint16_t tolerance) {
        static bool wasInZone = false;
        uint16_t v = readSmooth();
        bool inZ = abs((int)v - (int)target) <= (int)tolerance;
        if (inZ && !wasInZone) {
            BuzzerEngine::play(SFX::HAPTICK);
        }
        wasInZone = inZ;
        return inZ;
    }

    // Retourne un entier mappé dans [outMin, outMax]
    static int mapTo(int outMin, int outMax) {
        return map((long)readSmooth(), 0, 4095, outMin, outMax);
    }
};
