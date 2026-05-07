// ============================================================
// fsm/states/StateIntro.cpp — Intro cinematique + skip + mode select
// ============================================================
#include "StateIntro.h"
#include "../../engine/DisplayEngine.h"
#include "../../engine/BuzzerEngine.h"
#include "../../engine/RGBEngine.h"
#include "../../sensors/JoystickReader.h"

// ── Textes de la cinematique ──────────────────────────────────
static const char* BOOT_LINES[] = {
    "> INITIALISATION SYSTEME...",
    "> PROTOCOLE OMEGA : ACTIF",
    "> CONNEXION SECURISEE : OK",
    "> ACCES : CLASSIFIE",
    nullptr
};

static const char* BRIEF_LINES[] = {
    "LA BOITE EST VERROUILLEE.",
    "CINQ PROTOCOLES LA PROTEGENT.",
    "VOUS AVEZ 30 MINUTES.",
    "EN CAS D'ECHEC :",
    "EFFACEMENT AUTOMATIQUE.",
    nullptr
};

// ── Phases de l'intro ─────────────────────────────────────────
enum class IntroPhase {
    DARKNESS,
    BOOT_TEXT,
    GLITCH_BURST,
    BREATH_1,
    BRIEFING,
    BREATH_2,
    PROFILE_SELECT,
    MODE_SELECT,
    DONE
};

static IntroPhase    _phase   = IntroPhase::DARKNESS;
static uint32_t      _phaseT  = 0;
static int           _lineIdx = 0;
static uint32_t      _lineT   = 0;
static int           _profile = 1;   // 0=Public 1=Avance 2=Expert
static int           _mode    = 0;   // 0=Escape 1=Quiz

// Skip : pression longue du joystick pendant l'intro
static bool          _skipReq    = false;
static uint32_t      _pressStart = 0;
#define SKIP_HOLD_MS  800  // Appui 800ms pour skipper

static void _enterPhase(IntroPhase p) {
    _phase   = p;
    _phaseT  = millis();
    _lineIdx = 0;
}

// ── Verifie le skip ──────────────────────────────────────────
static bool _checkSkip() {
    auto joy = JoystickReader::read();
    if (joy.pressed) {
        if (_pressStart == 0) _pressStart = millis();
        if (millis() - _pressStart >= SKIP_HOLD_MS) {
            _skipReq = true;
            return true;
        }
    } else {
        _pressStart = 0;
    }
    return false;
}

