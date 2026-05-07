// ============================================================
// engine/BuzzerEngine.cpp — SFX + Melodies riches
// Toutes les sequences musicales du jeu
// ============================================================
#include "BuzzerEngine.h"

QueueHandle_t BuzzerEngine::_queue = nullptr;

// ============================================================
//  SEQUENCES SFX & MELODIES
//  Format : {freq_Hz, duree_ms}  |  0 = silence
// ============================================================

// ── SFX de base ──────────────────────────────────────────────
static const BuzzerNote SFX_CLICK_SEQ[]    = {{NOTE_A5, 30}};

static const BuzzerNote SFX_CONFIRM_SEQ[]  = {
    {NOTE_E4, 80}, {NOTE_G4, 80}, {NOTE_C5, 120}
};

static const BuzzerNote SFX_ERROR_SEQ[]    = {
    {NOTE_A3, 150}, {NOTE_REST, 50}, {NOTE_G3, 200}
};

static const BuzzerNote SFX_ALARM_SEQ[]    = {
    {NOTE_A4, 80}, {NOTE_REST, 40}, {NOTE_A4, 80},
    {NOTE_REST, 40}, {NOTE_A4, 80}
};

static const BuzzerNote SFX_SUCCESS_SEQ[]  = {
    {NOTE_C4, 80}, {NOTE_E4, 80}, {NOTE_G4, 80}, {NOTE_C5, 150}
};

static const BuzzerNote SFX_LEVEL_UP_SEQ[] = {
    {NOTE_G4, 100}, {NOTE_A4, 100}, {NOTE_B4, 100}, {NOTE_C5, 200}
};

static const BuzzerNote SFX_HEARTBEAT_SEQ[] = {
    {NOTE_C3, 80}, {NOTE_REST, 60}, {NOTE_C3, 80}, {NOTE_REST, 700}
};

static const BuzzerNote SFX_BOOT_SEQ[]     = {
    {NOTE_A3, 120}, {NOTE_C4, 120}, {NOTE_E4, 120},
    {NOTE_REST, 80}, {NOTE_G4, 200}
};

static const BuzzerNote SFX_HAPTICK_SEQ[]  = {{NOTE_D5, 20}};

static const BuzzerNote SFX_COUNTDOWN_SEQ[]= {
    {NOTE_A4, 150}, {NOTE_REST, 850}
};

// ── QUIZ : Bonne reponse ──────────────────────────────────────
// Fanfare joyeuse ascendante
static const BuzzerNote SFX_QUIZ_WIN_SEQ[] = {
    {NOTE_E4,  80}, {NOTE_G4,  80}, {NOTE_C5,  80},
    {NOTE_E5, 120}, {NOTE_REST,40}, {NOTE_G5, 200}
};

// ── QUIZ : Mauvaise reponse ───────────────────────────────────
// Descente grave, tension
static const BuzzerNote SFX_QUIZ_FAIL_SEQ[] = {
    {NOTE_B4, 100}, {NOTE_REST, 30},
    {NOTE_A4, 100}, {NOTE_REST, 30},
    {NOTE_G3, 300}
};

// ── QUIZ : Tic timer ──────────────────────────────────────────
static const BuzzerNote SFX_QUIZ_TICK_SEQ[] = {
    {NOTE_C5, 30}, {NOTE_REST, 70}
};

// ── QUIZ : Revelation bonne reponse ──────────────────────────
// Petit tintement calme
static const BuzzerNote SFX_QUIZ_REVEAL_SEQ[] = {
    {NOTE_C4, 60}, {NOTE_REST, 30},
    {NOTE_E4, 60}, {NOTE_REST, 30},
    {NOTE_G4, 80}
};

// ── QUIZ : Jingle d'introduction ─────────────────────────────
// Melodie espiegle et intrigante
static const BuzzerNote SFX_QUIZ_INTRO_SEQ[] = {
    {NOTE_C4, 100}, {NOTE_E4, 100}, {NOTE_G4, 100},
    {NOTE_REST,50}, {NOTE_C5, 80},  {NOTE_B4,  80},
    {NOTE_G4,  80}, {NOTE_REST,80}, {NOTE_A4, 160},
    {NOTE_REST,40}, {NOTE_C5, 200}
};

