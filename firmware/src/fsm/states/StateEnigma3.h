#pragma once
// Énigme 3 — "Le Spectre Radio" (Potentiomètre + Clavier + Web App)
#include "../IEnigma.h"
#include "../TutorialEngine.h"
#include <Keypad.h>

class StateEnigma3 : public IEnigma {
public:
    void  onEnter(GameProfile p) override;
    bool  update() override;
    const char* name()   const override { return "Spectre Radio"; }
    uint8_t     number() const override { return 3; }

private:
    TutorialEngine _tut;
    bool _tutDone    = false;
    bool _locked     = false;  // Fréquence verrouillée ?
    uint32_t _lockStart = 0;
    uint16_t _targetFreq = 0;
    uint16_t _targetPot  = 0;
    uint16_t _potTol     = 0;

    char _enteredCode[8] = {};
    uint8_t _codeLen = 0;
    uint8_t _requiredLen = 4;
    char _correctCode[8] = {};

    float _specBands[12] = {};
    float _specT = 0.0f;

    void _generateMission(uint32_t seed);
    void _drawScene();
    bool _checkCode();
};
