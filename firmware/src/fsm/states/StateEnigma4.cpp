// ============================================================
// Énigme 4 — "La Sentinelle"
// HC-SR04 : séquence de gestes à reproduire
// TFT : radar animé en temps réel
// Web App : séquence affichée 8 secondes puis disparaît
// ============================================================
#include "StateEnigma4.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/SonarReader.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;

static const TutStep SENT_TUT[] = {
    {
        "APPROCHE TA MAIN",
        "Du capteur en face de toi",
        TutInputType::SONAR_NEAR, ArrowDir::DOWN, 0, TFT_W/2, 130
    },
    {
        "MAINTIENS LA DISTANCE",
        "Garde ta main stable 2s",
        TutInputType::SONAR_HOLD, ArrowDir::DOWN, 0, TFT_W/2, 130
    },
    {
        "MEMORISE LA SEQUENCE",
        "Elle disparait apres 8s !",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2000, TFT_W/2, 130
    }
};

static const char* GEST_NAMES[] = {
    "APPROCHE LENTE", "MAINTIEN 3s", "VAGUE RAPIDE", "RECUL RAPIDE"
};

const char* StateEnigma4::_gestName(SentinelleGest g) {
    return GEST_NAMES[(int)g];
}

void StateEnigma4::_generateSequence(uint32_t seed) {
    srand(seed);
    SentinelleGest pool[] = {
        SentinelleGest::SLOW_APPROACH,
        SentinelleGest::HOLD,
        SentinelleGest::FAST_WAVE,
        SentinelleGest::RETREAT
    };
    for (int i = 0; i < _seqLen; ++i)
        _sequence[i] = pool[rand() % 4];
}

bool StateEnigma4::_detectGesture(float distCm, SentinelleGest expected) {
    switch (expected) {
        case SentinelleGest::SLOW_APPROACH:
            if (distCm < 40.0f && !_inApproach) {
                _inApproach = true;
                _approachStart = millis();
            }
            if (_inApproach && distCm < 20.0f && (millis() - _approachStart) > 1500)
                return true;
            if (_inApproach && distCm >= 40.0f) _inApproach = false;
            break;

        case SentinelleGest::HOLD:
            if (distCm >= 8.0f && distCm <= 20.0f) {
                if (!_inHold) { _inHold = true; _holdStart = millis(); }
                if (millis() - _holdStart > 3000) return true;
            } else {
                _inHold = false;
            }
            break;

        case SentinelleGest::FAST_WAVE:
            return SonarReader::detectWave(25.0f);

        case SentinelleGest::RETREAT:
            if (distCm > 50.0f && !_inApproach) return true;
            break;
    }
    return false;
}

void StateEnigma4::_drawScene(float distCm) {
    DisplayEngine::clear(0x0008);

    // Titre
    DisplayEngine::drawRoundBox(0, 0, TFT_W, 24, 0, COL_GREY_DARK, true);
    DisplayEngine::drawTextCentered(6, "SENTINELLE", COL_DANGER, 1);

    // ── Radar ───────────────────────────────────────────────
    DisplayEngine::drawRadar(TFT_W/2, 120, 80, distCm, 15.0f);

    // Distance textuelle
    char dbuf[20];
    snprintf(dbuf, sizeof(dbuf), "%.1f cm", distCm);
    DisplayEngine::drawTextCentered(210, dbuf, COL_TERMINAL, 1);

    // ── Progression gestes ──────────────────────────────────
    DisplayEngine::drawRoundBox(5, 228, TFT_W - 10, 50, 6, COL_GREY_DARK, true);
    char progBuf[32];
    snprintf(progBuf, sizeof(progBuf), "GESTE %d/%d", _seqIndex + 1, _seqLen);
    DisplayEngine::drawText(12, 234, progBuf, COL_GREY, 1);
    DisplayEngine::drawText(12, 252,
        _gestName(_sequence[_seqIndex]), COL_GOLD, 1);

    // Indicateurs de progression
    for (int i = 0; i < _seqLen; ++i) {
        uint32_t col = (i < _seqIndex)  ? COL_TERMINAL :
                       (i == _seqIndex) ? COL_GOLD     : COL_GREY_DARK;
        DisplayEngine::buf.fillCircle(TFT_W/2 - _seqLen*8 + i*16 + 8, 244, 5, col);
    }

    // Compte à rebours si expert
    if (_profile == GameProfile::EXPERT) {
        uint32_t rem = (_timeLimit > elapsed()) ? (_timeLimit - elapsed()) / 1000 : 0;
        char tb[8]; snprintf(tb, sizeof(tb), "%02lus", rem);
        DisplayEngine::drawText(TFT_W - 32, 6, tb, rem < 20 ? COL_DANGER : COL_WHITE, 1);
    }
}

