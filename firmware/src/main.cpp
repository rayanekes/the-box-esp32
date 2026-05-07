// ============================================================
// main.cpp — Point d'entree Smart Escape Box
// Initialisation + creation des taches FreeRTOS
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Keypad.h>
#include <RTClib.h>
#include <LittleFS.h>

#include "config.h"
#include "engine/DisplayEngine.h"
#include "engine/BuzzerEngine.h"
#include "engine/RGBEngine.h"
#include "sensors/JoystickReader.h"
#include "sensors/SonarReader.h"
#include "sensors/PotReader.h"
#include "sensors/ServoController.h"
#include "fsm/states/StateIntro.h"
#include "fsm/states/StateEnigma1.h"
#include "fsm/states/StateEnigma2.h"
#include "fsm/states/StateEnigma3.h"
#include "fsm/states/StateEnigma4.h"
#include "fsm/states/StateEnigma5.h"
#include "fsm/states/StateVictory.h"
#include "fsm/states/StateInterlude.h"
#include "network/EscapeWebServer.h"

// ── Instances globales ────────────────────────────────────────
RTC_DS3231  rtc;
uint32_t    g_sessionStartTime = 0;
GameProfile g_profile          = GameProfile::ADVANCED;
GameMode    g_mode             = GameMode::ESCAPE_GAME;
GameState   g_state            = GameState::BOOT;

// Keypad 4x4
const char KEYPAD_KEYS[KP_ROWS][KP_COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
byte rowPins[KP_ROWS] = {PIN_KP_R1, PIN_KP_R2, PIN_KP_R3, PIN_KP_R4};
byte colPins[KP_COLS] = {PIN_KP_C1, PIN_KP_C2, PIN_KP_C3, PIN_KP_C4};
Keypad keypad = Keypad(makeKeymap(KEYPAD_KEYS), rowPins, colPins, KP_ROWS, KP_COLS);

// Enigmes (instanciees sur la heap pour eviter la pile)
static StateEnigma1* enigma1;
static StateEnigma2* enigma2;
static StateEnigma3* enigma3;
static StateEnigma4* enigma4;
static StateEnigma5* enigma5;

// ── Tache principale de jeu (Core 1) ─────────────────────────
void taskGame(void* pvParams) {
    vTaskDelay(pdMS_TO_TICKS(500));

    while (true) {
        switch (g_state) {

        case GameState::BOOT:
            g_state = GameState::INTRO;
            break;

        case GameState::INTRO: {
            IntroResult res = StateIntro::run();
            g_profile = res.profile;
            g_mode    = res.mode;

            DateTime now = rtc.now();
            g_sessionStartTime = now.unixtime();

            EscapeWebServer::setProfile(g_profile);
            EscapeWebServer::setMode(g_mode);

            if (g_mode == GameMode::QUIZ_MODE) {
                g_state = GameState::QUIZ_CHALLENGE;
            } else {
                g_state = GameState::ENIGMA_1;
            }
            break;
        }

        // ── Escape Game ───────────────────────────────────────

        case GameState::ENIGMA_1:
            EscapeWebServer::notifyEnigmaStart(1);
            enigma1->onEnter(g_profile);
            while (!enigma1->update()) vTaskDelay(pdMS_TO_TICKS(16));
            enigma1->onExit();
            EscapeWebServer::notifyEnigmaSolved(1);
            g_state = GameState::INTERLUDE_1;
            break;

        case GameState::INTERLUDE_1:
            StateInterlude::run(1, g_profile);
            g_state = GameState::ENIGMA_2;
            break;

        case GameState::ENIGMA_2:
            EscapeWebServer::notifyEnigmaStart(2);
            enigma2->onEnter(g_profile);
            while (!enigma2->update()) vTaskDelay(pdMS_TO_TICKS(16));
            enigma2->onExit();
            EscapeWebServer::notifyEnigmaSolved(2);
            g_state = GameState::INTERLUDE_2;
            break;

        case GameState::INTERLUDE_2:
            StateInterlude::run(2, g_profile);
            g_state = GameState::ENIGMA_3;
            break;

        case GameState::ENIGMA_3:
            EscapeWebServer::notifyEnigmaStart(3);
            enigma3->onEnter(g_profile);
            while (!enigma3->update()) vTaskDelay(pdMS_TO_TICKS(16));
            enigma3->onExit();
            EscapeWebServer::notifyEnigmaSolved(3);
            g_state = GameState::INTERLUDE_3;
            break;

        case GameState::INTERLUDE_3:
            StateInterlude::run(3, g_profile);
            g_state = GameState::ENIGMA_4;
            break;

        case GameState::ENIGMA_4:
            EscapeWebServer::notifyEnigmaStart(4);
            enigma4->onEnter(g_profile);
            while (!enigma4->update()) vTaskDelay(pdMS_TO_TICKS(16));
            enigma4->onExit();
            EscapeWebServer::notifyEnigmaSolved(4);
            g_state = GameState::INTERLUDE_4;
            break;

        case GameState::INTERLUDE_4:
            StateInterlude::run(4, g_profile);
            g_state = GameState::ENIGMA_5;
            break;

        case GameState::ENIGMA_5:
            EscapeWebServer::notifyEnigmaStart(5);
            enigma5->onEnter(g_profile);
            while (!enigma5->update()) vTaskDelay(pdMS_TO_TICKS(16));
            enigma5->onExit();
            EscapeWebServer::notifyEnigmaSolved(5);
            g_state = GameState::VICTORY;
            break;

        case GameState::VICTORY:
            StateVictory::run(g_profile);
            EscapeWebServer::notifyVictory();
            g_state = GameState::INTRO;
            break;

        // ── Mode Quiz autonome ────────────────────────────────
        case GameState::QUIZ_CHALLENGE:
            // Le mode quiz tourne en boucle jusqu'au retour au menu
            // StateInterlude::run() avec levelJustCompleted=0 = mode libre
            // On alterne les domaines aleatoirement
            for (int i = 0; i < 10; i++) {
                StateInterlude::run(
                    (i % 4) + 1,  // Cycle les domaines 1->4
                    g_profile
                );
            }
            g_state = GameState::INTRO;
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// ── setup() ──────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("[ESCAPE BOX] Boot...");

    Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);
    if (!rtc.begin()) {
        Serial.println("[WARN] RTC non detecte - timestamp simule");
    }

    if (!LittleFS.begin(true)) {
        Serial.println("[ERROR] LittleFS mount failed !");
    }

    // Init hardware
    DisplayEngine::init();
    BuzzerEngine::init();
    RGBEngine::init();
    JoystickReader::init();
    SonarReader::init();
    PotReader::init();
    ServoController::init();

    // Enigmes en PSRAM
    enigma1 = new StateEnigma1();
    enigma2 = new StateEnigma2();
    enigma3 = new StateEnigma3();
    enigma4 = new StateEnigma4();
    enigma5 = new StateEnigma5();

    // Serveur Web + WebSocket (Core 0)
    EscapeWebServer::init();

    // Tache principale (Core 1)
    xTaskCreatePinnedToCore(
        taskGame, "task_game",
        TASK_STACK_GAME, nullptr,
        TASK_PRIO_GAME, nullptr, 1
    );

    Serial.println("[ESCAPE BOX] Pret !");
}

// loop() vide - tout tourne dans les taches FreeRTOS
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
