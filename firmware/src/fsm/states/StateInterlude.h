#pragma once
// ============================================================
// fsm/states/StateInterlude.h
// Question quiz OBLIGATOIRE entre deux enigmes (recompense narrative)
// ============================================================
#include "../../config.h"
#include "../../quiz/QuizEngine.h"

class StateInterlude {
public:
    // Affiche la sequence de recompense + question quiz
    // levelJustCompleted : 1..4 (apres l'enigme 1, 2, 3 ou 4)
    // Bloque jusqu'a ce que le joueur ait repondu (ou epuise ses tentatives)
    static void run(uint8_t levelJustCompleted, GameProfile profile);
};
