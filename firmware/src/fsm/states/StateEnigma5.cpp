// ============================================================
// Énigme 5 — "La Libération" (Boss Final)
// 3 verrous simultanés : Potentiomètre + HC-SR04 + Code clavier
// Servo + Particules + Rainbow = victoire épique
// ============================================================
#include "StateEnigma5.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/PotReader.h"
#include "../../sensors/SonarReader.h"
#include "../../sensors/ServoController.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;
extern Keypad     keypad;

static const TutStep LIB_TUT[] = {
    {
        "LE MOMENT DE VERITE",
        "3 verrous simultanement",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2500, TFT_W/2, 120
    },
    {
        "POTENTIOMETRE : zone cible",
        "Le buzzer clique dessus",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2000, TFT_W/2, 130
    },
    {
        "SONAR : main a 12cm",
        "Stable pendant 3 secondes",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2000, TFT_W/2, 130
    },
    {
        "CODE : sur le clavier",
        "Recupere-le sur l'App",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2000, TFT_W/2, 130
    },
    {
        "LES 3 EN MEME TEMPS",
        "Bonne chance, Agent.",
        TutInputType::AUTO_DONE, ArrowDir::UP, 3000, TFT_W/2, 130
    }
};

void StateEnigma5::_generateMission(uint32_t seed) {
    srand(seed);
    _requiredLen = (_profile == GameProfile::EXPERT) ? 6 : 4;
    for (int i = 0; i < _requiredLen; ++i)
        _correctCode[i] = '0' + (rand() % 10);
    _correctCode[_requiredLen] = '\0';

    _targetPot  = 500 + (rand() % 3000);  // Zone aléatoire
    _targetDist = 8.0f + (rand() % 8);     // 8–16cm

    switch (_profile) {
        case GameProfile::PUBLIC:
            _potTol = 250; _distTol = 5.0f; _holdRequired = 2000; _timeLimit = 120000; break;
        case GameProfile::ADVANCED:
            _potTol = 150; _distTol = 3.0f; _holdRequired = 3000; _timeLimit = 60000; break;
        case GameProfile::EXPERT:
            _potTol = 80;  _distTol = 2.0f; _holdRequired = 3000; _timeLimit = 45000; break;
    }
}

bool StateEnigma5::_drawLockIndicator(int x, int y, int w, int h,
                                       const char* label, bool locked) {
    uint32_t borderCol = locked ? COL_TERMINAL : COL_GREY_DARK;
    uint32_t fillCol   = locked ? 0x0280 : 0x0820;
    DisplayEngine::drawRoundBox(x, y, w, h, 6, fillCol, true);
    DisplayEngine::drawRoundBox(x, y, w, h, 6, borderCol);
    DisplayEngine::drawText(x + 6, y + 6, label, locked ? COL_TERMINAL : COL_GREY, 1);

    if (locked) {
        // Icône cadenas ouvert
        DisplayEngine::buf.fillCircle(x + w - 16, y + h/2, 5, COL_TERMINAL);
        DisplayEngine::buf.drawCircle(x + w - 16, y + h/2, 7, COL_GOLD);
    } else {
        // Icône cadenas fermé
        DisplayEngine::buf.fillRect(x + w - 22, y + 5, 14, 10, COL_GREY_DARK);
        DisplayEngine::buf.drawRect(x + w - 22, y + 5, 14, 10, COL_GREY);
    }
    return locked;
}

