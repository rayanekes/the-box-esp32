#pragma once
// Énigme 5 — "La Libération" (Boss Final — Tout simultanément)
#include "../IEnigma.h"
#include "../TutorialEngine.h"
#include <Keypad.h>

class StateEnigma5 : public IEnigma {
public:
    void  onEnter(GameProfile p) override;
    bool  update() override;
    const char* name()   const override { return "La Liberation"; }
    uint8_t     number() const override { return 5; }

private:
    TutorialEngine _tut;
    bool _tutDone = false;

    // Les 3 verrous
    bool _lockPot    = false;  // Potentiomètre sur la bonne valeur
    bool _lockSonar  = false;  // Sonar à la bonne distance
    bool _lockCode   = false;  // Bon code saisi

    // Code final
    char _correctCode[8] = {};
    char _enteredCode[8] = {};
    uint8_t _codeLen = 0;
    uint8_t _requiredLen = 4;

    // Valeurs cibles
    uint16_t _targetPot   = 0;
    uint16_t _potTol      = 150;
    float    _targetDist  = 12.0f;
    float    _distTol     = 3.0f;

    // Timing : les 3 verrous doivent être SIMULTANÉMENT vrais
    uint32_t _allLockStart = 0;
    bool     _allLocked    = false;
    uint32_t _holdRequired = 3000;  // ms

    uint32_t _timeLimit    = 45000;

    void _generateMission(uint32_t seed);
    void _drawScene();
    bool _drawLockIndicator(int x, int y, int w, int h,
                             const char* label, bool locked);
};
