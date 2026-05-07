#pragma once
// ============================================================
// network/EscapeWebServer.h
// HTTP + WebSocket async — gestion du jeu + quiz
// ============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "../quiz/QuizEngine.h"

class EscapeWebServer {
public:
    static void init();

    // ── Jeu ──────────────────────────────────────────────────
    static void setProfile(GameProfile p);
    static void setMode(GameMode m);
    static void notifyEnigmaStart(uint8_t n);
    static void notifyEnigmaSolved(uint8_t n);
    static void notifyVictory();

    // ── Quiz ─────────────────────────────────────────────────
    static void notifyQuizQuestion(const QuizQuestion& q, const char* domain);
    static void notifyQuizResult(bool correct, uint8_t correctIdx);

    // ── Utilitaires ──────────────────────────────────────────
    static void broadcastJson(const char* key, const char* value);
    static void broadcastJson(const char* key, int value);
    static void broadcastDoc(JsonDocument& doc);

    // ── Etat public accessible ────────────────────────────────
    static volatile bool     clientConnected;
    static volatile uint8_t  currentEnigma;
    static volatile uint32_t sessionCode;

private:
    static AsyncWebServer _server;
    static AsyncWebSocket _ws;
    static GameProfile    _profile;
    static GameMode       _mode;

    static void _onWsEvent(AsyncWebSocket* srv, AsyncWebSocketClient* client,
                            AwsEventType type, void* arg, uint8_t* data, size_t len);
    static void _sendState(AsyncWebSocketClient* client);
};
