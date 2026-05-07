// ============================================================
// fsm/states/StateVictory.cpp — Séquence de victoire épique
// ============================================================
#pragma once
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/ServoController.h"
#include "../../config.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;
extern uint32_t   g_sessionStartTime;

class StateVictory {
public:
    static void run(GameProfile profile) {
        // Phase 1 — Ouverture du servo (dramatique)
        ServoController::openDramatic();
        BuzzerEngine::play(SFX::VICTORY);
        RGBEngine::setMode(RGBMode::RAINBOW);

        // Phase 2 — Flash blanc + explosion de particules
        DisplayEngine::buf.fillScreen(COL_WHITE);
        DisplayEngine::flush();
        vTaskDelay(pdMS_TO_TICKS(150));

        DisplayEngine::clear();
        DisplayEngine::particleInit(TFT_W/2, TFT_H/2, COL_GOLD, 120);

        // Boucle d'animation victoire (3 secondes)
        uint32_t start = millis();
        while (!ServoController::isAnimating() || millis() - start < 3000) {
            ServoController::update();
            DisplayEngine::clear(0x0008);
            DisplayEngine::particleUpdate();
            DisplayEngine::particleDraw();

            // Titre pulsant
            float pulse = (sinf(millis() * 0.006f) + 1.0f) * 0.5f;
            int   y     = 100 + (int)(8 * pulse);
            DisplayEngine::drawTextCentered(y,      "BOITE OUVERTE !", COL_GOLD, 2);
            DisplayEngine::drawTextCentered(y + 35, "MISSION ACCOMPLIE", COL_TERMINAL, 1);

            DisplayEngine::flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }

        // Phase 3 — Respiration + écran de score
        DisplayEngine::fadeToBlack(400);
        DisplayEngine::breathPause(600);
        DisplayEngine::fadeFromBlack(400);

        // Calcul du temps total
        DateTime now = rtc.now();
        uint32_t totalSec = now.unixtime() - g_sessionStartTime;
        uint32_t mins = totalSec / 60;
        uint32_t secs = totalSec % 60;

        DisplayEngine::clear();
        DisplayEngine::drawRoundBox(5, 20, TFT_W - 10, TFT_H - 40, 12, COL_GREY_DARK, true);
        DisplayEngine::drawTextCentered(40,  "[ RAPPORT FINAL ]", COL_GOLD, 1);
        DisplayEngine::buf.drawFastHLine(20, 58, TFT_W - 40, COL_GREY);

        const char* rankName = "AGENT CERTIFIE";
        switch (profile) {
            case GameProfile::PUBLIC:   rankName = "CITOYEN HEROIQUE"; break;
            case GameProfile::ADVANCED: rankName = "TECHNICIEN ELITE"; break;
            case GameProfile::EXPERT:   rankName = "AGENT CERTIFIE";   break;
        }
        DisplayEngine::drawTextCentered(75, rankName, COL_TERMINAL, 1);

        char timeBuf[30];
        snprintf(timeBuf, sizeof(timeBuf), "TEMPS : %02lum %02lus", mins, secs);
        DisplayEngine::drawTextCentered(110, timeBuf, COL_WHITE, 1);

        DisplayEngine::drawTextCentered(145, "PROTOCOLES :", COL_GREY, 1);
        DisplayEngine::drawTextCentered(165, "01 L'ACADEMIE     OK", COL_TERMINAL, 1);
        DisplayEngine::drawTextCentered(183, "02 LABYRINTHE     OK", COL_TERMINAL, 1);
        DisplayEngine::drawTextCentered(201, "03 SPECTRE RADIO  OK", COL_TERMINAL, 1);
        DisplayEngine::drawTextCentered(219, "04 SENTINELLE     OK", COL_TERMINAL, 1);
        DisplayEngine::drawTextCentered(237, "05 LIBERATION     OK", COL_GOLD, 1);

        DisplayEngine::drawTextCentered(268, "Appuyer pour rejouer", COL_GREY, 1);
        DisplayEngine::flush();

        // Attendre input
        vTaskDelay(pdMS_TO_TICKS(5000));
        // Retour géré par GameFSM
    }
};
