#pragma once
// ============================================================
// fsm/IEnigma.h — Interface pure virtuelle pour toutes les énigmes
// Chaque énigme est une classe indépendante héritant de IEnigma
// ============================================================
#include <Arduino.h>
#include "../config.h"

class IEnigma {
public:
    virtual ~IEnigma() = default;

    // Appelé une seule fois à l'entrée dans l'état
    virtual void onEnter(GameProfile profile) = 0;

    // Appelé à chaque cycle de la tâche game (~30fps)
    // Retourne true quand l'énigme est résolue
    virtual bool update() = 0;

    // Appelé à la sortie (libère les ressources si besoin)
    virtual void onExit() {}

    // Libellé pour les logs et la Web App
    virtual const char* name() const = 0;

    // Numéro 1–5
    virtual uint8_t number() const = 0;

protected:
    GameProfile _profile   = GameProfile::PUBLIC;
    uint32_t    _enterTime = 0;
    uint32_t    _timeLimit = UINT32_MAX;  // Limite de temps (ms). UINT32_MAX = illimite
    bool        _solved    = false;

    uint32_t elapsed() const { return millis() - _enterTime; }

    // Timer retourne true si le temps imparti est dépassé
    bool timedOut(uint32_t limitMs) const {
        return (millis() - _enterTime) > limitMs;
    }
};
