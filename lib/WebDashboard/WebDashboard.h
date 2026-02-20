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
    );

    // Send a log message to the serial terminal tab on all clients
    void sendLog(const char* msg);

    void cleanupClients();
    uint8_t getClientCount() const;

    // Callback types
    using MotorMoveCallback = std::function<void(int motor, int steps)>;
    using BothMotorsCallback = std::function<void(int steps1, int steps2)>;
    using VoidCallback = std::function<void()>;
    using StateChangeCallback = std::function<void(const char* state)>;
    using GainCallback = std::function<void(float kpP, float kiP, float kpR, float kiR)>;
    using FloatCallback = std::function<void(float val)>;
    using IntCallback = std::function<void(int val)>;
    using LongCallback = std::function<void(long val)>;
    using StringCallback = std::function<void(const char* text)>;
    using LedCallback = std::function<void(const char* mode)>;
    using MotorToggleCallback = std::function<void(int motor)>;
    using LimitsCallback = std::function<void(long min, long max)>;

    void onMotorMove(MotorMoveCallback cb)       { _motorMoveCb = cb; }
    void onBothMotors(BothMotorsCallback cb)     { _bothMotorsCb = cb; }
    void onCalibrate(VoidCallback cb)            { _calibrateCb = cb; }
    void onResetPositions(VoidCallback cb)       { _resetPosCb = cb; }
    void onResetPosition1(VoidCallback cb)       { _resetPos1Cb = cb; }
    void onResetPosition2(VoidCallback cb)       { _resetPos2Cb = cb; }
    void onSetPositions(LongCallback cb)         { _setPosCb = cb; }
    void onStateChange(StateChangeCallback cb)   { _stateChangeCb = cb; }
    void onSetGains(GainCallback cb)             { _gainCb = cb; }
    void onSetTolerance(FloatCallback cb)        { _toleranceCb = cb; }
    void onSetStabilityTimeout(FloatCallback cb) { _stabTimeoutCb = cb; }
    void onRelease(VoidCallback cb)              { _releaseCb = cb; }
    void onMotorStop(VoidCallback cb)            { _motorStopCb = cb; }
    void onMotorSpeed(IntCallback cb)            { _motorSpeedCb = cb; }
    void onMotorContinuous(MotorToggleCallback cb) { _motorContCb = cb; }
    void onLed(LedCallback cb)                   { _ledCb = cb; }
    void onScan(VoidCallback cb)                 { _scanCb = cb; }
    void onStream(VoidCallback cb)               { _streamCb = cb; }
    void onSerial(StringCallback cb)             { _serialCb = cb; }
    void onUnlockLimits(VoidCallback cb)         { _unlockCb = cb; }
    void onLockLimits(VoidCallback cb)           { _lockCb = cb; }

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;

    MotorMoveCallback _motorMoveCb;
    BothMotorsCallback _bothMotorsCb;
    VoidCallback _calibrateCb;
    VoidCallback _resetPosCb;
    VoidCallback _resetPos1Cb;
    VoidCallback _resetPos2Cb;
    LongCallback _setPosCb;
    StateChangeCallback _stateChangeCb;
    GainCallback _gainCb;
    FloatCallback _toleranceCb;
    FloatCallback _stabTimeoutCb;
    VoidCallback _releaseCb;
    VoidCallback _motorStopCb;
    IntCallback _motorSpeedCb;
    MotorToggleCallback _motorContCb;
    LedCallback _ledCb;
    VoidCallback _scanCb;
    VoidCallback _streamCb;
    StringCallback _serialCb;
    VoidCallback _unlockCb;
    VoidCallback _lockCb;

    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);
    void handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
};

#endif
