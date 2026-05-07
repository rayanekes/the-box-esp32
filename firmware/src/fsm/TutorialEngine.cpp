// ============================================================
// fsm/TutorialEngine.cpp
// ============================================================
#include "TutorialEngine.h"

void TutorialEngine::begin(const TutStep* steps, uint8_t count, const char* title) {
    _steps   = steps;
    _count   = count;
    _title   = title;
    _current = 0;
    _stepEnter = millis();
    _showing = true;
    _holdActive = false;
}

void TutorialEngine::reset() {
    _current = 0;
    _showing = false;
}

bool TutorialEngine::update() {
    if (!_showing || _current >= _count) return true;

    const TutStep& s = _steps[_current];
    _drawStep(s);
    DisplayEngine::flush();

    if (_checkInput(s)) {
        _onStepSuccess();
        _current++;
        _stepEnter  = millis();
        _holdActive = false;
        if (_current >= _count) {
            // Tutoriel terminé — respiration + GO
            DisplayEngine::clear();
            DisplayEngine::drawTextCentered(140, "C'EST PARTI !", COL_TERMINAL, 2);
            DisplayEngine::flush();
            BuzzerEngine::play(SFX::LEVEL_UP);
            RGBEngine::setMode(RGBMode::ALERT_GREEN);
            vTaskDelay(pdMS_TO_TICKS(1500));
            return true;
        }
    }
    return false;
}

void TutorialEngine::_drawStep(const TutStep& s) {
    DisplayEngine::clear(COL_BLACK);

    // Titre du tutoriel en haut
    if (_title) {
        DisplayEngine::drawRoundBox(4, 4, TFT_W - 8, 22, 4, COL_GREY_DARK, true);
        DisplayEngine::drawTextCentered(8, _title, COL_GOLD, 1);
    }

    // Indicateur de progression
    for (uint8_t i = 0; i < _count; ++i) {
        uint32_t col = (i < _current) ? COL_TERMINAL : (i == _current ? COL_GOLD : COL_GREY_DARK);
        DisplayEngine::buf.fillCircle(TFT_W / 2 - (_count * 10) / 2 + i * 10 + 5, 32, 3, col);
    }

    // Icône animée centrale
    int ix = s.iconCenterX ? s.iconCenterX : TFT_W / 2;
    int iy = s.iconCenterY ? s.iconCenterY : 120;

    switch (s.inputType) {
        case TutInputType::JOY_UP:
        case TutInputType::JOY_DOWN:
        case TutInputType::JOY_LEFT:
        case TutInputType::JOY_RIGHT:
        case TutInputType::JOY_PRESS:
            DisplayEngine::drawJoystickIcon(ix, iy, s.arrowDir);
            DisplayEngine::drawPulsingArrow(ix, iy - 50, s.arrowDir, COL_GOLD);
            break;
        case TutInputType::POT_TURN_RIGHT:
        case TutInputType::POT_TURN_LEFT: {
            float a = (s.inputType == TutInputType::POT_TURN_RIGHT)
                ? fmod(millis() * 0.1f, 270.0f)
                : 270.0f - fmod(millis() * 0.1f, 270.0f);
            DisplayEngine::drawPotIcon(ix, iy, a);
            break;
        }
        case TutInputType::SONAR_NEAR:
        case TutInputType::SONAR_HOLD: {
            float dist = SonarReader::readCm();
            DisplayEngine::drawSonarIcon(ix, iy, dist, 60.0f);
            break;
        }
        default: break;
    }

    // Texte principal
    if (s.line1) {
        DisplayEngine::drawRoundBox(10, 200, TFT_W - 20, 30, 6, COL_GREY_DARK, true);
        DisplayEngine::drawTextCentered(208, s.line1, COL_WHITE, 1);
    }
    // Sous-texte
    if (s.line2) {
        DisplayEngine::drawTextCentered(240, s.line2, COL_GREY, 1);
    }

    // Indicateur "Auto WAIT"
    if (s.inputType == TutInputType::WAIT || s.inputType == TutInputType::AUTO_DONE) {
        float pct = (float)(millis() - _stepEnter) / s.waitMs;
        DisplayEngine::drawProgressBar(20, 270, TFT_W - 40, 8, pct, COL_TERMINAL, COL_GREY_DARK);
    } else {
        // Flèche "faites-le" qui pulse en bas
        uint8_t flash = ((millis() / 500) % 2) ? 255 : 0;
        DisplayEngine::buf.setTextColor(DisplayEngine::buf.color888(flash, flash, 0));
        DisplayEngine::buf.setTextSize(1);
        DisplayEngine::buf.setCursor(TFT_W / 2 - 30, 278);
        DisplayEngine::buf.print("[ A VOUS ! ]");
    }
}

bool TutorialEngine::_checkInput(const TutStep& s) {
    switch (s.inputType) {
        case TutInputType::JOY_UP:    return JoystickReader::isDir(JoyDir::UP);
        case TutInputType::JOY_DOWN:  return JoystickReader::isDir(JoyDir::DOWN);
        case TutInputType::JOY_LEFT:  return JoystickReader::isDir(JoyDir::LEFT);
        case TutInputType::JOY_RIGHT: return JoystickReader::isDir(JoyDir::RIGHT);
        case TutInputType::JOY_PRESS: {
            auto s2 = JoystickReader::read();
            return s2.pressed;
        }
        case TutInputType::POT_TURN_RIGHT: return PotReader::readSmooth() > 3000;
        case TutInputType::POT_TURN_LEFT:  return PotReader::readSmooth() < 1000;
        case TutInputType::SONAR_NEAR: return SonarReader::readCm() < 30.0f;
        case TutInputType::SONAR_HOLD: {
            float d = SonarReader::readCm();
            bool near = (d >= 8.0f && d <= 18.0f);
            if (near && !_holdActive) { _holdStart = millis(); _holdActive = true; }
            if (!near) _holdActive = false;
            return _holdActive && (millis() - _holdStart > 2000);
        }
        case TutInputType::WAIT:
        case TutInputType::AUTO_DONE:
            return (millis() - _stepEnter) >= s.waitMs;
        case TutInputType::ANY_KEY:
            return false; // Géré par KeypadReader dans les états qui l'utilisent
        default: return false;
    }
}

void TutorialEngine::_onStepSuccess() {
    BuzzerEngine::play(SFX::CONFIRM);
    RGBEngine::setMode(RGBMode::PULSE);
    RGBEngine::setColor(0, 200, 80);  // Flash vert

    // Flash blanc bref sur l'écran
    DisplayEngine::buf.fillScreen(COL_WHITE);
    DisplayEngine::flush();
    vTaskDelay(pdMS_TO_TICKS(80));
}
