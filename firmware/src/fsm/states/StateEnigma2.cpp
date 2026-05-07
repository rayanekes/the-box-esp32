// ============================================================
// Énigme 2 — "Le Labyrinthe Cognitif"
// Joystick + Labyrinthe procédural DFS + Brouillard de guerre
// Web App : vue aérienne partielle
// ============================================================
#include "StateEnigma2.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/JoystickReader.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;

static const TutStep MAZE_TUT[] = {
    {
        "NAVIGUE DANS LE LABYRINTHE",
        "Utilise le joystick",
        TutInputType::JOY_UP, ArrowDir::UP, 0, TFT_W/2, 130
    },
    {
        "EVITE LES MURS !",
        "Chaque collision te ralentit",
        TutInputType::AUTO_DONE, ArrowDir::UP, 2500, TFT_W/2, 130
    },
    {
        "TROUVE LA SORTIE",
        "En bas a droite du labyrinthe",
        TutInputType::AUTO_DONE, ArrowDir::DOWN, 2000, TFT_W/2, 130
    }
};

// ── Génération DFS récursive ──────────────────────────────────
void StateEnigma2::_dfsCarve(int x, int y) {
    // Directions mélangées avec Fisher-Yates
    int dirs[4][2] = {{0,-2},{0,2},{-2,0},{2,0}};
    for (int i = 3; i > 0; --i) {
        int j = random(0, i + 1);
        int tx = dirs[i][0]; dirs[i][0] = dirs[j][0]; dirs[j][0] = tx;
        int ty = dirs[i][1]; dirs[i][1] = dirs[j][1]; dirs[j][1] = ty;
    }
    for (auto& d : dirs) {
        int nx = x + d[0], ny = y + d[1];
        if (nx > 0 && nx < MAZE_W-1 && ny > 0 && ny < MAZE_H-1
            && _maze[ny][nx] == 1) {
            _maze[ny][nx]             = 0;
            _maze[y + d[1]/2][x + d[0]/2] = 0;
            _dfsCarve(nx, ny);
        }
    }
}

void StateEnigma2::_generateMaze(uint32_t seed) {
    randomSeed(seed);
    for (int y = 0; y < MAZE_H; ++y)
        for (int x = 0; x < MAZE_W; ++x)
            _maze[y][x] = 1;
    _maze[1][1] = 0;
    _dfsCarve(1, 1);
    _maze[_exitY][_exitX] = 0;
    // S'assurer que la sortie est accessible
    _maze[_exitY-1][_exitX] = 0;
}

// ── Dessin du labyrinthe avec brouillard ─────────────────────
void StateEnigma2::_drawMazeScene() {
    DisplayEngine::clear(0x0008);  // Fond très sombre

    // Timer en haut
    uint32_t remaining = (_timeLimit > elapsed()) ? (_timeLimit - elapsed()) / 1000 : 0;
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "TEMPS: %02lus", remaining);
    DisplayEngine::drawText(5, 5, timeBuf, remaining < 30 ? COL_DANGER : COL_GOLD, 1);

    // Barre de timer
    float pct = remaining / (_timeLimit / 1000.0f / 1000.0f);
    DisplayEngine::drawProgressBar(5, 18, TFT_W - 10, 5,
        (float)remaining / (_timeLimit / 1000.0f), COL_TERMINAL, COL_GREY_DARK);

    // Rendu du labyrinthe avec brouillard
    for (int cy = 0; cy < MAZE_H; ++cy) {
        for (int cx = 0; cx < MAZE_W; ++cx) {
            int dx    = abs(cx - _px);
            int dy    = abs(cy - _py);
            float dist = sqrtf(dx*dx + dy*dy);

            int px = OFF_X + cx * CELL;
            int py = OFF_Y + cy * CELL;

            if (dist > _fogRadius) {
                // Brouillard — couleur très sombre
                DisplayEngine::buf.fillRect(px, py, CELL, CELL, 0x0104);
                continue;
            }
            float fade = 1.0f - (dist / _fogRadius) * 0.6f;

            if (_maze[cy][cx] == 1) {
                // Mur
                uint16_t wallCol = DisplayEngine::buf.color888(
                    (uint8_t)(30 * fade), (uint8_t)(60 * fade), (uint8_t)(80 * fade));
                DisplayEngine::buf.fillRect(px, py, CELL, CELL, wallCol);
                DisplayEngine::buf.drawRect(px, py, CELL, CELL, 0x0209);
            } else {
                // Couloir
                uint16_t floorCol = DisplayEngine::buf.color888(
                    0, (uint8_t)(15 * fade), (uint8_t)(25 * fade));
                DisplayEngine::buf.fillRect(px, py, CELL, CELL, floorCol);
            }

            // Sortie — pulsante
            if (cx == _exitX && cy == _exitY) {
                float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
                uint8_t gv = (uint8_t)(150 + 105 * pulse);
                DisplayEngine::buf.fillRect(px+2, py+2, CELL-4, CELL-4,
                    DisplayEngine::buf.color888(0, gv, 0));
                DisplayEngine::buf.drawRect(px, py, CELL, CELL, COL_TERMINAL);
            }
        }
    }

    // Joueur
    int ppx = OFF_X + _px * CELL + CELL/2;
    int ppy = OFF_Y + _py * CELL + CELL/2;
    float playerPulse = (sinf(millis() * 0.01f) + 1.0f) * 0.5f;
    DisplayEngine::buf.fillCircle(ppx, ppy, 4, COL_GOLD);
    DisplayEngine::buf.drawCircle(ppx, ppy, (int)(5 + 2 * playerPulse), COL_WHITE);

    // Info bas
    char hitBuf[24];
    snprintf(hitBuf, sizeof(hitBuf), "COLS: %d", _wallHits);
    DisplayEngine::drawText(5, TFT_H - 14, hitBuf, COL_GREY, 1);
    DisplayEngine::drawTextCentered(TFT_H - 14, "EXIT >", COL_TERMINAL, 1);
}

