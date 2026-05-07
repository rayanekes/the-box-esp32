#pragma once
// ============================================================
// engine/BuzzerEngine.h — Synthetiseur SFX + Melodies
// File de notes LEDC sur tache dediee Core 0
// ============================================================
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"

// ── Structure d'une note dans la file ────────────────────────
struct BuzzerNote {
    uint16_t freq;      // Hz (0 = silence/pause)
    uint16_t duration;  // ms
};

// ── SFX + Melodies ────────────────────────────────────────────
enum class SFX : uint8_t {
    // Effets sonores courts
    CLICK,
    CONFIRM,
    ERROR,
    ALARM,
    SUCCESS,
    VICTORY,
    HEARTBEAT,
    BOOT_CHIME,
    LEVEL_UP,
    HAPTICK,
    COUNTDOWN,
    // Quiz
    QUIZ_WIN,         // Bonne reponse — fanfare courte
    QUIZ_FAIL,        // Mauvaise reponse — son descendant
    QUIZ_TICK,        // Tic du timer quiz
    QUIZ_REVEAL,      // Revelation de la bonne reponse
    QUIZ_INTRO,       // Jingle d'intro du quiz
    // Ambiances / Transitions
    INTERLUDE_FANFARE, // Fanfare recompense entre niveaux
    INTERLUDE_UNLOCK,  // Son de deverrouillage
    DRAMATIC_STING,    // Accent dramatique court
    MELODY_VICTORY_LONG // Melodie de victoire complete
};

class BuzzerEngine {
public:
    static void init();
    static void play(SFX sfx);
    static void playNote(uint16_t freq, uint16_t durationMs);
    static void stop();
    static void taskRunner(void* pvParams);  // Tache FreeRTOS

private:
    static QueueHandle_t _queue;
    static void _sendSequence(const BuzzerNote* seq, size_t len);
};
