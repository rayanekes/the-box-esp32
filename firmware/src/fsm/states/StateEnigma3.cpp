// ============================================================
// Énigme 3 — "Le Spectre Radio"
// Potentiomètre (trouver la freq) + Clavier (déchiffrer le code)
// Oscilloscope en temps réel sur TFT, table de décodage sur Web App
// ============================================================
#include "StateEnigma3.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/PotReader.h"
#include <RTClib.h>
#include <math.h>

extern RTC_DS3231 rtc;
extern Keypad     keypad;

// Décalage César selon profil
static int _caesarShift = 3;

static const TutStep RADIO_TUT[] = {
    {
        "SYNTONISE LA FREQUENCE",
        "Tourne le potentiometre",
        TutInputType::POT_TURN_RIGHT, ArrowDir::RIGHT, 0, TFT_W/2, 140
    },
    {
        "SENS LE 'CLIC' HAPTIQUE",
        "Le buzzer indique la zone cible",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2500, TFT_W/2, 140
    },
    {
        "DECHIFFRE ET TAPE LE CODE",
        "4 chiffres sur le clavier",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2000, TFT_W/2, 130
    }
};

void StateEnigma3::_generateMission(uint32_t seed) {
    srand(seed);
    // Code à 4 chiffres (ou 6 en expert)
    _requiredLen = (_profile == GameProfile::EXPERT) ? 6 : 4;
    for (int i = 0; i < _requiredLen; ++i)
        _correctCode[i] = '0' + (rand() % 10);
    _correctCode[_requiredLen] = '\0';

    // Fréquence cible 500–3500 Hz mappée sur 0–4095 ADC
    _targetFreq = 500 + (rand() % 3000);
    _targetPot  = (uint16_t)((float)_targetFreq / 3500.0f * 4095);
    _potTol     = (_profile == GameProfile::EXPERT) ? 80 : 200;
}

bool StateEnigma3::_checkCode() {
    if (_codeLen < _requiredLen) return false;
    for (int i = 0; i < _requiredLen; ++i)
        if (_enteredCode[i] != _correctCode[i]) return false;
    return true;
}

void StateEnigma3::_drawScene() {
    DisplayEngine::clear(0x0008);

    // ── Oscilloscope (haut) ──────────────────────────────────
    _specT += 0.05f;
    float potNorm = PotReader::readNorm();
    float freqVal = 0.5f + potNorm * 4.5f;
    DisplayEngine::drawOscilloscope(5, 40, TFT_W - 10, 80, freqVal, _specT);

    // Indicateur fréquence
    char freqBuf[24];
    snprintf(freqBuf, sizeof(freqBuf), "FREQ: %4.0f Hz",
             potNorm * 3500.0f + 500.0f);
    DisplayEngine::drawText(5, 130, freqBuf, COL_TERMINAL, 1);

    // Indicateur "VERROUILLE" si dans la zone
    bool inZone = PotReader::inZone(_targetPot, _potTol);
    if (inZone) {
        if (!_locked) { _locked = true; _lockStart = millis(); }
        DisplayEngine::drawRoundBox(5, 125, TFT_W - 10, 18, 4, 0x0380, true);
        DisplayEngine::drawTextCentered(128, "[ SIGNAL VERROUILLE ]", COL_TERMINAL, 1);
    } else {
        _locked = false;
    }

    // ── Zone de saisie du code ───────────────────────────────
    DisplayEngine::drawRoundBox(5, 150, TFT_W - 10, 50, 6, COL_GREY_DARK, true);
    DisplayEngine::drawTextCentered(158, "CODE D'ACCES", COL_GREY, 1);

    // Affichage des caractères entrés
    char displayCode[16] = "";
    for (int i = 0; i < _requiredLen; ++i) {
        if (i < _codeLen)
            displayCode[i*2] = _enteredCode[i];
        else
            displayCode[i*2] = '_';
        displayCode[i*2+1] = ' ';
    }
    displayCode[_requiredLen * 2] = '\0';
    DisplayEngine::drawTextCentered(173, displayCode, COL_GOLD, 2);

    // ── Clavier visuel ───────────────────────────────────────
    DisplayEngine::drawText(5, 210, "DECHIFFRE VIA L'APP WEB", COL_GREY, 1);
    DisplayEngine::drawText(5, 225, "PUIS ENTRE LE CODE :", COL_WHITE, 1);

    // Barre de progression de verrouillage
    if (_locked) {
        uint32_t holdTime = min((uint32_t)2000, millis() - _lockStart);
        DisplayEngine::drawProgressBar(5, 245, TFT_W - 10, 6,
            (float)holdTime / 2000.0f, COL_TERMINAL, COL_GREY_DARK);
    }

    // Timer (experts seulement)
    if (_profile == GameProfile::EXPERT) {
        uint32_t rem = (_timeLimit > elapsed()) ? (_timeLimit - elapsed()) / 1000 : 0;
        char tb[16]; snprintf(tb, sizeof(tb), "%02lus", rem);
        DisplayEngine::drawText(TFT_W - 28, 5, tb,
            rem < 30 ? COL_DANGER : COL_GOLD, 1);
    }
}

