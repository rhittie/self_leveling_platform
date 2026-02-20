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
    float accelX, float accelY, float accelZ,
    float gyroX, float gyroY, float gyroZ,
    float temperature,
    long m1pos, long m2pos,
    long minPos, long maxPos,
    bool m1limit, bool m2limit,
    const char* state,
    bool isCalibrated, bool isLevel,
    float tolerance,
    unsigned long stabilityTimeoutMs,
    float kpPitch, float kiPitch, float kpRoll, float kiRoll,
    unsigned long uptime
) {
    JsonDocument doc;
    doc["t"] = "status";
    doc["pitch"] = serialized(String(pitch, 2));
    doc["roll"] = serialized(String(roll, 2));
    doc["ax"] = serialized(String(accelX, 3));
    doc["ay"] = serialized(String(accelY, 3));
    doc["az"] = serialized(String(accelZ, 3));
    doc["gx"] = serialized(String(gyroX, 1));
    doc["gy"] = serialized(String(gyroY, 1));
    doc["gz"] = serialized(String(gyroZ, 1));
    doc["temp"] = serialized(String(temperature, 1));
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
    doc["stMs"] = stabilityTimeoutMs;
    doc["kpP"] = serialized(String(kpPitch, 2));
    doc["kiP"] = serialized(String(kiPitch, 3));
    doc["kpR"] = serialized(String(kpRoll, 2));
    doc["kiR"] = serialized(String(kiRoll, 3));
    doc["up"] = uptime;

    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    _ws.textAll(buf, len);
}

void WebDashboard::sendLog(const char* msg) {
    JsonDocument doc;
    doc["t"] = "log";
    doc["msg"] = msg;
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

    // Motor move: {"cmd":"motor","id":1,"steps":100}
    if (strcmp(cmd, "motor") == 0 && _motorMoveCb) {
        int id = doc["id"] | 0;
        int steps = doc["steps"] | 0;
        if (id == 1 || id == 2) {
            _motorMoveCb(id, steps);
        }
    }
    // Both motors: {"cmd":"both","m1":100,"m2":-100}
    else if (strcmp(cmd, "both") == 0 && _bothMotorsCb) {
        int s1 = doc["m1"] | 0;
        int s2 = doc["m2"] | 0;
        _bothMotorsCb(s1, s2);
    }
    // Motor stop: {"cmd":"mstop"}
    else if (strcmp(cmd, "mstop") == 0 && _motorStopCb) {
        _motorStopCb();
    }
    // Motor speed: {"cmd":"mspeed","value":10}
    else if (strcmp(cmd, "mspeed") == 0 && _motorSpeedCb) {
        int rpm = doc["value"] | 10;
        _motorSpeedCb(rpm);
    }
    // Motor continuous: {"cmd":"mcont","id":1}
    else if (strcmp(cmd, "mcont") == 0 && _motorContCb) {
        int id = doc["id"] | 0;
        _motorContCb(id);
    }
    // Reset both positions: {"cmd":"mreset"}
    else if (strcmp(cmd, "mreset") == 0 && _resetPosCb) {
        _resetPosCb();
    }
    // Reset motor 1: {"cmd":"mreset1"}
    else if (strcmp(cmd, "mreset1") == 0 && _resetPos1Cb) {
        _resetPos1Cb();
    }
    // Reset motor 2: {"cmd":"mreset2"}
    else if (strcmp(cmd, "mreset2") == 0 && _resetPos2Cb) {
        _resetPos2Cb();
    }
    // Set both positions: {"cmd":"mset","value":0}
    else if (strcmp(cmd, "mset") == 0 && _setPosCb) {
        long val = doc["value"] | 0L;
        _setPosCb(val);
    }
    // Unlock motor limits: {"cmd":"munlock"}
    else if (strcmp(cmd, "munlock") == 0 && _unlockCb) {
        _unlockCb();
    }
    // Lock motor limits: {"cmd":"mlock"}
    else if (strcmp(cmd, "mlock") == 0 && _lockCb) {
        _lockCb();
    }
    // Calibrate IMU: {"cmd":"calibrate"}
    else if (strcmp(cmd, "calibrate") == 0 && _calibrateCb) {
        _calibrateCb();
    }
    // I2C scan: {"cmd":"scan"}
    else if (strcmp(cmd, "scan") == 0 && _scanCb) {
        _scanCb();
    }
    // Stream toggle: {"cmd":"stream"}
    else if (strcmp(cmd, "stream") == 0 && _streamCb) {
        _streamCb();
    }
    // State change: {"cmd":"state","to":"IDLE"}
    else if (strcmp(cmd, "state") == 0 && _stateChangeCb) {
        const char* to = doc["to"];
        if (to) _stateChangeCb(to);
    }
    // Set PI gains: {"cmd":"gains","kpP":1.0,"kiP":0.05,"kpR":0.5,"kiR":0.03}
    else if (strcmp(cmd, "gains") == 0 && _gainCb) {
        float kpP = doc["kpP"] | 1.0f;
        float kiP = doc["kiP"] | 0.05f;
        float kpR = doc["kpR"] | 0.5f;
        float kiR = doc["kiR"] | 0.03f;
        _gainCb(kpP, kiP, kpR, kiR);
    }
    // Set tolerance: {"cmd":"tolerance","deg":0.5}
    else if (strcmp(cmd, "tolerance") == 0 && _toleranceCb) {
        float deg = doc["deg"] | 0.5f;
        _toleranceCb(deg);
    }
    // Set stability timeout: {"cmd":"stabTimeout","sec":3.0}
    else if (strcmp(cmd, "stabTimeout") == 0 && _stabTimeoutCb) {
        float sec = doc["sec"] | 3.0f;
        _stabTimeoutCb(sec);
    }
    // LED control: {"cmd":"led","mode":"red"}
    else if (strcmp(cmd, "led") == 0 && _ledCb) {
        const char* mode = doc["mode"];
        if (mode) _ledCb(mode);
    }
    // Release motors: {"cmd":"release"}
    else if (strcmp(cmd, "release") == 0 && _releaseCb) {
        _releaseCb();
    }
    // Raw serial passthrough: {"cmd":"serial","text":"mpos"}
    else if (strcmp(cmd, "serial") == 0 && _serialCb) {
        const char* text = doc["text"];
        if (text) _serialCb(text);
    }
}
