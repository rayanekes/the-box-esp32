// ============================================================
// engine/DisplayEngine.cpp — Implémentation moteur graphique
// ============================================================
#include "DisplayEngine.h"
#include <math.h>

// ── Membres statiques ────────────────────────────────────────
LGFX           DisplayEngine::tft;
LGFX_Sprite    DisplayEngine::buf(&DisplayEngine::tft);
Particle       DisplayEngine::_particles[200];
uint8_t        DisplayEngine::_particleCount = 0;
uint8_t        DisplayEngine::_brightness    = 200;
SemaphoreHandle_t DisplayEngine::_mutex      = nullptr;

// ── Init ─────────────────────────────────────────────────────
void DisplayEngine::init() {
    _mutex = xSemaphoreCreateMutex();
    tft.init();
    tft.setRotation(0);
    setBrightness(0);  // Démarrage écran éteint (intro dramatique)

    // Sprite double-buffer en PSRAM
    buf.setPsram(true);
    buf.setColorDepth(16);
    buf.createSprite(TFT_W, TFT_H);

    tft.fillScreen(COL_BLACK);
    buf.fillScreen(COL_BLACK);
}

void DisplayEngine::flush() {
    buf.pushSprite(0, 0);
}

void DisplayEngine::setBrightness(uint8_t val) {
    _brightness = val;
    tft.setBrightness(val);
}