void StateEnigma3::onEnter(GameProfile p) {
    _profile   = p;
    _enterTime = millis();
    _tutDone   = false;
    _locked    = false;
    _codeLen   = 0;
    _specT     = 0.0f;

    switch (p) {
        case GameProfile::PUBLIC:   _caesarShift = 0; _timeLimit = UINT32_MAX; break;
        case GameProfile::ADVANCED: _caesarShift = 3; _timeLimit = UINT32_MAX; break;
        case GameProfile::EXPERT:   _caesarShift = 5; _timeLimit = 240000; break;
    }

    DateTime now = rtc.now();
    _generateMission(now.unixtime() ^ 0xDEAD);

    DisplayEngine::clear();
    DisplayEngine::drawTextCentered(110, "PROTOCOLE 03", COL_GOLD, 2);
    DisplayEngine::drawTextCentered(145, "LE SPECTRE", COL_WHITE, 2);
    DisplayEngine::drawTextCentered(175, "RADIO", COL_WHITE, 2);
    DisplayEngine::flush();
    BuzzerEngine::play(SFX::LEVEL_UP);
    RGBEngine::setMode(RGBMode::BREATHING);
    RGBEngine::setColor(200, 80, 0);
    vTaskDelay(pdMS_TO_TICKS(2500));
    DisplayEngine::fadeToBlack(400);
    DisplayEngine::breathPause(700);

    _tut.begin(RADIO_TUT, 3, "PROTOCOLE 03");
}

bool StateEnigma3::update() {
    if (!_tutDone) {
        _tutDone = _tut.update();
        if (_tutDone) {
            _enterTime = millis();
            RGBEngine::setMode(RGBMode::BREATHING);
            RGBEngine::setColor(200, 60, 0);
        }
        return false;
    }

    if (_profile == GameProfile::EXPERT && timedOut(_timeLimit)) {
        // Timeout Expert
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(130, "SIGNAL PERDU !", COL_DANGER, 2);
        DisplayEngine::flush();
        BuzzerEngine::play(SFX::ALARM);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return false;
    }

    // Lecture clavier
    char key = keypad.getKey();
    if (key && key >= '0' && key <= '9' && _codeLen < _requiredLen) {
        _enteredCode[_codeLen++] = key;
        BuzzerEngine::play(SFX::CLICK);
    }
    if (key == '*') {  // Effacer
        if (_codeLen > 0) _codeLen--;
        BuzzerEngine::play(SFX::ERROR);
    }

    _drawScene();
    DisplayEngine::flush();

    // Validation : signal verrouillé ET bon code
    bool inZone = abs((int)PotReader::readSmooth() - (int)_targetPot) <= _potTol;
    if (inZone && _checkCode()) {
        BuzzerEngine::play(SFX::SUCCESS);
        RGBEngine::setMode(RGBMode::RAINBOW);
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(110, "FREQUENCE OK !", COL_TERMINAL, 2);
        DisplayEngine::drawTextCentered(145, "CODE VALIDE", COL_GOLD, 2);
        DisplayEngine::drawTextCentered(175, "Protocole 03 : OK", COL_WHITE, 1);
        DisplayEngine::flush();
        vTaskDelay(pdMS_TO_TICKS(2500));
        DisplayEngine::fadeToBlack(500);
        DisplayEngine::breathPause(800);
        return true;
    }
    return false;
}
