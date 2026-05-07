// ============================================================
// fsm/states/StateInterlude.cpp
// Quiz obligatoire inter-niveaux : recompense narrative
// 3 tentatives max, puis revelation + passage au niveau suivant
// ============================================================
#include "StateInterlude.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/JoystickReader.h"
#include "../../network/EscapeWebServer.h"

// Titres narratifs par niveau
static const char* REWARD_TITLES[] = {
    "PROTOCOLE 01 : VALIDE !",
    "PROTOCOLE 02 : VALIDE !",
    "PROTOCOLE 03 : VALIDE !",
    "PROTOCOLE 04 : VALIDE !"
};

static const char* REWARD_SUBS[] = {
    "Defi intellectuel debloque",
    "Acces niveau suivant requis",
    "Epreuve mentale activee",
    "Dernier verrou intellectuel"
};

// ── Dessine la question + les choix ──────────────────────────
static void _drawQuestion(const QuizQuestion& q, int8_t highlighted,
                          int8_t answered, bool correct,
                          uint32_t remainMs, uint8_t attempt,
                          const char* domainLabel) {
    DisplayEngine::clear(0x0008);

    // ── Header domaine ────────────────────────────────────────
    DisplayEngine::buf.fillRect(0, 0, TFT_W, 28, COL_VIOLET);
    char hdr[40];
    snprintf(hdr, sizeof(hdr), "[ %s ]", domainLabel);
    DisplayEngine::drawTextCentered(7, hdr, COL_GOLD, 1);
    char attBuf[20];
    snprintf(attBuf, sizeof(attBuf), "Essai %d/%d", attempt, QUIZ_MAX_ATTEMPTS);
    DisplayEngine::drawText(5, 14, attBuf, COL_GREY, 1);

    // ── Timer arc ────────────────────────────────────────────
    // Simple barre de progression
    uint32_t totalMs = q.difficulty == 1 ? 30000 : (q.difficulty == 2 ? 40000 : 50000);
    float pct = (float)remainMs / totalMs;
    uint16_t barCol = pct > 0.5f ? COL_TERMINAL : (pct > 0.25f ? COL_GOLD : COL_DANGER);
    DisplayEngine::drawProgressBar(5, 28, TFT_W - 10, 5, pct, barCol, COL_GREY_DARK);

    // ── Texte de la question (wrap manuel) ───────────────────
    // Zone question : y=40 a y=140
    {
        const char* txt = q.question;
        int y = 42;
        int lineW = TFT_W - 10;
        char lineBuf[36];
        int pos = 0, len = strlen(txt);
        int lineLen = 0;

        while (pos < len && y < 140) {
            // Remplir une ligne
            lineLen = 0;
            int wordStart = pos;
            while (pos < len && lineLen < 28) {
                if (txt[pos] == ' ' || pos == len - 1) {
                    int wl = (pos == len-1) ? pos - wordStart + 1 : pos - wordStart;
                    if (lineLen + wl > 28 && lineLen > 0) break;
                    lineLen += wl + 1;
                    pos++;
                    wordStart = pos;
                } else {
                    pos++;
                }
            }
            // Copier
            int copyLen = min((int)(pos - (pos >= len ? len-1 : pos)), 35);
            // Simplification : on coupe par tranches de 28 chars max
            snprintf(lineBuf, sizeof(lineBuf), "%.*s", 28, txt + (y-42)/12*28);
            DisplayEngine::drawText(5, y, lineBuf, COL_WHITE, 1);
            y += 14;
        }
        // Fallback simple : affichage tronque sur 2 lignes
        if (len <= 28) {
            DisplayEngine::drawText(5, 42, q.question, COL_WHITE, 1);
        } else if (len <= 56) {
            char l1[29], l2[29];
            strncpy(l1, q.question, 28); l1[28]='\0';
            strncpy(l2, q.question+28, 28); l2[28]='\0';
            DisplayEngine::clear(0x0008);
            DisplayEngine::buf.fillRect(0, 0, TFT_W, 28, COL_VIOLET);
            snprintf(hdr, sizeof(hdr), "[ %s ]", domainLabel);
            DisplayEngine::drawTextCentered(7, hdr, COL_GOLD, 1);
            DisplayEngine::drawText(5, 14, attBuf, COL_GREY, 1);
            DisplayEngine::drawProgressBar(5, 28, TFT_W-10, 5, pct, barCol, COL_GREY_DARK);
            DisplayEngine::drawText(5, 42, l1, COL_WHITE, 1);
            DisplayEngine::drawText(5, 56, l2, COL_WHITE, 1);
        } else {
            char l1[29], l2[29], l3[29];
            strncpy(l1, q.question, 28); l1[28]='\0';
            strncpy(l2, q.question+28, 28); l2[28]='\0';
            strncpy(l3, q.question+56, 28); l3[28]='\0';
            DisplayEngine::clear(0x0008);
            DisplayEngine::buf.fillRect(0, 0, TFT_W, 28, COL_VIOLET);
            snprintf(hdr, sizeof(hdr), "[ %s ]", domainLabel);
            DisplayEngine::drawTextCentered(7, hdr, COL_GOLD, 1);
            DisplayEngine::drawText(5, 14, attBuf, COL_GREY, 1);
            DisplayEngine::drawProgressBar(5, 28, TFT_W-10, 5, pct, barCol, COL_GREY_DARK);
            DisplayEngine::drawText(5, 42, l1, COL_WHITE, 1);
            DisplayEngine::drawText(5, 56, l2, COL_WHITE, 1);
            DisplayEngine::drawText(5, 70, l3, COL_WHITE, 1);
        }
    }

    // ── Choix de reponse ─────────────────────────────────────
    int startY = 150;
    int btnH   = (TFT_H - startY - 10) / q.numChoices;

    for (uint8_t i = 0; i < q.numChoices; i++) {
        bool isSel = (i == (uint8_t)highlighted);
        bool isAns = (answered >= 0 && i == (uint8_t)answered);
        bool isCorrect = (i == q.correctIdx);

        uint16_t bgCol, txtCol, borderCol;

        if (answered >= 0) {
            // Resultat affiche
            if (isCorrect) {
                bgCol = COL_TERMINAL; txtCol = COL_BLACK; borderCol = COL_TERMINAL;
            } else if (isAns && !correct) {
                bgCol = COL_DANGER; txtCol = COL_WHITE; borderCol = COL_DANGER;
            } else {
                bgCol = COL_GREY_DARK; txtCol = COL_GREY; borderCol = COL_GREY_DARK;
            }
        } else {
            bgCol    = isSel ? COL_GOLD     : COL_GREY_DARK;
            txtCol   = isSel ? COL_BLACK    : COL_WHITE;
            borderCol= isSel ? COL_GOLD     : COL_GREY;
        }

        int y = startY + i * (btnH + 2);
        DisplayEngine::buf.fillRoundRect(5, y, TFT_W - 10, btnH - 2, 5, bgCol);
        DisplayEngine::buf.drawRoundRect(5, y, TFT_W - 10, btnH - 2, 5, borderCol);

        // Numero de choix
        char numBuf[4];
        snprintf(numBuf, sizeof(numBuf), "%c.", 'A' + i);
        DisplayEngine::drawText(12, y + (btnH-10)/2, numBuf, txtCol, 1);
        // Texte (tronque)
        char choiceBuf[30];
        snprintf(choiceBuf, sizeof(choiceBuf), "%.26s", q.choices[i]);
        DisplayEngine::drawText(28, y + (btnH-10)/2, choiceBuf, txtCol, 1);
    }

    DisplayEngine::flush();
}

