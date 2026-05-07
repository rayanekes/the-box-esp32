#pragma once
// Énigme 4 — "La Sentinelle" (HC-SR04 Gestuel + Web App)
#include "../IEnigma.h"
#include "../TutorialEngine.h"

enum class SentinelleGest : uint8_t { SLOW_APPROACH, HOLD, FAST_WAVE, RETREAT };

class StateEnigma4 : public IEnigma {
public:
    void  onEnter(GameProfile p) override;
    bool  update() override;
    const char* name()   const override { return "La Sentinelle"; }
    uint8_t     number() const override { return 4; }

private:
    TutorialEngine _tut;
    bool  _tutDone = false;

    // Séquence de gestes à reproduire
    static const int MAX_GESTURES = 4;
    SentinelleGest _sequence[MAX_GESTURES];
    int    _seqLen     = 3;
    int    _seqIndex   = 0;  // Geste courant à valider

    bool   _showSeq    = true;
    uint32_t _showEnd  = 0;

    // États de détection des gestes
    uint32_t _approachStart = 0;
    uint32_t _holdStart     = 0;
    bool     _inApproach    = false;
    bool     _inHold        = false;

    void _generateSequence(uint32_t seed);
    void _drawScene(float distCm);
    bool _detectGesture(float distCm, SentinelleGest expected);
    const char* _gestName(SentinelleGest g);
};