// ── INTERLUDE : Fanfare recompense ───────────────────────────
// Sonnerie heroique : "Tu as reussi !"
static const BuzzerNote SFX_INTERLUDE_FANFARE_SEQ[] = {
    // Levee
    {NOTE_G4, 150},
    // Corps
    {NOTE_C5, 150}, {NOTE_C5, 150}, {NOTE_C5, 150},
    {NOTE_REST,60},
    {NOTE_G4, 100}, {NOTE_E4, 100}, {NOTE_G4, 100},
    // Chute + reprise
    {NOTE_C5, 300}, {NOTE_REST,80},
    // Flourish final
    {NOTE_E5, 100}, {NOTE_D5, 100}, {NOTE_C5, 200},
    {NOTE_REST,60},
    {NOTE_G5, 400}
};

// ── INTERLUDE : Deverrouillage ────────────────────────────────
// Son mecanique de serrure qui s'ouvre
static const BuzzerNote SFX_INTERLUDE_UNLOCK_SEQ[] = {
    {NOTE_C3, 80},  {NOTE_E3, 80},
    {NOTE_G3, 80},  {NOTE_REST,40},
    {NOTE_C4, 80},  {NOTE_E4, 80},
    {NOTE_REST,60},
    {NOTE_G4, 120}, {NOTE_C5, 200}
};

// ── DRAMATIC STING ────────────────────────────────────────────
// Court accent dramatique (transition, annonce)
static const BuzzerNote SFX_DRAMATIC_SEQ[] = {
    {NOTE_A3, 60},  {NOTE_REST,30},
    {NOTE_A3, 60},  {NOTE_REST,30},
    {NOTE_E4, 300}
};

// ── VICTOIRE LONGUE ───────────────────────────────────────────
// Melodie complete : hymne de victoire
// Inspiree de la structure "Do-Mi-Sol-Do (octave)" + coda
static const BuzzerNote SFX_VICTORY_LONG_SEQ[] = {
    // Theme A : montee heroique
    {NOTE_C4, 150}, {NOTE_E4, 150}, {NOTE_G4, 150},
    {NOTE_C5, 300}, {NOTE_REST,100},
    {NOTE_G4, 120}, {NOTE_C5, 120}, {NOTE_E5, 400},
    {NOTE_REST,150},
    // Theme B : variation
    {NOTE_D5, 120}, {NOTE_C5, 120}, {NOTE_B4, 120},
    {NOTE_A4, 120}, {NOTE_G4, 300}, {NOTE_REST,100},
    {NOTE_E4, 120}, {NOTE_G4, 120}, {NOTE_C5, 200},
    {NOTE_REST,100},
    // Coda triomphale
    {NOTE_G4, 80}, {NOTE_A4, 80}, {NOTE_B4, 80},
    {NOTE_C5, 80}, {NOTE_D5, 80}, {NOTE_E5, 80},
    {NOTE_REST,60},
    {NOTE_C5, 80}, {NOTE_E5, 80}, {NOTE_G5, 600}
};

// ── Victoire courte (ancienne) ────────────────────────────────
static const BuzzerNote SFX_VICTORY_SEQ[] = {
    {NOTE_C4, 100}, {NOTE_E4, 100}, {NOTE_G4, 100}, {NOTE_C5, 80},
    {NOTE_REST, 60}, {NOTE_G4, 80}, {NOTE_C5, 80}, {NOTE_E5, 200},
    {NOTE_REST, 80}, {NOTE_C5, 60}, {NOTE_E5, 60}, {NOTE_G5, 400}
};

// ============================================================
//  IMPL
// ============================================================

