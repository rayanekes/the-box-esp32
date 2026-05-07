#pragma once
// ============================================================
// fsm/TutorialEngine.h — Système de tutoriels AAA réutilisable
// Chaque étape = une animation + un input à détecter
// ============================================================
#include <Arduino.h>
#include "../config.h"
#include "../engine/DisplayEngine.h"
#include "../engine/BuzzerEngine.h"
#include "../engine/RGBEngine.h"
#include "../sensors/JoystickReader.h"
#include "../sensors/SonarReader.h"
#include "../sensors/PotReader.h"

enum class TutInputType : uint8_t {
    JOY_UP, JOY_DOWN, JOY_LEFT, JOY_RIGHT, JOY_PRESS,
    POT_TURN_RIGHT,   // pot > 3000
    POT_TURN_LEFT,    // pot < 1000
    SONAR_NEAR,       // dist < 30cm
    SONAR_HOLD,       // dist 8–15cm stable 2s
    ANY_KEY,          // N'importe quelle touche clavier
    WAIT,             // Pas d'input requis, juste attendre N ms
    AUTO_DONE         // Étape narrative pure, validée auto
};

struct TutStep {
    const char*   line1;        // Texte principal (gros)
    const char*   line2;        // Sous-texte (petit, peut être nullptr)
    TutInputType  inputType;
    ArrowDir      arrowDir;     // Pour les flèches directionnelles
    uint32_t      waitMs;       // Pour WAIT ou délai auto
    uint32_t      iconCenterX;  // Position X de l'icône animée
    uint32_t      iconCenterY;  // Position Y de l'icône animée
};

class TutorialEngine {
public:
    // Définir les étapes du tutoriel
    void begin(const TutStep* steps, uint8_t count, const char* title);

    // Appelé chaque frame — retourne true quand TOUTES les étapes sont passées
    bool update();

    // Reset pour réutilisation
    void reset();

private:
    const TutStep* _steps    = nullptr;
    uint8_t        _count    = 0;
    uint8_t        _current  = 0;
    const char*    _title    = nullptr;
    uint32_t       _stepEnter= 0;
    bool           _showing  = false;
    uint32_t       _holdStart= 0;
    bool           _holdActive = false;

    void _drawStep(const TutStep& s);
    bool _checkInput(const TutStep& s);
    void _onStepSuccess();
};