// ── Transitions ──────────────────────────────────────────────
void DisplayEngine::fadeToBlack(uint32_t durationMs) {
    uint8_t start = _brightness;
    uint32_t steps = 30;
    uint32_t delay = durationMs / steps;
    for (uint32_t i = 0; i <= steps; ++i) {
        setBrightness((uint8_t)(start * (1.0f - (float)i / steps)));
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
    buf.fillScreen(COL_BLACK);
    flush();
}

void DisplayEngine::fadeFromBlack(uint32_t durationMs) {
    uint32_t steps = 30;
    uint32_t delay = durationMs / steps;
    for (uint32_t i = 0; i <= steps; ++i) {
        setBrightness((uint8_t)(200 * ((float)i / steps)));
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void DisplayEngine::breathPause(uint32_t durationMs) {
    buf.fillScreen(COL_BLACK);
    flush();
    vTaskDelay(pdMS_TO_TICKS(durationMs));
}

// ── Primitives ───────────────────────────────────────────────
void DisplayEngine::clear(uint32_t color) {
    buf.fillScreen(color);
}

void DisplayEngine::drawText(int x, int y, const char* txt, uint32_t color, uint8_t size) {
    buf.setTextColor(color);
    buf.setTextSize(size);
    buf.setCursor(x, y);
    buf.print(txt);
}

void DisplayEngine::drawTextCentered(int y, const char* txt, uint32_t color, uint8_t size) {
    buf.setTextColor(color);
    buf.setTextSize(size);
    int w = strlen(txt) * 6 * size;
    buf.setCursor((TFT_W - w) / 2, y);
    buf.print(txt);
}

void DisplayEngine::typewriterText(int x, int y, const char* txt,
                                    uint32_t color, uint16_t charDelayMs, uint8_t size) {
    buf.setTextColor(color);
    buf.setTextSize(size);
    int cx = x;
    for (const char* p = txt; *p; ++p) {
        buf.setCursor(cx, y);
        buf.print(*p);
        flush();
        cx += 6 * size;
        vTaskDelay(pdMS_TO_TICKS(charDelayMs));
    }
}

void DisplayEngine::drawProgressBar(int x, int y, int w, int h,
                                     float pct, uint32_t colFill, uint32_t colBg) {
    buf.fillRect(x, y, w, h, colBg);
    int filled = (int)(w * constrain(pct, 0.0f, 1.0f));
    buf.fillRect(x, y, filled, h, colFill);
    buf.drawRect(x, y, w, h, COL_GREY);
}

void DisplayEngine::drawRoundBox(int x, int y, int w, int h, int r,
                                  uint32_t color, bool filled) {
    if (filled)
        buf.fillRoundRect(x, y, w, h, r, color);
    else
        buf.drawRoundRect(x, y, w, h, r, color);
}

// ── Flèche pulsante (tutoriel AAA) ───────────────────────────
void DisplayEngine::drawPulsingArrow(int cx, int cy, ArrowDir dir, uint32_t color) {
    float    t     = millis() * 0.004f;
    float    pulse = (sinf(t) + 1.0f) * 0.5f;
    uint8_t  alpha = (uint8_t)(80 + 175 * pulse);
    int      sz    = 20 + (int)(8 * pulse);  // Taille qui palpite

    // Flèche en triangle selon la direction
    int x0, y0, x1, y1, x2, y2;
    switch (dir) {
        case ArrowDir::UP:
            x0=cx; y0=cy-sz; x1=cx-sz/2; y1=cy+sz/2; x2=cx+sz/2; y2=cy+sz/2; break;
        case ArrowDir::DOWN:
            x0=cx; y0=cy+sz; x1=cx-sz/2; y1=cy-sz/2; x2=cx+sz/2; y2=cy-sz/2; break;
        case ArrowDir::LEFT:
            x0=cx-sz; y0=cy; x1=cx+sz/2; y1=cy-sz/2; x2=cx+sz/2; y2=cy+sz/2; break;
        default: // RIGHT
            x0=cx+sz; y0=cy; x1=cx-sz/2; y1=cy-sz/2; x2=cx-sz/2; y2=cy+sz/2; break;
    }
    buf.fillTriangle(x0, y0, x1, y1, x2, y2, color);

    // Halo autour de la pointe
    int haloR = 4 + (int)(3 * pulse);
    buf.drawCircle(x0, y0, haloR, color);
}

// ── Icône joystick animée (tutoriel) ─────────────────────────
void DisplayEngine::drawJoystickIcon(int cx, int cy, ArrowDir hint) {
    // Corps du joystick
    buf.drawCircle(cx, cy, 22, COL_GREY);
    buf.drawCircle(cx, cy, 20, COL_GREY_DARK);

    // Stick animé vers la direction indiquée
    float t    = millis() * 0.003f;
    float pulse= sinf(t);
    int   dx   = 0, dy = 0;
    switch (hint) {
        case ArrowDir::UP:    dy = -(int)(8 * fabs(pulse)); break;
        case ArrowDir::DOWN:  dy =  (int)(8 * fabs(pulse)); break;
        case ArrowDir::LEFT:  dx = -(int)(8 * fabs(pulse)); break;
        case ArrowDir::RIGHT: dx =  (int)(8 * fabs(pulse)); break;
    }
    buf.fillCircle(cx + dx, cy + dy, 8, COL_TERMINAL);
    buf.drawCircle(cx + dx, cy + dy, 9, COL_WHITE);
}

// ── Icône sonar (tutoriel) ────────────────────────────────────
void DisplayEngine::drawSonarIcon(int cx, int cy, float dist, float maxDist) {
    float norm = constrain(1.0f - (dist / maxDist), 0.0f, 1.0f);
    for (int r = 40; r >= 10; r -= 10) {
        float rNorm = 1.0f - (float)(r - 10) / 30.0f;
        uint32_t col = (rNorm < norm) ? COL_TERMINAL : COL_GREY_DARK;
        buf.drawCircle(cx, cy, r, col);
    }
    buf.fillCircle(cx, cy, 4, COL_WHITE);
}

// ── Icône potentiomètre (tutoriel) ───────────────────────────
void DisplayEngine::drawPotIcon(int cx, int cy, float angleDeg) {
    buf.drawCircle(cx, cy, 20, COL_GREY);
    float rad = (angleDeg - 90) * DEG_TO_RAD;
    int ex    = cx + (int)(16 * cosf(rad));
    int ey    = cy + (int)(16 * sinf(rad));
    buf.drawLine(cx, cy, ex, ey, COL_TERMINAL);
    buf.fillCircle(ex, ey, 3, COL_GOLD);
}

// ── Radar (Enigme 4 — La Sentinelle) ─────────────────────────
void DisplayEngine::drawRadar(int cx, int cy, int r, float distCm, float targetCm) {
    float t = millis() * 0.001f;

    // Fond radar
    buf.fillCircle(cx, cy, r, 0x0008);
    for (int i = 1; i <= 4; ++i)
        buf.drawCircle(cx, cy, r * i / 4, 0x0210);

    // Zone cible (anneau vert)
    int targetR = (int)(r * constrain(targetCm / 80.0f, 0.1f, 0.9f));
    buf.drawCircle(cx, cy, targetR + 3, COL_TERMINAL);
    buf.drawCircle(cx, cy, targetR - 3, COL_TERMINAL);

    // Ligne rotative (scanner)
    float scanAngle = fmod(t * 1.5f, 2 * PI);
    int ex = cx + (int)(r * cosf(scanAngle));
    int ey = cy + (int)(r * sinf(scanAngle));
    buf.drawLine(cx, cy, ex, ey, 0x07C0);

    // Blip de la main
    float bDist = constrain(distCm / 80.0f, 0.0f, 1.0f);
    int   bR    = (int)(r * bDist);
    buf.fillCircle(cx + bR, cy, 4, COL_DANGER);
}

// ── Oscilloscope (Enigme 3 — Le Spectre Radio) ───────────────
void DisplayEngine::drawOscilloscope(int x, int y, int w, int h, float freq, float t) {
    buf.drawRect(x, y, w, h, COL_GREY_DARK);
    int cy  = y + h / 2;
    int lastY = cy;

    for (int i = 0; i < w; ++i) {
        float phase  = (float)i / w * 2 * PI * 4;
        float noise  = sinf(phase * 7.3f + t * 11.0f) * 0.15f;
        float signal = sinf(phase + t * freq * 0.1f) * 0.7f + noise;
        int   sy     = cy - (int)(signal * h * 0.45f);
        if (i > 0) buf.drawLine(x + i - 1, lastY, x + i, sy, COL_TERMINAL);
        lastY = sy;
    }
    // Grille
    for (int gx = x; gx < x + w; gx += 20)
        buf.drawFastVLine(gx, y, h, COL_GREY_DARK);
    for (int gy = y; gy < y + h; gy += 20)
        buf.drawFastHLine(x, gy, w, COL_GREY_DARK);
}

// ── Barres de spectre ─────────────────────────────────────────
void DisplayEngine::drawSpectrumBars(int x, int y, int w, int h,
                                      float* bands, int nbands, uint32_t color) {
    int bw = w / nbands - 2;
    for (int i = 0; i < nbands; ++i) {
        int bh = (int)(h * constrain(bands[i], 0.0f, 1.0f));
        buf.fillRect(x + i * (bw + 2), y + h - bh, bw, bh, color);
        buf.drawRect(x + i * (bw + 2), y, bw, h, COL_GREY_DARK);
    }
}

// ── Zone de chaleur (Enigme 5 — indicateur potentiomètre) ────
void DisplayEngine::drawHeatZone(int cx, int cy, int r, float intensity) {
    float t = millis() * 0.005f;
    for (int ring = r; ring >= 0; ring -= 5) {
        float   iFade = intensity * (1.0f - (float)ring / r);
        uint8_t g     = (uint8_t)(min(255.0f, iFade * 512));
        uint8_t rb    = (uint8_t)(min(255.0f, iFade * 255));
        uint16_t col  = buf.color888(rb, g, 0);
        buf.drawCircle(cx + (int)(2 * sinf(t + ring * 0.5f)),
                        cy + (int)(2 * cosf(t + ring * 0.3f)),
                        ring, col);
    }
}

// ── Système de particules ─────────────────────────────────────
void DisplayEngine::particleInit(int cx, int cy, uint32_t color, uint8_t count) {
    _particleCount = min((int)count, 200);
    for (int i = 0; i < _particleCount; ++i) {
        float angle  = random(0, 628) / 100.0f;
        float speed  = 0.5f + random(0, 30) * 0.1f;
        _particles[i] = {
            (float)cx, (float)cy,
            cosf(angle) * speed, sinf(angle) * speed,
            color, 60, 60,
            1.0f + random(0, 20) * 0.1f
        };
    }
}

void DisplayEngine::particleUpdate() {
    for (int i = 0; i < _particleCount; ++i) {
        _particles[i].x  += _particles[i].vx;
        _particles[i].y  += _particles[i].vy;
        _particles[i].vy += 0.05f;  // Gravité légère
        _particles[i].life--;
    }
}

void DisplayEngine::particleDraw() {
    for (int i = 0; i < _particleCount; ++i) {
        auto& p   = _particles[i];
        if (p.life == 0) continue;
        float alpha = (float)p.life / p.maxLife;
        // Fade vers le noir
        uint8_t r = ((p.color565 >> 11) & 0x1F) * alpha * 8;
        uint8_t g = ((p.color565 >>  5) & 0x3F) * alpha * 4;
        uint8_t b = ( p.color565        & 0x1F) * alpha * 8;
        uint16_t col = buf.color888(r, g, b);
        int sz = max(1, (int)(p.size * alpha));
        buf.fillCircle((int)p.x, (int)p.y, sz, col);
    }
}

// ── Effet CRT scanlines ───────────────────────────────────────
void DisplayEngine::scanlineEffect() {
    for (int y = 0; y < TFT_H; y += 4) {
        buf.drawFastHLine(0, y, TFT_W, COL_BLACK);
    }
}

// ── Glitch visuel ─────────────────────────────────────────────
void DisplayEngine::glitchFrame(uint8_t intensity) {
    for (int i = 0; i < intensity; ++i) {
        int y  = random(0, TFT_H);
        int x  = random(0, TFT_W / 2);
        int w  = random(10, TFT_W / 2);
        int off= random(-15, 15);
        // Décalage horizontal d'une ligne
        for (int px = 0; px < w && (x + px + off) < TFT_W && (x + px) < TFT_W; ++px) {
            uint16_t c = buf.readPixel(x + px, y);
            buf.drawPixel(x + px + off, y, c);
        }
        buf.drawFastHLine(x, y, w, random(0, 2) ? COL_TERMINAL : COL_DANGER);
    }
}
