#pragma once
// Énigme 1 — "L'Académie des Agents" (Onboarding + Tutoriel pur)
#include "../IEnigma.h"
#include "../TutorialEngine.h"

class StateEnigma1 : public IEnigma {
public:
    void  onEnter(GameProfile p) override;
    bool  update() override;
    const char* name()   const override { return "L'Academie"; }
    uint8_t     number() const override { return 1; }

private:
    TutorialEngine _tut;
    bool _tutDone = false;
};
