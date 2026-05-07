// ============================================================
// quiz/QuizEngine.cpp — Implementation du moteur de quiz
// ============================================================
#include "QuizEngine.h"

// ── Metadonnees des domaines ──────────────────────────────────
static const char* DOMAIN_PATHS[] = {
    "/quiz/logique.json",
    "/quiz/culture.json",
    "/quiz/electro.json",
    "/quiz/maths.json",
    "/quiz/physique.json",
    "/quiz/info.json",
    "/quiz/chimie.json",
    "/quiz/bio.json",
    "/quiz/histoire.json",
    "/quiz/enigmes.json"
};

static const char* DOMAIN_LABELS[] = {
    "LOGIQUE",
    "CULTURE GENERALE",
    "ELECTRONIQUE",
    "MATHEMATIQUES",
    "PHYSIQUE",
    "INFORMATIQUE",
    "CHIMIE",
    "SCIENCES NAT.",
    "HIST. SCIENCES",
    "ENIGMES & JEUX"
};

static const char* DOMAIN_ICONS[] = {
    "LOGIC", "WORLD", "BOLT", "MATH", "ATOM",
    "CODE",  "FLASK", "LEAF", "HIST", "GAME"
};

// ── API domaine ───────────────────────────────────────────────
const char* QuizEngine::domainPath(QuizDomain d) {
    uint8_t idx = (uint8_t)d;
    if (idx >= 10) idx = random(0, 10);   // RANDOM
    return DOMAIN_PATHS[idx];
}

const char* QuizEngine::domainLabel(QuizDomain d) {
    uint8_t idx = (uint8_t)d;
    if (idx >= 10) idx = 0;
    return DOMAIN_LABELS[idx];
}

const char* QuizEngine::domainIcon(QuizDomain d) {
    uint8_t idx = (uint8_t)d;
    if (idx >= 10) idx = 0;
    return DOMAIN_ICONS[idx];
}

// ── Nombre de choix selon profil ──────────────────────────────
uint8_t QuizEngine::numChoicesForProfile(GameProfile p) {
    switch (p) {
        case GameProfile::PUBLIC:   return 3;
        case GameProfile::ADVANCED: return 4;
        case GameProfile::EXPERT:   return 5;
        default: return 3;
    }
}

// ── Temps imparti ─────────────────────────────────────────────
uint32_t QuizEngine::timeLimitMs(GameProfile p, uint8_t difficulty) {
    // Base : PUBLIC=30s, AVANCED=25s, EXPERT=20s
    // Bonus par difficulte : +10s par niveau
    uint32_t base;
    switch (p) {
        case GameProfile::PUBLIC:   base = 30000; break;
        case GameProfile::ADVANCED: base = 25000; break;
        case GameProfile::EXPERT:   base = 20000; break;
        default: base = 30000;
    }
    return base + (difficulty - 1) * 10000UL;
}

// ── Comptage des questions valides ────────────────────────────
int QuizEngine::_countValid(JsonArray arr, uint8_t maxDiff) {
    int count = 0;
    for (JsonObject q : arr) {
        if (q["d"].as<uint8_t>() <= maxDiff) count++;
    }
    return count;
}

// ── Selection aleatoire parmi les valides ─────────────────────
int QuizEngine::_pickRandom(JsonArray arr, uint8_t maxDiff, int totalValid) {
    int target = random(0, totalValid);
    int count  = 0;
    int idx    = 0;
    for (JsonObject q : arr) {
        if (q["d"].as<uint8_t>() <= maxDiff) {
            if (count == target) return idx;
            count++;
        }
        idx++;
    }
    return 0;
}

// ── Chargement principal ──────────────────────────────────────
QuizQuestion QuizEngine::loadQuestion(QuizDomain domain, GameProfile profile) {
    QuizQuestion result;
    memset(&result, 0, sizeof(result));
    result.valid = false;

    // Resoudre RANDOM
    QuizDomain resolved = domain;
    if (domain == QuizDomain::RANDOM) {
        resolved = (QuizDomain)random(0, 10);
    }

    const char* path = domainPath(resolved);

    if (!LittleFS.exists(path)) {
        Serial.printf("[QUIZ] Fichier introuvable : %s\n", path);
        return result;
    }

    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.printf("[QUIZ] Erreur ouverture : %s\n", path);
        return result;
    }

    // Allouer le document en PSRAM via heap
    // Taille max file ~18Ko, on prend 24Ko de marge
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[QUIZ] JSON error: %s\n", err.c_str());
        return result;
    }

    JsonArray arr = doc["questions"].as<JsonArray>();
    if (arr.isNull() || arr.size() == 0) {
        Serial.println("[QUIZ] Aucune question trouvee");
        return result;
    }

    // Filtrage selon difficulte max du profil
    uint8_t maxDiff;
    switch (profile) {
        case GameProfile::PUBLIC:   maxDiff = 1; break;
        case GameProfile::ADVANCED: maxDiff = 2; break;
        case GameProfile::EXPERT:   maxDiff = 3; break;
        default: maxDiff = 2;
    }

    int totalValid = _countValid(arr, maxDiff);
    if (totalValid == 0) {
        Serial.println("[QUIZ] Aucune question pour ce profil");
        return result;
    }

    int pickedIdx = _pickRandom(arr, maxDiff, totalValid);
    JsonObject q  = arr[pickedIdx];

    // Remplir la structure
    result.difficulty = q["d"].as<uint8_t>();
    strlcpy(result.question, q["q"].as<const char*>(), sizeof(result.question));

    JsonArray choicesArr = q["c"].as<JsonArray>();
    uint8_t wantedChoices = numChoicesForProfile(profile);
    uint8_t availChoices  = choicesArr.size();   // 3, 4 ou 5 selon la question
    result.numChoices     = min(wantedChoices, availChoices);

    // Mélanger les choix et tracker la bonne reponse
    uint8_t correctOrig = q["a"].as<uint8_t>();
    // Pour simplifier : on prend les choix tels quels (deja bien ordonnes)
    // Si on a plus de choix disponibles que voulus, on les coupe
    for (uint8_t i = 0; i < result.numChoices; i++) {
        strlcpy(result.choices[i], choicesArr[i].as<const char*>(), sizeof(result.choices[i]));
    }
    result.correctIdx = correctOrig;  // L'index reste valide tant qu'on ne melange pas

    // Explication optionnelle
    if (q.containsKey("e")) {
        strlcpy(result.explanation, q["e"].as<const char*>(), sizeof(result.explanation));
    }

    result.valid = true;
    Serial.printf("[QUIZ] Question chargee : diff=%d, choices=%d\n",
                  result.difficulty, result.numChoices);
    return result;
}