// ── Run ───────────────────────────────────────────────────────
IntroResult StateIntro::run() {
    _phase      = IntroPhase::DARKNESS;
    _phaseT     = millis();
    _skipReq    = false;
    _pressStart = 0;
    _mode       = 0;
    _profile    = 1;

    DisplayEngine::setBrightness(0);
    DisplayEngine::clear();
    DisplayEngine::flush();
    RGBEngine::setMode(RGBMode::OFF);

    while (_phase != IntroPhase::DONE) {
        uint32_t elapsed = millis() - _phaseT;

        // ── Skip possible pendant les phases cinematiques ──────
        if (_phase != IntroPhase::PROFILE_SELECT &&
            _phase != IntroPhase::MODE_SELECT &&
            _phase != IntroPhase::DONE) {
            if (_checkSkip()) {
                DisplayEngine::fadeToBlack(200);
                DisplayEngine::breathPause(300);
                _enterPhase(IntroPhase::PROFILE_SELECT);
                _profile = 1;
                RGBEngine::setMode(RGBMode::PULSE);
                RGBEngine::setColor(0, 100, 200);
                DisplayEngine::fadeFromBlack(300);
                BuzzerEngine::play(SFX::CONFIRM);
                continue;
            }
        }

        switch (_phase) {

        // ── Phase 0 : Tenebres 2 secondes ──────────────────────
        case IntroPhase::DARKNESS:
            if (elapsed > 2000) {
                DisplayEngine::fadeFromBlack(600);
                RGBEngine::setMode(RGBMode::HEARTBEAT);
                BuzzerEngine::play(SFX::BOOT_CHIME);
                _enterPhase(IntroPhase::BOOT_TEXT);
                _lineT = millis();
            }
            break;

        // ── Phase 1 : Texte terminal ────────────────────────────
        case IntroPhase::BOOT_TEXT: {
            if (BOOT_LINES[_lineIdx] == nullptr) {
                vTaskDelay(pdMS_TO_TICKS(600));
                _enterPhase(IntroPhase::GLITCH_BURST);
                break;
            }
            if (millis() - _lineT > 500) {
                int y = 60 + _lineIdx * 24;
                DisplayEngine::typewriterText(
                    10, y, BOOT_LINES[_lineIdx],
                    (_lineIdx == 3) ? COL_DANGER : COL_TERMINAL,
                    25, 1
                );
                if (_lineIdx == 3) {
                    BuzzerEngine::play(SFX::ALARM);
                    RGBEngine::setMode(RGBMode::ALERT_RED);
                }
                _lineIdx++;
                _lineT = millis();
            }
            break;
        }

        // ── Phase 2 : Glitch dramatique ─────────────────────────
        case IntroPhase::GLITCH_BURST:
            for (int g = 0; g < 8; ++g) {
                DisplayEngine::glitchFrame(6);
                DisplayEngine::flush();
                vTaskDelay(pdMS_TO_TICKS(60));
            }
            DisplayEngine::fadeToBlack(300);
            DisplayEngine::breathPause(900);
            RGBEngine::setMode(RGBMode::BREATHING);
            RGBEngine::setColor(80, 0, 120);
            _enterPhase(IntroPhase::BREATH_1);
            break;

        // ── Phase 3 : Titre ─────────────────────────────────────
        case IntroPhase::BREATH_1:
            if (elapsed < 200) {
                DisplayEngine::clear();
                DisplayEngine::drawRoundBox(20, 110, TFT_W - 40, 100, 10, COL_VIOLET, true);
                DisplayEngine::drawTextCentered(125, "SMART", COL_GOLD, 3);
                DisplayEngine::drawTextCentered(158, "ESCAPE BOX", COL_WHITE, 2);
                DisplayEngine::buf.drawFastHLine(30, 148, TFT_W - 60, COL_GOLD);
                // Hint skip
                DisplayEngine::drawTextCentered(280, "Maintenir joystick = skip", COL_GREY, 1);
                DisplayEngine::flush();
                vTaskDelay(pdMS_TO_TICKS(2500));
                DisplayEngine::fadeToBlack(400);
                DisplayEngine::breathPause(700);
                _lineIdx = 0;
                _lineT   = millis();
                _enterPhase(IntroPhase::BRIEFING);
            }
            break;

        // ── Phase 4 : Briefing ──────────────────────────────────
        case IntroPhase::BRIEFING: {
            if (BRIEF_LINES[_lineIdx] == nullptr) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                DisplayEngine::fadeToBlack(500);
                DisplayEngine::breathPause(800);
                _enterPhase(IntroPhase::BREATH_2);
                break;
            }
            if (millis() - _lineT > 700) {
                if (_lineIdx == 0) {
                    DisplayEngine::clear();
                    DisplayEngine::drawTextCentered(30, "[ BRIEFING ]", COL_GREY, 1);
                    DisplayEngine::buf.drawFastHLine(10, 46, TFT_W - 20, COL_GREY_DARK);
                }
                int y = 70 + _lineIdx * 28;
                bool danger = (_lineIdx >= 3);
                DisplayEngine::typewriterText(
                    20, y, BRIEF_LINES[_lineIdx],
                    danger ? COL_DANGER : COL_WHITE, 30, 1
                );
                DisplayEngine::flush();
                if (danger) BuzzerEngine::play(SFX::CLICK);
                _lineIdx++;
                _lineT = millis();
            }
            break;
        }

        // ── Phase 5 : Respiration avant selection ───────────────
        case IntroPhase::BREATH_2:
            RGBEngine::setMode(RGBMode::PULSE);
            RGBEngine::setColor(0, 100, 200);
            DisplayEngine::fadeFromBlack(400);
            _enterPhase(IntroPhase::PROFILE_SELECT);
            _profile = 1;
            break;

        // ── Phase 6 : Selection du profil ───────────────────────
        case IntroPhase::PROFILE_SELECT: {
            static const char* profiles[] = {"PUBLIC", "AVANCE", "EXPERT"};
            static const char* descs[]    = {
                "Famille & Debutants",
                "Lycee / BTS",
                "Ingenieurs / Pro"
            };
            static const uint32_t cols[] = {0x07E0, 0xFEA0, 0xF800};

            DisplayEngine::clear();
            DisplayEngine::drawTextCentered(18, "CHOISISSEZ VOTRE PROFIL", COL_WHITE, 1);
            DisplayEngine::buf.drawFastHLine(10, 33, TFT_W - 20, COL_GREY_DARK);

            for (int i = 0; i < 3; ++i) {
                bool sel = (i == _profile);
                int  y   = 55 + i * 70;
                DisplayEngine::drawRoundBox(
                    sel ? 5 : 15, y,
                    sel ? TFT_W - 10 : TFT_W - 30,
                    55, 8, sel ? cols[i] : COL_GREY_DARK, sel
                );
                DisplayEngine::drawText(sel ? 20 : 30, y + 10, profiles[i], sel ? COL_BLACK : cols[i], 2);
                DisplayEngine::drawText(sel ? 20 : 30, y + 35, descs[i], sel ? COL_BLACK : COL_GREY, 1);
            }
            DisplayEngine::drawTextCentered(290, "OK = Appuyer joystick", COL_GREY, 1);
            DisplayEngine::flush();

            auto joy = JoystickReader::read();
            static uint32_t lastNav = 0;
            if (millis() - lastNav > 200) {
                if (joy.dir == JoyDir::UP   && _profile > 0) { _profile--; BuzzerEngine::play(SFX::CLICK); lastNav = millis(); }
                if (joy.dir == JoyDir::DOWN && _profile < 2) { _profile++; BuzzerEngine::play(SFX::CLICK); lastNav = millis(); }
            }

            if (joy.pressed) {
                BuzzerEngine::play(SFX::CONFIRM);
                RGBEngine::setMode(RGBMode::PULSE);
                RGBEngine::setColor(0, 200, 80);
                DisplayEngine::fadeToBlack(300);
                DisplayEngine::breathPause(400);
                _enterPhase(IntroPhase::MODE_SELECT);
                _mode = 0;
            }
            break;
        }

        // ── Phase 7 : Selection du mode ─────────────────────────
        case IntroPhase::MODE_SELECT: {
            // Titres et icones
            static const char* modeNames[] = {"ESCAPE GAME", "MODE DEFI"};
            static const char* modeDescs[] = {
                "5 Protocoles + Quiz entre les niveaux",
                "Quiz multi-domaines en rafale"
            };
            static const uint16_t modeCols[] = {COL_GOLD, COL_TERMINAL};

            DisplayEngine::clear();
            DisplayEngine::drawTextCentered(18, "CHOISISSEZ VOTRE MISSION", COL_WHITE, 1);
            DisplayEngine::buf.drawFastHLine(10, 33, TFT_W - 20, COL_GREY_DARK);

            for (int i = 0; i < 2; ++i) {
                bool sel = (i == _mode);
                int  y   = 80 + i * 100;
                DisplayEngine::drawRoundBox(
                    sel ? 5 : 20, y,
                    sel ? TFT_W - 10 : TFT_W - 40,
                    80, 10, sel ? modeCols[i] : COL_GREY_DARK, sel
                );
                // Icone
                DisplayEngine::drawText(sel ? 20 : 35, y + 12,
                    (i == 0) ? "[ESC]" : "[QIZ]",
                    sel ? COL_BLACK : modeCols[i], 2);
                DisplayEngine::drawText(sel ? 20 : 35, y + 36,
                    modeNames[i], sel ? COL_BLACK : modeCols[i], 1);
                DisplayEngine::drawText(sel ? 20 : 35, y + 52,
                    modeDescs[i], sel ? COL_GREY_DARK : COL_GREY, 1);
            }
            DisplayEngine::drawTextCentered(290, "OK = Appuyer joystick", COL_GREY, 1);
            DisplayEngine::flush();

            auto joy = JoystickReader::read();
            static uint32_t lastNav2 = 0;
            if (millis() - lastNav2 > 200) {
                if (joy.dir == JoyDir::UP   && _mode > 0) { _mode--; BuzzerEngine::play(SFX::CLICK); lastNav2 = millis(); }
                if (joy.dir == JoyDir::DOWN && _mode < 1) { _mode++; BuzzerEngine::play(SFX::CLICK); lastNav2 = millis(); }
            }

            if (joy.pressed) {
                BuzzerEngine::play(SFX::CONFIRM);
                if (_mode == 0) RGBEngine::setMode(RGBMode::ALERT_GREEN);
                else { RGBEngine::setMode(RGBMode::RAINBOW); }
                DisplayEngine::fadeToBlack(400);
                DisplayEngine::breathPause(600);
                _enterPhase(IntroPhase::DONE);
            }
            break;
        }

        default: break;
        }

        vTaskDelay(pdMS_TO_TICKS(16));  // ~60fps
    }

    IntroResult result;
    result.profile = static_cast<GameProfile>(_profile);
    result.mode    = static_cast<GameMode>(_mode);
    return result;
}
