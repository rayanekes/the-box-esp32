#pragma once
// ============================================================
// fsm/states/StateIntro.h
// ============================================================
#include "../../config.h"

class StateIntro {
public:
    // Retourne le profil ET le mode choisis
    static IntroResult run();
};