void StateEnigma5::_drawScene() {
    DisplayEngine::clear(0x0008);

    // ── Timer ───────────────────────────────────────────────
    uint32_t rem = (_timeLimit > elapsed()) ? (_timeLimit - elapsed()) / 1000 : 0;
    bool danger = rem < 15;
    char tb[20]; snprintf(tb, sizeof(tb), "%02lus", rem);
    DisplayEngine::drawRoundBox(0, 0, TFT_W, 26, 0, danger ? 0x2000 : COL_GREY_DARK, true);
    DisplayEngine::drawTextCentered(6, "LIBERATION FINALE", COL_GOLD, 1);
    DisplayEngine::drawText(TFT_W - 28, 6, tb, danger ? COL_DANGER : COL_WHITE, 1);

    // Barre de timer
    DisplayEngine::drawProgressBar(0, 24, TFT_W, 5,
        (float)rem / (_timeLimit / 1000.0f), COL_DANGER, 0x2000);

    // ── 3 Indicateurs de verrou ──────────────────────────────
    _lockPot   = PotReader::inZone(_targetPot, _potTol);
    float dist = SonarReader::readCm();
    _lockSonar = (dist >= _targetDist - _distTol && dist <= _targetDist + _distTol);
    _lockCode  = (_codeLen >= _requiredLen);
    for (int i = 0; i < _requiredLen; ++i)
        if (_lockCode) _lockCode = (_enteredCode[i] == _correctCode[i]);

    _drawLockIndicator(5,  36, TFT_W - 10, 42, "POTENTIOMETRE", _lockPot);
    _drawLockIndicator(5,  85, TFT_W - 10, 42, "CAPTEUR SONAR", _lockSonar);
    _drawLockIndicator(5, 134, TFT_W - 10, 42, "CODE D'ACCES",  _lockCode);

    // Valeurs en temps réel
    char potBuf[20];
    float potPct = (float)abs((int)PotReader::readSmooth() - (int)_targetPot) / _potTol;
    snprintf(potBuf, sizeof(potBuf), "%.0f%%", max(0.0f, (1.0f - potPct) * 100));
    DisplayEngine::drawText(10, 52, potBuf, _lockPot ? COL_TERMINAL : COL_GREY, 1);

    char distBuf[20];
    snprintf(distBuf, sizeof(distBuf), "%.1fcm / cible %.0fcm", dist, _targetDist);
    DisplayEngine::drawText(10, 101, distBuf, _lockSonar ? COL_TERMINAL : COL_GREY, 1);

    // Affichage code saisi
    char codeBuf[14] = "";
    for (int i = 0; i < _requiredLen; ++i) {
        codeBuf[i*2]   = (i < _codeLen) ? _enteredCode[i] : '_';
        codeBuf[i*2+1] = ' ';
    }
    DisplayEngine::drawText(10, 150, codeBuf, _lockCode ? COL_TERMINAL : COL_GOLD, 1);

    // ── Indicateur "HOLD" si tous verrouillés ───────────────
    if (_lockPot && _lockSonar && _lockCode) {
        if (!_allLocked) {
            _allLocked    = true;
            _allLockStart = millis();
        }
        uint32_t holdMs = millis() - _allLockStart;
        float    pct    = (float)holdMs / _holdRequired;
        DisplayEngine::drawRoundBox(5, 185, TFT_W - 10, 50, 8, 0x0380, true);
        DisplayEngine::drawTextCentered(192, "MAINTENEZ !", COL_TERMINAL, 2);
        DisplayEngine::drawProgressBar(15, 220, TFT_W - 30, 10, pct, COL_GOLD, COL_GREY_DARK);

        // Buzz heartbeat
        static uint32_t lastHb = 0;
        if (millis() - lastHb > 1000) {
            BuzzerEngine::play(SFX::HEARTBEAT);
            lastHb = millis();
        }
    } else {
        _allLocked = false;
        DisplayEngine::drawRoundBox(5, 185, TFT_W - 10, 55, 8, 0x2000, true);
        DisplayEngine::drawTextCentered(195, "SYNCHRONISEZ LES", COL_GREY, 1);
        DisplayEngine::drawTextCentered(210, "3 VERROUS !", COL_WHITE, 2);
    }
}

void StateEnigma5::onEnter(GameProfile p) {
    _profile   = p;
    _enterTime = millis();
    _tutDone   = false;
    _codeLen   = 0;
    _allLocked = false;
    _lockPot = _lockSonar = _lockCode = false;

    DateTime now = rtc.now();
    _generateMission(now.unixtime() ^ 0xCAFE);

    // Annonce boss
    DisplayEngine::clear();
    DisplayEngine::drawTextCentered(90,  "PROTOCOLE FINAL", COL_DANGER, 1);
    DisplayEngine::drawTextCentered(115, "LA", COL_GOLD, 3);
    DisplayEngine::drawTextCentered(160, "LIBERATION", COL_GOLD, 2);
    DisplayEngine::flush();
    BuzzerEngine::play(SFX::ALARM);
    RGBEngine::setMode(RGBMode::HEARTBEAT);
    vTaskDelay(pdMS_TO_TICKS(3000));
    DisplayEngine::fadeToBlack(600);
    DisplayEngine::breathPause(1000);

    _tut.begin(LIB_TUT, 5, "PROTOCOLE FINAL");
}

bool StateEnigma5::update() {
    if (!_tutDone) {
        _tutDone = _tut.update();
        if (_tutDone) {
            _enterTime = millis();
            RGBEngine::setMode(RGBMode::ALERT_RED);
        }
        return false;
    }

    // Timeout
    if (timedOut(_timeLimit)) {
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(100, "EFFACEMENT...", COL_DANGER, 2);
        DisplayEngine::drawTextCentered(135, "PROTOCOLES REINITIALISES", COL_WHITE, 1);
        DisplayEngine::flush();
        BuzzerEngine::play(SFX::ALARM);
        RGBEngine::setMode(RGBMode::ALERT_RED);
        vTaskDelay(pdMS_TO_TICKS(3000));
        DisplayEngine::fadeToBlack(500);
        return false;
    }

    // Lecture clavier
    char key = keypad.getKey();
    if (key && key >= '0' && key <= '9' && _codeLen < _requiredLen) {
        _enteredCode[_codeLen++] = key;
        BuzzerEngine::play(SFX::CLICK);
    }
    if (key == '*' && _codeLen > 0) {
        _codeLen--;
        BuzzerEngine::play(SFX::ERROR);
    }

    _drawScene();
    DisplayEngine::flush();

    // Validation finale
    if (_allLocked && (millis() - _allLockStart) >= _holdRequired) {
        return true;
    }
    return false;
}