// ── Affichage recompense ──────────────────────────────────────
static void _showReward(uint8_t level) {
    // Fond dramatique
    RGBEngine::setMode(RGBMode::RAINBOW);
    BuzzerEngine::play(SFX::INTERLUDE_FANFARE);

    DisplayEngine::clear();
    DisplayEngine::buf.fillRect(0, 0, TFT_W, TFT_H, 0x0010);

    // Encadre dore
    DisplayEngine::buf.drawRoundRect(8, 60, TFT_W - 16, 180, 12, COL_GOLD);
    DisplayEngine::buf.drawRoundRect(10, 62, TFT_W - 20, 176, 10, COL_GOLD);

    DisplayEngine::drawTextCentered(80,  "*** FELICITATIONS ***", COL_GOLD, 1);
    DisplayEngine::drawTextCentered(105, REWARD_TITLES[level - 1], COL_TERMINAL, 1);
    DisplayEngine::buf.drawFastHLine(20, 125, TFT_W - 40, COL_GOLD);
    DisplayEngine::drawTextCentered(140, REWARD_SUBS[level - 1], COL_WHITE, 1);
    DisplayEngine::drawTextCentered(175, "EPREUVE INTELLECTUELLE", COL_GOLD, 1);
    DisplayEngine::drawTextCentered(195, "EN COURS DE CHARGEMENT...", COL_GREY, 1);

    // Compte a rebours visuel
    for (int i = 3; i > 0; i--) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", i);
        // Effacer la zone du compteur
        DisplayEngine::buf.fillRect(TFT_W/2 - 20, 228, 40, 24, 0x0010);
        DisplayEngine::drawTextCentered(232, buf, COL_DANGER, 2);
        DisplayEngine::flush();
        BuzzerEngine::play(SFX::QUIZ_TICK);
        vTaskDelay(pdMS_TO_TICKS(900));
    }

    DisplayEngine::fadeToBlack(300);
    DisplayEngine::breathPause(400);
    BuzzerEngine::play(SFX::QUIZ_INTRO);
    vTaskDelay(pdMS_TO_TICKS(1200));
}

