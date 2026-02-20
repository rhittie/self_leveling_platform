#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

class WebDashboard {
public:
    WebDashboard();

    bool begin();

    void broadcastStatus(
        float pitch, float roll,
        long m1pos, long m2pos,
        long minPos, long maxPos,
        bool m1limit, bool m2limit,
        const char* state,
        bool isCalibrated, bool isLevel,
        float tolerance
    );

    void cleanupClients();
    uint8_t getClientCount() const;

    // Callback types
    using MotorMoveCallback = std::function<void(int motor, int steps)>;
    using VoidCallback = std::function<void()>;
    using StateChangeCallback = std::function<void(const char* state)>;
    using GainCallback = std::function<void(float kpP, float kiP, float kpR, float kiR)>;
    using ToleranceCallback = std::function<void(float tol)>;

    void onMotorMove(MotorMoveCallback cb)     { _motorMoveCb = cb; }
    void onCalibrate(VoidCallback cb)          { _calibrateCb = cb; }
    void onResetPositions(VoidCallback cb)     { _resetPosCb = cb; }
    void onStateChange(StateChangeCallback cb) { _stateChangeCb = cb; }
    void onSetGains(GainCallback cb)           { _gainCb = cb; }
    void onSetTolerance(ToleranceCallback cb)  { _toleranceCb = cb; }
    void onRelease(VoidCallback cb)            { _releaseCb = cb; }

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;

    MotorMoveCallback _motorMoveCb;
    VoidCallback _calibrateCb;
    VoidCallback _resetPosCb;
    StateChangeCallback _stateChangeCb;
    GainCallback _gainCb;
    ToleranceCallback _toleranceCb;
    VoidCallback _releaseCb;

    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);
    void handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
};

#endif