void BuzzerEngine::init() {
    _queue = xQueueCreate(64, sizeof(BuzzerNote));

    // API ESP-IDF v5 : ledcAttach remplace ledcSetup + ledcAttachPin
    ledcAttach(PIN_BUZZER, 1000, 8);
    ledcWrite(PIN_BUZZER, 0);

    xTaskCreatePinnedToCore(
        taskRunner, "buzzer_task",
        TASK_STACK_BUZZER, nullptr,
        TASK_PRIO_BUZZER, nullptr, 0  // Core 0
    );
}

// ── Tache FreeRTOS ───────────────────────────────────────────
void BuzzerEngine::taskRunner(void* pvParams) {
    BuzzerNote note;
    while (true) {
        if (xQueueReceive(_queue, &note, portMAX_DELAY) == pdTRUE) {
            if (note.freq > 0) {
                ledcChangeFrequency(PIN_BUZZER, note.freq, 8);
                ledcWrite(PIN_BUZZER, 128);
            } else {
                ledcWrite(PIN_BUZZER, 0);
            }
            vTaskDelay(pdMS_TO_TICKS(note.duration));
            ledcWrite(PIN_BUZZER, 0);
        }
    }
}

// ── Envoi d'une sequence ─────────────────────────────────────
void BuzzerEngine::_sendSequence(const BuzzerNote* seq, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        xQueueSend(_queue, &seq[i], 0);
    }
}

// ── API publique ─────────────────────────────────────────────
#define SEQ(arr) _sendSequence(arr, sizeof(arr)/sizeof(arr[0]))

void BuzzerEngine::play(SFX sfx) {
    xQueueReset(_queue);
    switch (sfx) {
        case SFX::CLICK:              SEQ(SFX_CLICK_SEQ);                 break;
        case SFX::CONFIRM:            SEQ(SFX_CONFIRM_SEQ);               break;
        case SFX::ERROR:              SEQ(SFX_ERROR_SEQ);                 break;
        case SFX::ALARM:              SEQ(SFX_ALARM_SEQ);                 break;
        case SFX::SUCCESS:            SEQ(SFX_SUCCESS_SEQ);               break;
        case SFX::LEVEL_UP:           SEQ(SFX_LEVEL_UP_SEQ);              break;
        case SFX::VICTORY:            SEQ(SFX_VICTORY_SEQ);               break;
        case SFX::HEARTBEAT:          SEQ(SFX_HEARTBEAT_SEQ);             break;
        case SFX::BOOT_CHIME:         SEQ(SFX_BOOT_SEQ);                  break;
        case SFX::HAPTICK:            SEQ(SFX_HAPTICK_SEQ);               break;
        case SFX::COUNTDOWN:          SEQ(SFX_COUNTDOWN_SEQ);             break;
        // Quiz
        case SFX::QUIZ_WIN:           SEQ(SFX_QUIZ_WIN_SEQ);              break;
        case SFX::QUIZ_FAIL:          SEQ(SFX_QUIZ_FAIL_SEQ);             break;
        case SFX::QUIZ_TICK:          SEQ(SFX_QUIZ_TICK_SEQ);             break;
        case SFX::QUIZ_REVEAL:        SEQ(SFX_QUIZ_REVEAL_SEQ);           break;
        case SFX::QUIZ_INTRO:         SEQ(SFX_QUIZ_INTRO_SEQ);            break;
        // Transitions
        case SFX::INTERLUDE_FANFARE:  SEQ(SFX_INTERLUDE_FANFARE_SEQ);    break;
        case SFX::INTERLUDE_UNLOCK:   SEQ(SFX_INTERLUDE_UNLOCK_SEQ);     break;
        case SFX::DRAMATIC_STING:     SEQ(SFX_DRAMATIC_SEQ);              break;
        case SFX::MELODY_VICTORY_LONG:SEQ(SFX_VICTORY_LONG_SEQ);         break;
    }
}

void BuzzerEngine::playNote(uint16_t freq, uint16_t durationMs) {
    BuzzerNote n = {freq, durationMs};
    xQueueSend(_queue, &n, 0);
}

void BuzzerEngine::stop() {
    xQueueReset(_queue);
    ledcWrite(PIN_BUZZER, 0);
}
