#pragma once
// ============================================================
// quiz/QuizEngine.h — Moteur de quiz sur LittleFS
// Charge 1 fichier domaine a la fois, randomise, filtre par difficulte
// ============================================================
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"

// ── Domaines disponibles ──────────────────────────────────────
enum class QuizDomain : uint8_t {
    LOGIQUE  = 0,
    CULTURE  = 1,
    ELECTRO  = 2,
    MATHS    = 3,
    PHYSIQUE = 4,
    INFO     = 5,
    CHIMIE   = 6,
    BIO      = 7,
    HISTOIRE = 8,
    ENIGMES  = 9,
    RANDOM   = 10   // Choisit un domaine aleatoirement
};

// Mapping niveau→domaine (inter-niveaux)
// Apres enigme 1 → culture, 2 → maths, 3 → physique, 4 → electro
static const QuizDomain LEVEL_DOMAIN[] = {
    QuizDomain::CULTURE,
    QuizDomain::MATHS,
    QuizDomain::PHYSIQUE,
    QuizDomain::ELECTRO
};

// ── Structure d'une question chargee ─────────────────────────
struct QuizQuestion {
    char   question[160];
    char   choices[5][80];   // Max 5 choix
    uint8_t numChoices;      // 3, 4 ou 5
    uint8_t correctIdx;
    uint8_t difficulty;      // 1, 2 ou 3
    char   explanation[180]; // Optionnel
    bool   valid;            // false si chargement echoue
};

// ── API du moteur ─────────────────────────────────────────────
class QuizEngine {
public:
    // Charge un fichier domaine et selectionne une question aleatoire
    // filtre par difficulte maximum (selon profil)
    static QuizQuestion loadQuestion(QuizDomain domain, GameProfile profile);

    // Nombre de choix selon profil
    static uint8_t numChoicesForProfile(GameProfile p);

    // Temps imparti en secondes selon profil et difficulte
    static uint32_t timeLimitMs(GameProfile p, uint8_t difficulty);

    // Chemin du fichier JSON pour un domaine
    static const char* domainPath(QuizDomain d);
    static const char* domainLabel(QuizDomain d);
    static const char* domainIcon(QuizDomain d);

private:
    // Compte les questions valides dans un doc JSON
    static int _countValid(JsonArray arr, uint8_t maxDiff);

    // Selectionne un index au hasard parmi les valides
    static int _pickRandom(JsonArray arr, uint8_t maxDiff, int totalValid);
};
