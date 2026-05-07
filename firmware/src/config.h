#pragma once
// ============================================================
// config.h — Pinout & Constantes globales
// YD-ESP32-S3 N16R8 | Smart Escape Box
// ============================================================
// Garantir uint8_t, uint16_t, uint32_t avant les enum class
#include <stdint.h>

// ────────────────────────────────────────────────────────────
//  DISPLAY TFT ST7789 (SPI2)
// ────────────────────────────────────────────────────────────
#define PIN_TFT_MOSI    11
#define PIN_TFT_SCLK    12
#define PIN_TFT_CS      10
#define PIN_TFT_DC      13
#define PIN_TFT_RST     14
#define PIN_TFT_BLK     15
#define TFT_W           240
#define TFT_H           320

// ────────────────────────────────────────────────────────────
//  RTC DS3231 (I2C)
// ────────────────────────────────────────────────────────────
#define PIN_RTC_SDA     8
#define PIN_RTC_SCL     9

// ────────────────────────────────────────────────────────────
//  HC-SR04 ULTRASON
// ────────────────────────────────────────────────────────────
#define PIN_SONAR_TRIG  6
#define PIN_SONAR_ECHO  7

// ────────────────────────────────────────────────────────────
//  JOYSTICK ANALOGIQUE
// ────────────────────────────────────────────────────────────
#define PIN_JOY_X       1   // ADC1_CH0
#define PIN_JOY_Y       2   // ADC1_CH1
#define PIN_JOY_SW      42  // Bouton joystick (digital)
#define JOY_DEADZONE    300 // Zone morte sur 4095

// ────────────────────────────────────────────────────────────
//  CLAVIER 4×4
// ────────────────────────────────────────────────────────────
#define KP_ROWS         4
#define KP_COLS         4
// Rangées (sorties)
#define PIN_KP_R1       16
#define PIN_KP_R2       17
#define PIN_KP_R3       18
#define PIN_KP_R4       21
// Colonnes (entrées avec pull-up)
#define PIN_KP_C1       38
#define PIN_KP_C2       39
#define PIN_KP_C3       40
#define PIN_KP_C4       41

// ────────────────────────────────────────────────────────────
//  POTENTIOMÈTRE
// ────────────────────────────────────────────────────────────
#define PIN_POT         4   // ADC1_CH3
#define POT_SMOOTH      32  // Nb échantillons moving average

// ────────────────────────────────────────────────────────────
//  SERVO SG90 (LEDC PWM) — alimenté sur 5V Boost Converter
// ────────────────────────────────────────────────────────────
#define PIN_SERVO       5
#define SERVO_CH        0
#define SERVO_FREQ      50
#define SERVO_LOCKED    10   // Degrés — position verrouillée
#define SERVO_OPEN      90   // Degrés — boîte ouverte

// ────────────────────────────────────────────────────────────
//  BUZZER PIÉZO (LEDC PWM)
// ────────────────────────────────────────────────────────────
#define PIN_BUZZER      47
#define BUZZER_CH       1

// ────────────────────────────────────────────────────────────
//  MODULE RGB LED (PWM)
// ────────────────────────────────────────────────────────────
#define PIN_RGB_R       45
#define PIN_RGB_G       46
#define PIN_RGB_B       48
#define RGB_CH_R        2
#define RGB_CH_G        3
#define RGB_CH_B        4
#define RGB_PWM_FREQ    5000
#define RGB_PWM_RES     8   // 8-bit → 0-255

// ────────────────────────────────────────────────────────────
//  RÉSEAU WiFi SoftAP
// ────────────────────────────────────────────────────────────
#define WIFI_SSID       "EscapeBox"
#define WIFI_PASS       "unlock2025"
#define WIFI_CHANNEL    6
#define SERVER_PORT     80
#define WS_PATH         "/ws"

// ────────────────────────────────────────────────────────────
//  TÂCHES FREERTOS
// ────────────────────────────────────────────────────────────
#define TASK_STACK_DISPLAY   8192
#define TASK_STACK_GAME      8192
#define TASK_STACK_SENSORS   4096
#define TASK_STACK_WIFI      8192
#define TASK_STACK_BUZZER    2048

#define TASK_PRIO_DISPLAY    4
#define TASK_PRIO_GAME       3
#define TASK_PRIO_SENSORS    2
#define TASK_PRIO_WIFI       5
#define TASK_PRIO_BUZZER     2

// ────────────────────────────────────────────────────────────
//  COULEURS (RGB565 pour LovyanGFX)
// ────────────────────────────────────────────────────────────
#define COL_BLACK       0x0000
#define COL_WHITE       0xFFFF
#define COL_TERMINAL    0x07E0  // Vert phosphorescent
#define COL_DANGER      0xF800  // Rouge vif
#define COL_GOLD        0xFEA0  // Doré
#define COL_CYAN_DARK   0x0410  // Cyan sombre
#define COL_VIOLET      0x780F  // Violet profond
#define COL_ORANGE      0xFC60  // Orange
#define COL_GREY_DARK   0x2104  // Gris très sombre
#define COL_GREY        0x8410  // Gris moyen

// ────────────────────────────────────────────────────────────
//  NOTES DE MUSIQUE (Hz) pour BuzzerEngine
// ────────────────────────────────────────────────────────────
#define NOTE_C3  131
#define NOTE_D3  147
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_REST 0

// Constantes Quiz
#define QUIZ_MAX_ATTEMPTS    3     // Tentatives avant revelation
#define QUIZ_TRANSITION_MS   2500  // Pause affichage resultat

// ────────────────────────────────────────────────────────────
//  PROFILS JOUEUR
// ────────────────────────────────────────────────────────────
enum class GameProfile : uint8_t {
    PUBLIC   = 0,  // Famille / Enfants
    ADVANCED = 1,  // Terminale / BTS
    EXPERT   = 2   // Ingénieurs / Enseignants
};

// ────────────────────────────────────────────────────────────
//  MODE DE JEU + FSM
// ────────────────────────────────────────────────────────────
enum class GameMode : uint8_t {
    ESCAPE_GAME = 0,
    QUIZ_MODE   = 1
};

// ────────────────────────────────────────────────────────────
//  RESULTAT DE L'INTRO (profil + mode choisis)
// ────────────────────────────────────────────────────────────
struct IntroResult {
    GameProfile profile;
    GameMode    mode;
};

// ────────────────────────────────────────────────────────────
//  ETATS FSM PRINCIPAUX
// ────────────────────────────────────────────────────────────
enum class GameState : uint8_t {
    BOOT = 0,
    INTRO,
    PROFILE_SELECT,
    ENIGMA_1,
    INTERLUDE_1,      // Quiz obligatoire apres enigme 1
    ENIGMA_2,
    INTERLUDE_2,      // Quiz obligatoire apres enigme 2
    ENIGMA_3,
    INTERLUDE_3,      // Quiz obligatoire apres enigme 3
    ENIGMA_4,
    INTERLUDE_4,      // Quiz obligatoire apres enigme 4
    ENIGMA_5,
    VICTORY,
    QUIZ_CHALLENGE,   // Mode defi autonome
    GAME_OVER
};