bool StateEnigma2::_tryMove(int dx, int dy) {
    int nx = _px + dx, ny = _py + dy;
    if (nx >= 0 && nx < MAZE_W && ny >= 0 && ny < MAZE_H
        && _maze[ny][nx] == 0) {
        _px = nx; _py = ny;
        return true;
    }
    // Collision mur
    _wallHits++;
    BuzzerEngine::play(SFX::ERROR);
    RGBEngine::setMode(RGBMode::ALERT_RED);
    return false;
}

void StateEnigma2::onEnter(GameProfile p) {
    _profile   = p;
    _enterTime = millis();
    _tutDone   = false;
    _wallHits  = 0;
    _px = 1; _py = 1;

    switch (p) {
        case GameProfile::PUBLIC:   _fogRadius = 5; _timeLimit = 300000; break;
        case GameProfile::ADVANCED: _fogRadius = 3; _timeLimit = 180000; break;
        case GameProfile::EXPERT:   _fogRadius = 2; _timeLimit = 120000; break;
    }

    // Seed unique depuis RTC
    DateTime now = rtc.now();
    uint32_t seed = now.unixtime();
    _generateMaze(seed);

    // Annonce
    DisplayEngine::clear();
    DisplayEngine::drawTextCentered(110, "PROTOCOLE 02", COL_GOLD, 2);
    DisplayEngine::drawTextCentered(145, "LE LABYRINTHE", COL_WHITE, 2);
    DisplayEngine::drawTextCentered(175, "COGNITIF", COL_WHITE, 1);
    DisplayEngine::flush();
    BuzzerEngine::play(SFX::LEVEL_UP);
    RGBEngine::setMode(RGBMode::PULSE);
    RGBEngine::setColor(0, 100, 200);
    vTaskDelay(pdMS_TO_TICKS(2500));
    DisplayEngine::fadeToBlack(400);
    DisplayEngine::breathPause(700);

    _tut.begin(MAZE_TUT, 3, "PROTOCOLE 02");
}

bool StateEnigma2::update() {
    if (!_tutDone) {
        _tutDone = _tut.update();
        if (_tutDone) {
            _enterTime = millis();  // Reset timer après tuto
            RGBEngine::setMode(RGBMode::BREATHING);
            RGBEngine::setColor(0, 40, 100);
        }
        return false;
    }

    // ── Jeu principal ─────────────────────────────────────────
    if (timedOut(_timeLimit)) {
        // Timeout → Game Over
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(130, "TEMPS ECOULE !", COL_DANGER, 2);
        DisplayEngine::flush();
        BuzzerEngine::play(SFX::ALARM);
        RGBEngine::setMode(RGBMode::ALERT_RED);
        vTaskDelay(pdMS_TO_TICKS(2000));
        DisplayEngine::fadeToBlack(400);
        return false;  // Retenter (GameFSM gère les retry)
    }

    // Mouvement joystick
    if (millis() - _lastMoveTime > 150) {
        auto joy = JoystickReader::read();
        bool moved = false;
        if      (joy.dir == JoyDir::UP)    moved = _tryMove(0, -1);
        else if (joy.dir == JoyDir::DOWN)  moved = _tryMove(0,  1);
        else if (joy.dir == JoyDir::LEFT)  moved = _tryMove(-1, 0);
        else if (joy.dir == JoyDir::RIGHT) moved = _tryMove( 1, 0);

        if (moved) {
            _lastMoveTime = millis();
            RGBEngine::setMode(RGBMode::BREATHING);
        }
    }

    _drawMazeScene();
    DisplayEngine::flush();

    // Condition de victoire
    if (_px == _exitX && _py == _exitY) {
        BuzzerEngine::play(SFX::SUCCESS);
        RGBEngine::setMode(RGBMode::RAINBOW);
        DisplayEngine::clear();
        DisplayEngine::drawTextCentered(120, "SORTIE TROUVEE !", COL_TERMINAL, 2);
        DisplayEngine::drawTextCentered(155, "Protocole 02 : OK", COL_GOLD, 1);
        DisplayEngine::flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        DisplayEngine::fadeToBlack(500);
        DisplayEngine::breathPause(800);
        return true;
    }
    return false;
}

void StateEnigma2::onExit() {
    RGBEngine::setMode(RGBMode::OFF);
}
