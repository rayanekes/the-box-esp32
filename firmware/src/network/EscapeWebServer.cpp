// ============================================================
// network/EscapeWebServer.cpp
// ============================================================
#include "EscapeWebServer.h"

// ── Membres statiques ─────────────────────────────────────────
AsyncWebServer   EscapeWebServer::_server(SERVER_PORT);
AsyncWebSocket   EscapeWebServer::_ws(WS_PATH);
GameProfile      EscapeWebServer::_profile     = GameProfile::ADVANCED;
GameMode         EscapeWebServer::_mode        = GameMode::ESCAPE_GAME;
volatile bool    EscapeWebServer::clientConnected = false;
volatile uint8_t EscapeWebServer::currentEnigma  = 0;
volatile uint32_t EscapeWebServer::sessionCode   = 0;

// ── Init SoftAP + serveur ────────────────────────────────────
void EscapeWebServer::init() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS, WIFI_CHANNEL, 0, 4);

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[NET] SoftAP: %s | IP: %s\n", WIFI_SSID, ip.toString().c_str());

    _ws.onEvent(_onWsEvent);
    _server.addHandler(&_ws);

    // Servir la Web App depuis LittleFS
    _server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");

    // API REST legere
    _server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["enigma"]   = EscapeWebServer::currentEnigma;
        doc["profile"]  = (int)EscapeWebServer::_profile;
        doc["mode"]     = (int)EscapeWebServer::_mode;
        doc["code"]     = EscapeWebServer::sessionCode;
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    _server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not Found");
    });

    _server.begin();

    // Tache de nettoyage WebSocket (Core 0)
    xTaskCreatePinnedToCore([](void*) {
        while (true) {
            EscapeWebServer::_ws.cleanupClients();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }, "ws_cleanup", 2048, nullptr, 1, nullptr, 0);
}

// ── Handler WebSocket ─────────────────────────────────────────
void EscapeWebServer::_onWsEvent(AsyncWebSocket* srv,
                                   AsyncWebSocketClient* client,
                                   AwsEventType type, void* arg,
                                   uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            clientConnected = true;
            Serial.printf("[WS] Client #%u connecte\n", client->id());
            _sendState(client);
            break;

        case WS_EVT_DISCONNECT:
            clientConnected = (srv->count() > 0);
            Serial.printf("[WS] Client #%u deconnecte\n", client->id());
            break;

        case WS_EVT_DATA: {
            // Messages entrants (reponses quiz depuis le telephone)
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len &&
                info->opcode == WS_TEXT) {
                data[len] = 0;
                JsonDocument doc;
                if (!deserializeJson(doc, data)) {
                    // Pour l'instant on logue - la reponse est traitee cote firmware
                    if (doc["type"] == "quiz_answer") {
                        Serial.printf("[WS] Quiz answer from phone: %d\n", (int)doc["choice"]);
                    }
                }
            }
            break;
        }

        default: break;
    }
}

// ── Envoi etat complet au client ──────────────────────────────
void EscapeWebServer::_sendState(AsyncWebSocketClient* client) {
    JsonDocument doc;
    doc["type"]    = "state";
    doc["enigma"]  = currentEnigma;
    doc["profile"] = (int)_profile;
    doc["mode"]    = (int)_mode;
    doc["code"]    = sessionCode;
    String msg;
    serializeJson(doc, msg);
    client->text(msg);
}

// ── API publique : jeu ────────────────────────────────────────
void EscapeWebServer::setProfile(GameProfile p) {
    _profile = p;
    broadcastJson("profile", (int)p);
}

void EscapeWebServer::setMode(GameMode m) {
    _mode = m;
    JsonDocument doc;
    doc["type"] = "update";
    doc["mode"] = (int)m;
    broadcastDoc(doc);
}

void EscapeWebServer::notifyEnigmaStart(uint8_t n) {
    currentEnigma = n;
    JsonDocument doc;
    doc["type"]   = "update";
    doc["enigma"] = n;
    doc["phase"]  = "start";
    broadcastDoc(doc);
}

void EscapeWebServer::notifyEnigmaSolved(uint8_t n) {
    currentEnigma = n;
    JsonDocument doc;
    doc["type"]   = "update";
    doc["solved"] = n;
    broadcastDoc(doc);
}

void EscapeWebServer::notifyVictory() {
    currentEnigma = 99;
    broadcastJson("event", "victory");
}

// ── API publique : quiz ───────────────────────────────────────
void EscapeWebServer::notifyQuizQuestion(const QuizQuestion& q, const char* domain) {
    JsonDocument doc;
    doc["type"]       = "quiz_question";
    doc["domain"]     = domain;
    doc["question"]   = q.question;
    doc["difficulty"] = q.difficulty;
    doc["numChoices"] = q.numChoices;
    doc["correctIdx"] = q.correctIdx;

    JsonArray choices = doc["choices"].to<JsonArray>();
    for (uint8_t i = 0; i < q.numChoices; i++) {
        choices.add(q.choices[i]);
    }

    if (strlen(q.explanation) > 0) {
        doc["explanation"] = q.explanation;
    }

    broadcastDoc(doc);
}

void EscapeWebServer::notifyQuizResult(bool correct, uint8_t correctIdx) {
    JsonDocument doc;
    doc["type"]       = "quiz_result";
    doc["correct"]    = correct;
    doc["correctIdx"] = correctIdx;
    broadcastDoc(doc);
}

// ── Broadcast helpers ─────────────────────────────────────────
void EscapeWebServer::broadcastJson(const char* key, const char* value) {
    JsonDocument doc;
    doc["type"] = "update";
    doc[key]    = value;
    String msg;
    serializeJson(doc, msg);
    _ws.textAll(msg);
}

void EscapeWebServer::broadcastJson(const char* key, int value) {
    JsonDocument doc;
    doc["type"] = "update";
    doc[key]    = value;
    String msg;
    serializeJson(doc, msg);
    _ws.textAll(msg);
}

void EscapeWebServer::broadcastDoc(JsonDocument& doc) {
    String msg;
    serializeJson(doc, msg);
    _ws.textAll(msg);
}
