// ============================================================
// Énigme 1 — "L'Académie des Agents"
// Tutoriel pur AAA : chaque composant est introduit interactivement
// ============================================================
#include "StateEnigma1.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"

// Étapes du tutoriel (identiques tous profils — c'est l'onboarding)
static const TutStep ACAD_STEPS[] = {
    {
        "DEPLACE VERS LE HAUT",
        "Utilise le joystick",
        TutInputType::JOY_UP, ArrowDir::UP, 0,
        TFT_W/2, 130
    },
    {
        "DEPLACE VERS LE BAS",
        nullptr,
        TutInputType::JOY_DOWN, ArrowDir::DOWN, 0,
        TFT_W/2, 130
    },
    {
        "DEPLACE A GAUCHE",
        nullptr,
        TutInputType::JOY_LEFT, ArrowDir::LEFT, 0,
        TFT_W/2, 130
    },
    {
        "DEPLACE A DROITE",
        nullptr,
        TutInputType::JOY_RIGHT, ArrowDir::RIGHT, 0,
        TFT_W/2, 130
    },
    {
        "APPUIE SUR LE JOYSTICK",
        "Enfonce-le verticalement",
        TutInputType::JOY_PRESS, ArrowDir::UP, 0,
        TFT_W/2, 130
    },
    {
        "TOURNE LE POTENTIOMETRE",
        "Vers la droite au maximum",
        TutInputType::POT_TURN_RIGHT, ArrowDir::RIGHT, 0,
        TFT_W/2, 140
    },
    {
        "APPROCHE TA MAIN",
        "Du capteur ultrason (<30cm)",
        TutInputType::SONAR_NEAR, ArrowDir::DOWN, 0,
        TFT_W/2, 130
    },
    {
        "QUALIFICATION : REUSSIE",
        "Tu es pret, Agent.",
        TutInputType::WAIT, ArrowDir::UP, 2500,
        TFT_W/2, 130
    }
};

void StateEnigma1::onEnter(GameProfile p) {
    _profile = p;
    _enterTime = millis();
    _tutDone = false;

    // Respirations + annonce
    DisplayEngine::clear();
    DisplayEngine::drawTextCentered(100, "PROTOCOLE 01", COL_GOLD, 2);
    DisplayEngine::drawTextCentered(135, "L'ACADEMIE", COL_WHITE, 2);
    DisplayEngine::drawTextCentered(170, "DES AGENTS", COL_WHITE, 2);
    DisplayEngine::flush();
    BuzzerEngine::play(SFX::LEVEL_UP);
    RGBEngine::setMode(RGBMode::BREATHING);
    RGBEngine::setColor(0, 60, 200);
    vTaskDelay(pdMS_TO_TICKS(2500));
    DisplayEngine::fadeToBlack(400);
    DisplayEngine::breathPause(700);

    _tut.begin(ACAD_STEPS, 8, "FORMATION AGENT");
}

bool StateEnigma1::update() {
    RGBEngine::setMode(RGBMode::BREATHING);
    if (_tut.update()) {
        // Victoire de l'onboarding
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(120, "AGENT QUALIFIE", COL_TERMINAL, 2);
        DisplayEngine::drawTextCentered(155, "Bonne chance...", COL_GREY, 1);
        DisplayEngine::flush();
        BuzzerEngine::play(SFX::SUCCESS);
        RGBEngine::setMode(RGBMode::RAINBOW);
        vTaskDelay(pdMS_TO_TICKS(2000));
        DisplayEngine::fadeToBlack(500);
        DisplayEngine::breathPause(800);
        return true;
    }
    return false;
}
