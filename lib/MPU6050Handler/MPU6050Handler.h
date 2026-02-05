#ifndef MPU6050_HANDLER_H
#define MPU6050_HANDLER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "types.h"

/**
 * MPU6050Handler - Handles IMU communication, filtering, and motion detection
 *
 * Uses complementary filter to combine accelerometer (absolute reference but noisy)
 * with gyroscope (smooth but drifts over time) for stable angle estimation.
 */
class MPU6050Handler {
public:
    MPU6050Handler();

    /**
     * Initialize the MPU6050 sensor
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Update sensor readings and apply filtering
     * Call this at regular intervals (e.g., 100 Hz)
     */
    void update();

    /**
     * Run calibration routine - platform must be stationary and level
     * @return true if calibration successful
     */
    bool calibrate();

    /**
     * Check if platform is currently in motion
     * @return true if motion detected
     */
    bool isMoving() const;

    /**
     * Check if platform is within level tolerance
     * @param tolerance Maximum acceptable angle deviation in degrees
     * @return true if level
     */
    bool isLevel(float tolerance) const;

    /**
     * Get current processed IMU data
     */
    const IMUData& getData() const { return _data; }

    /**
     * Get raw IMU data for debugging
     */
    const IMURawData& getRawData() const { return _rawData; }

    /**
     * Get calibration data
     */
    const IMUCalibration& getCalibration() const { return _calibration; }

    /**
     * Get current pitch angle (degrees)
     */
    float getPitch() const { return _data.pitch; }

    /**
     * Get current roll angle (degrees)
     */
    float getRoll() const { return _data.roll; }

private:
    IMURawData _rawData;
    IMUData _data;
    IMUCalibration _calibration;

    unsigned long _lastUpdateTime;
    float _accelPitch;  // Angle from accelerometer only
    float _accelRoll;

    // Motion detection
    float _lastAccelMagnitude;
    bool _isMoving;

    /**
     * Read raw data from sensor
     */
    void readRawData();

    /**
     * Apply calibration offsets and convert to physical units
     */
    void processData();

    /**
     * Apply complementary filter to calculate pitch and roll
     */
    void applyComplementaryFilter(float dt);

    /**
     * Detect motion based on acceleration changes and gyro readings
     */
    void detectMotion();

    /**
     * Write a byte to an MPU6050 register
     */
    void writeRegister(uint8_t reg, uint8_t value);

    /**
     * Read bytes from MPU6050 registers
     */
    void readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
};

#endif // MPU6050_HANDLER_H
