#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============================================================================
// System State Machine
// ============================================================================

enum class SystemState {
    IDLE,           // Powered off, waiting for button
    INITIALIZING,   // Starting sensors
    WAIT_FOR_STABLE,// Waiting for platform to stop moving
    LEVELING,       // Actively adjusting motors
    LEVEL_OK,       // Platform is level, monitoring
    ERROR,          // Fault condition
    TEST_MODE,      // Admin/test mode for component testing
    SAFE_SHUTDOWN   // Positions saved, safe to power off
};

// Convert state to string for debugging
inline const char* stateToString(SystemState state) {
    switch (state) {
        case SystemState::IDLE:           return "IDLE";
        case SystemState::INITIALIZING:   return "INITIALIZING";
        case SystemState::WAIT_FOR_STABLE: return "WAIT_FOR_STABLE";
        case SystemState::LEVELING:       return "LEVELING";
        case SystemState::LEVEL_OK:       return "LEVEL_OK";
        case SystemState::ERROR:          return "ERROR";
        case SystemState::TEST_MODE:      return "TEST_MODE";
        case SystemState::SAFE_SHUTDOWN:  return "SAFE_SHUTDOWN";
        default:                          return "UNKNOWN";
    }
}

// ============================================================================
// Button Events
// ============================================================================

enum class ButtonEvent {
    NONE,
    SHORT_PRESS,
    LONG_PRESS
};

// ============================================================================
// LED Patterns
// ============================================================================

enum class LEDPattern {
    OFF,
    SOLID,
    SLOW_BLINK,    // 1 Hz
    FAST_BLINK,    // 4 Hz
    DOUBLE_PULSE,  // Double pulse every 2s
    ERROR_BLINK    // 10 Hz
};

// ============================================================================
// Data Structures
// ============================================================================

// Raw IMU data
struct IMURawData {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    int16_t temperature;
};

// Processed IMU data
struct IMUData {
    float accelX;    // g units
    float accelY;
    float accelZ;
    float gyroX;     // degrees/second
    float gyroY;
    float gyroZ;
    float pitch;     // degrees (filtered)
    float roll;      // degrees (filtered)
    float temperature; // Celsius
};

// Calibration offsets
struct IMUCalibration {
    int16_t accelXOffset;
    int16_t accelYOffset;
    int16_t accelZOffset;
    int16_t gyroXOffset;
    int16_t gyroYOffset;
    int16_t gyroZOffset;
    bool isCalibrated;
};

// PI controller state
struct PIController {
    float kp;
    float ki;
    float integral;
    float lastError;
};

// Motor correction output
struct MotorCorrection {
    int motor1Steps;  // Left back leg
    int motor2Steps;  // Right back leg
};

// System configuration (can be modified via serial)
struct SystemConfig {
    float kpPitch;
    float kiPitch;
    float kpRoll;
    float kiRoll;
    float levelTolerance;
    bool continuousLogging;
};

#endif // TYPES_H