// ── Main run ─────────────────────────────────────────────────
void StateInterlude::run(uint8_t levelJustCompleted, GameProfile profile) {
    // 1. Ecran de recompense
    _showReward(levelJustCompleted);

    // 2. Charger une question
    QuizDomain domain = LEVEL_DOMAIN[levelJustCompleted - 1];
    QuizQuestion q = QuizEngine::loadQuestion(domain, profile);

    if (!q.valid) {
        // Si le chargement echoue, on passe direct
        Serial.println("[INTERLUDE] Chargement question echoue, passage direct");
        return;
    }

    // 3. Notifier l'app web
    EscapeWebServer::notifyQuizQuestion(q, QuizEngine::domainLabel(domain));

    // 4. Boucle de quiz
    uint32_t limitMs = QuizEngine::timeLimitMs(profile, q.difficulty);
    uint32_t startMs = millis();
    int8_t   cursor  = 0;
    uint8_t  attempts = 0;
    bool     answered = false;
    bool     correct  = false;
    bool     timeOut  = false;

    RGBEngine::setMode(RGBMode::BREATHING);
    RGBEngine::setColor(60, 0, 120);

    while (!answered && attempts < QUIZ_MAX_ATTEMPTS) {
        uint32_t remaining = 0;
        uint32_t now = millis();

        if (now - startMs < limitMs) {
            remaining = limitMs - (now - startMs);
        } else {
            timeOut = true;
        }

        // Timer tick sonore dans les 5 dernieres secondes
        if (remaining < 5000 && remaining > 0 && (remaining % 1000 < 50)) {
            BuzzerEngine::play(SFX::QUIZ_TICK);
            RGBEngine::setMode(RGBMode::ALERT_RED);
        }

        _drawQuestion(q, cursor, -1, false, remaining, attempts + 1,
                      QuizEngine::domainLabel(domain));

        if (timeOut) {
            // Temps ecoule = compte comme mauvaise reponse
            BuzzerEngine::play(SFX::QUIZ_FAIL);
            RGBEngine::setMode(RGBMode::ALERT_RED);
            _drawQuestion(q, cursor, cursor, false, 0, attempts + 1,
                          QuizEngine::domainLabel(domain));
            vTaskDelay(pdMS_TO_TICKS(QUIZ_TRANSITION_MS));
            attempts++;
            startMs = millis();
            timeOut = false;
            if (attempts >= QUIZ_MAX_ATTEMPTS) break;
            continue;
        }

        // Navigation joystick
        static uint32_t lastNav = 0;
        auto joy = JoystickReader::read();
        if (millis() - lastNav > 150) {
            if (joy.dir == JoyDir::UP   && cursor > 0) {
                cursor--;
                BuzzerEngine::play(SFX::HAPTICK);
                lastNav = millis();
            }
            if (joy.dir == JoyDir::DOWN && cursor < (int8_t)(q.numChoices - 1)) {
                cursor++;
                BuzzerEngine::play(SFX::HAPTICK);
                lastNav = millis();
            }
        }

        // Confirmation (bouton joystick)
        if (joy.pressed) {
            attempts++;
            correct = (cursor == q.correctIdx);

            if (correct) {
                BuzzerEngine::play(SFX::QUIZ_WIN);
                RGBEngine::setMode(RGBMode::RAINBOW);
            } else {
                BuzzerEngine::play(SFX::QUIZ_FAIL);
                RGBEngine::setMode(RGBMode::ALERT_RED);
            }

            _drawQuestion(q, cursor, cursor, correct, remaining, attempts,
                          QuizEngine::domainLabel(domain));

            // Notifier l'app web
            EscapeWebServer::notifyQuizResult(cursor == q.correctIdx, q.correctIdx);

            vTaskDelay(pdMS_TO_TICKS(QUIZ_TRANSITION_MS));

            if (correct) {
                answered = true;
                break;
            }

            // Mauvaise reponse : autre tentative si dispo
            if (attempts < QUIZ_MAX_ATTEMPTS) {
                DisplayEngine::clear();
                DisplayEngine::drawTextCentered(130, "INCORRECT !", COL_DANGER, 2);
                char retryBuf[30];
                snprintf(retryBuf, sizeof(retryBuf), "Encore %d essai(s)",
                         QUIZ_MAX_ATTEMPTS - attempts);
                DisplayEngine::drawTextCentered(165, retryBuf, COL_GOLD, 1);
                DisplayEngine::flush();
                vTaskDelay(pdMS_TO_TICKS(1500));
                startMs = millis();  // Reset timer
                RGBEngine::setMode(RGBMode::BREATHING);
                RGBEngine::setColor(60, 0, 120);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(16));
    }

    // 5. Ecran de conclusion
    DisplayEngine::clear();
    if (correct) {
        // Bonne reponse
        BuzzerEngine::play(SFX::INTERLUDE_UNLOCK);
        RGBEngine::setMode(RGBMode::ALERT_GREEN);
        DisplayEngine::drawRoundBox(10, 90, TFT_W - 20, 140, 10, COL_TERMINAL, true);
        DisplayEngine::drawTextCentered(105, "EXACT !", COL_BLACK, 3);
        DisplayEngine::buf.drawFastHLine(20, 135, TFT_W - 40, COL_BLACK);
        if (strlen(q.explanation) > 0) {
            // Afficher l'explication
            DisplayEngine::drawText(15, 148, q.explanation, COL_BLACK, 1);
        }
        DisplayEngine::drawTextCentered(220, "ACCES NIVEAU SUIVANT", COL_GOLD, 1);
        DisplayEngine::drawTextCentered(240, "DEBLOQUE", COL_GOLD, 1);
    } else {
        // Echec / tentatives epuisees → revelation
        BuzzerEngine::play(SFX::QUIZ_REVEAL);
        RGBEngine::setMode(RGBMode::BREATHING);
        RGBEngine::setColor(40, 0, 80);
        DisplayEngine::drawTextCentered(90, "REPONSE CORRECTE :", COL_GREY, 1);
        DisplayEngine::drawTextCentered(115, q.choices[q.correctIdx], COL_TERMINAL, 1);
        DisplayEngine::buf.drawFastHLine(10, 140, TFT_W - 20, COL_GREY_DARK);
        if (strlen(q.explanation) > 0) {
            DisplayEngine::drawText(5, 155, q.explanation, COL_WHITE, 1);
        }
        DisplayEngine::drawTextCentered(240, "Acces accorde malgre tout", COL_GREY, 1);
    }
    DisplayEngine::flush();
    vTaskDelay(pdMS_TO_TICKS(3000));
    DisplayEngine::fadeToBlack(400);
    DisplayEngine::breathPause(600);
    RGBEngine::setMode(RGBMode::OFF);
}