void StateEnigma4::onEnter(GameProfile p) {
    _profile   = p;
    _enterTime = millis();
    _tutDone   = false;
    _seqIndex  = 0;
    _inApproach= false;
    _inHold    = false;
    _showSeq   = true;

    switch (p) {
        case GameProfile::PUBLIC:   _seqLen = 2; _timeLimit = UINT32_MAX; break;
        case GameProfile::ADVANCED: _seqLen = 3; _timeLimit = UINT32_MAX; break;
        case GameProfile::EXPERT:   _seqLen = 4; _timeLimit = 180000; break;
    }

    DateTime now = rtc.now();
    _generateSequence(now.unixtime() ^ 0xBEEF);
    _showEnd = millis() + 8000;  // Séquence visible 8 secondes sur Web App

    DisplayEngine::clear();
    DisplayEngine::drawTextCentered(110, "PROTOCOLE 04", COL_GOLD, 2);
    DisplayEngine::drawTextCentered(145, "LA SENTINELLE", COL_WHITE, 2);
    DisplayEngine::flush();
    BuzzerEngine::play(SFX::LEVEL_UP);
    RGBEngine::setMode(RGBMode::ALERT_RED);
    vTaskDelay(pdMS_TO_TICKS(2500));
    DisplayEngine::fadeToBlack(400);
    DisplayEngine::breathPause(700);

    _tut.begin(SENT_TUT, 3, "PROTOCOLE 04");
}

bool StateEnigma4::update() {
    if (!_tutDone) {
        _tutDone = _tut.update();
        if (_tutDone) {
            _enterTime = millis();
            _showEnd = millis() + 8000;  // Reset affichage séquence
            RGBEngine::setMode(RGBMode::BREATHING);
            RGBEngine::setColor(150, 0, 0);
        }
        return false;
    }

    if (_profile == GameProfile::EXPERT && timedOut(_timeLimit)) {
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(130, "SENTINELLE ALERTE!", COL_DANGER, 2);
        DisplayEngine::flush();
        BuzzerEngine::play(SFX::ALARM);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return false;
    }

    float dist = SonarReader::readCm();
    _drawScene(dist);
    DisplayEngine::flush();

    // Détection du geste courant
    if (_detectGesture(dist, _sequence[_seqIndex])) {
        BuzzerEngine::play(SFX::CONFIRM);
        RGBEngine::setMode(RGBMode::PULSE);
        RGBEngine::setColor(0, 200, 0);
        _inApproach = false;
        _inHold     = false;
        _seqIndex++;

        if (_seqIndex >= _seqLen) {
            // Tous les gestes validés
            BuzzerEngine::play(SFX::SUCCESS);
            RGBEngine::setMode(RGBMode::RAINBOW);
            DisplayEngine::clear();
            DisplayEngine::drawTextCentered(110, "SENTINELLE", COL_TERMINAL, 2);
            DisplayEngine::drawTextCentered(145, "DESACTIVEE !", COL_GOLD, 2);
            DisplayEngine::flush();
            vTaskDelay(pdMS_TO_TICKS(2000));
            DisplayEngine::fadeToBlack(500);
            DisplayEngine::breathPause(800);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return false;
}
