#pragma once
// ============================================================
// engine/DisplayEngine.h — Moteur graphique double buffer PSRAM
// LovyanGFX DMA, 60fps, animations procédurales, particules
// ============================================================
#include <LovyanGFX.hpp>
#include "../display_lgfx.h"
#include "../config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ── Particule pour le système de particules ───────────────────
struct Particle {
    float x, y;
    float vx, vy;
    uint32_t color565;
    uint16_t life, maxLife;
    float    size;
};

// ── Direction pour les flèches tutoriel ──────────────────────
enum class ArrowDir { UP, DOWN, LEFT, RIGHT };

class DisplayEngine {
public:
    static LGFX      tft;
    static LGFX_Sprite buf;  // Buffer de travail PSRAM

    static void init();
    static void flush();     // Pousse le buffer vers l'écran via DMA

    // ── Transitions "respirations" ───────────────────────────
    static void fadeToBlack(uint32_t durationMs = 400);
    static void fadeFromBlack(uint32_t durationMs = 400);
    static void breathPause(uint32_t durationMs = 800);  // Noir + RGB + buzzer

    // ── Primitives de base ───────────────────────────────────
    static void clear(uint32_t color = COL_BLACK);
    static void drawText(int x, int y, const char* txt, uint32_t color, uint8_t size = 1);
    static void drawTextCentered(int y, const char* txt, uint32_t color, uint8_t size = 1);
    static void typewriterText(int x, int y, const char* txt, uint32_t color, uint16_t charDelayMs, uint8_t size = 1);
    static void drawProgressBar(int x, int y, int w, int h, float pct, uint32_t colFill, uint32_t colBg);
    static void drawRoundBox(int x, int y, int w, int h, int r, uint32_t color, bool filled = false);

    // ── Tutoriel AAA ─────────────────────────────────────────
    static void drawJoystickIcon(int cx, int cy, ArrowDir hint);
    static void drawKeypadIcon(int cx, int cy, uint8_t row, uint8_t col);
    static void drawPotIcon(int cx, int cy, float angleDeg);
    static void drawSonarIcon(int cx, int cy, float dist, float maxDist);
    static void drawPulsingArrow(int cx, int cy, ArrowDir dir, uint32_t color);

    // ── Visualisations enigmes ───────────────────────────────
    static void drawMaze(uint8_t* maze, int cols, int rows,
                         int playerX, int playerY, int fogRadius);
    static void drawRadar(int cx, int cy, int r, float distCm, float targetCm);
    static void drawSpectrumBars(int x, int y, int w, int h, float* bands, int nbands, uint32_t color);
    static void drawOscilloscope(int x, int y, int w, int h, float freq, float t);
    static void drawHeatZone(int cx, int cy, int r, float intensity);

    // ── Système de particules ────────────────────────────────
    static void particleInit(int cx, int cy, uint32_t color, uint8_t count);
    static void particleUpdate();
    static void particleDraw();

    // ── Effets spéciaux ──────────────────────────────────────
    static void scanlineEffect();          // Overlay CRT scanlines
    static void glitchFrame(uint8_t intensity);  // Glitch visuel dramatique

    // ── Rétroéclairage ───────────────────────────────────────
    static void setBrightness(uint8_t val);  // 0–255

private:
    static Particle _particles[200];
    static uint8_t  _particleCount;
    static uint8_t  _brightness;
    static SemaphoreHandle_t _mutex;
};
