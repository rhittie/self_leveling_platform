#include "WebDashboard.h"

WebDashboard::WebDashboard() : _server(80), _ws("/ws") {}

bool WebDashboard::begin() {
    // Start WiFi AP
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    delay(100);
    Serial.printf("[WEB] AP started: SSID=%s IP=%s\n",
                  WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());

    // Mount LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("[WEB] ERROR: LittleFS mount failed!");
        return false;
    }
    Serial.println("[WEB] LittleFS mounted");

    // WebSocket event handler
    _ws.onEvent([this](AsyncWebSocket* s, AsyncWebSocketClient* c,
                       AwsEventType t, void* a, uint8_t* d, size_t l) {
        onWebSocketEvent(s, c, t, a, d, l);
    });
    _server.addHandler(&_ws);

    // Serve static files from LittleFS
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    _server.begin();
    Serial.println("[WEB] HTTP server started on port 80");
    return true;
}

void WebDashboard::broadcastStatus(
    float pitch, float roll,
    long m1pos, long m2pos,
    long minPos, long maxPos,
    bool m1limit, bool m2limit,
    const char* state,
    bool isCalibrated, bool isLevel,
    float tolerance
) {
    JsonDocument doc;
    doc["pitch"] = serialized(String(pitch, 2));
    doc["roll"] = serialized(String(roll, 2));
    doc["m1"] = m1pos;
    doc["m2"] = m2pos;
    doc["mMin"] = minPos;
    doc["mMax"] = maxPos;
    doc["m1Lim"] = m1limit;
    doc["m2Lim"] = m2limit;
    doc["state"] = state;
    doc["cal"] = isCalibrated;
    doc["level"] = isLevel;
    doc["tol"] = serialized(String(tolerance, 2));

    char buf[256];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    _ws.textAll(buf, len);
}

void WebDashboard::cleanupClients() {
    _ws.cleanupClients(WS_MAX_CLIENTS);
}

uint8_t WebDashboard::getClientCount() const {
    return _ws.count();
}

void WebDashboard::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WEB] Client #%u connected from %s\n",
                          client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WEB] Client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                handleMessage(client, data, len);
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebDashboard::handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        Serial.printf("[WEB] JSON parse error: %s\n", err.c_str());
        return;
    }

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "motor") == 0 && _motorMoveCb) {
        int id = doc["id"] | 0;
        int steps = doc["steps"] | 0;
        if (id == 1 || id == 2) {
            _motorMoveCb(id, steps);
            Serial.printf("[WEB] Motor %d move %d steps\n", id, steps);
        }
    }
    else if (strcmp(cmd, "mreset") == 0 && _resetPosCb) {
        _resetPosCb();
        Serial.println("[WEB] Motor positions reset");
    }
    else if (strcmp(cmd, "calibrate") == 0 && _calibrateCb) {
        _calibrateCb();
        Serial.println("[WEB] IMU calibration triggered");
    }
    else if (strcmp(cmd, "state") == 0 && _stateChangeCb) {
        const char* to = doc["to"];
        if (to) {
            _stateChangeCb(to);
            Serial.printf("[WEB] State change: %s\n", to);
        }
    }
    else if (strcmp(cmd, "gains") == 0 && _gainCb) {
        float kpP = doc["kpP"] | 1.0f;
        float kiP = doc["kiP"] | 0.05f;
        float kpR = doc["kpR"] | 0.5f;
        float kiR = doc["kiR"] | 0.03f;
        _gainCb(kpP, kiP, kpR, kiR);
        Serial.printf("[WEB] Gains set: kpP=%.2f kiP=%.2f kpR=%.2f kiR=%.2f\n", kpP, kiP, kpR, kiR);
    }
    else if (strcmp(cmd, "tolerance") == 0 && _toleranceCb) {
        float deg = doc["deg"] | 0.5f;
        _toleranceCb(deg);
        Serial.printf("[WEB] Tolerance set: %.2f deg\n", deg);
    }
    else if (strcmp(cmd, "release") == 0 && _releaseCb) {
        _releaseCb();
        Serial.println("[WEB] Motors released");
    }
}
