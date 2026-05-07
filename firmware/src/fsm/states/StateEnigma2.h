#pragma once
// Énigme 2 — "Le Labyrinthe Cognitif" (Joystick + TFT + Web App)
#include "../IEnigma.h"
#include "../TutorialEngine.h"

class StateEnigma2 : public IEnigma {
public:
    void  onEnter(GameProfile p) override;
    bool  update() override;
    void  onExit() override;
    const char* name()   const override { return "Labyrinthe"; }
    uint8_t     number() const override { return 2; }

private:
    TutorialEngine _tut;
    bool _tutDone = false;

    // Labyrinthe 15×19 (cases) — 1=mur, 0=couloir
    static const int MAZE_W = 15;
    static const int MAZE_H = 19;
    uint8_t _maze[MAZE_H][MAZE_W];

    int _px = 1, _py = 1;  // Position joueur
    int _exitX = MAZE_W - 2;
    int _exitY = MAZE_H - 2;

    uint32_t _lastMoveTime = 0;
    uint32_t _timeLimit    = 0;
    int      _wallHits     = 0;

    void  _generateMaze(uint32_t seed);
    void  _dfsCarve(int x, int y);
    void  _drawMazeScene();
    bool  _tryMove(int dx, int dy);

    // Taille pixel d'une cellule
    static const int CELL = 15;
    // Offset pour centrer le labyrinthe sur le TFT
    static const int OFF_X = (TFT_W - MAZE_W * CELL) / 2;
    static const int OFF_Y = 30;

    // Brouillard de guerre — rayon en cellules
    int _fogRadius = 4;
};
