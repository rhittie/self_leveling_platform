#include "MPU6050Handler.h"

// MPU6050 Register addresses
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_WHO_AM_I     0x75

// Conversion factors
#define ACCEL_SCALE_FACTOR 16384.0f  // ±2g range
#define GYRO_SCALE_FACTOR 131.0f     // ±250°/s range
// RAD_TO_DEG is already defined in Arduino.h

MPU6050Handler::MPU6050Handler()
    : _lastUpdateTime(0)
    , _accelPitch(0)
    , _accelRoll(0)
    , _lastAccelMagnitude(1.0f)
    , _isMoving(false)
{
    memset(&_rawData, 0, sizeof(_rawData));
    memset(&_data, 0, sizeof(_data));
    memset(&_calibration, 0, sizeof(_calibration));
    _calibration.isCalibrated = false;
}

bool MPU6050Handler::begin() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);  // 400 kHz I2C

    // Check if MPU6050 is responding
    Wire.beginTransmission(MPU6050_ADDRESS);
    if (Wire.endTransmission() != 0) {
        Serial.println("MPU6050: No response on I2C bus");
        return false;
    }

    // Verify WHO_AM_I register (should be 0x68)
    uint8_t whoAmI;
    readRegisters(MPU6050_REG_WHO_AM_I, &whoAmI, 1);
    if (whoAmI != 0x68) {
        Serial.printf("MPU6050: Unexpected WHO_AM_I value: 0x%02X\n", whoAmI);
        return false;
    }

    // Wake up the MPU6050 (clear sleep bit)
    writeRegister(MPU6050_REG_PWR_MGMT_1, 0x00);
    delay(100);

    // Set sample rate divider (1kHz / (1 + 9) = 100Hz)
    writeRegister(MPU6050_REG_SMPLRT_DIV, 0x09);

    // Set digital low-pass filter (bandwidth ~44Hz)
    writeRegister(MPU6050_REG_CONFIG, 0x03);

    // Set gyroscope range to ±250°/s
    writeRegister(MPU6050_REG_GYRO_CONFIG, 0x00);

    // Set accelerometer range to ±2g
    writeRegister(MPU6050_REG_ACCEL_CONFIG, 0x00);

    _lastUpdateTime = millis();

    Serial.println("MPU6050: Initialized successfully");
    return true;
}

void MPU6050Handler::update() {
    unsigned long currentTime = millis();
    float dt = (currentTime - _lastUpdateTime) / 1000.0f;
    _lastUpdateTime = currentTime;

    // Clamp dt to reasonable values (prevent issues on first call or delays)
    if (dt <= 0 || dt > 0.5f) {
        dt = 0.01f;
    }

    readRawData();
    processData();
    applyComplementaryFilter(dt);
    detectMotion();
}

bool MPU6050Handler::calibrate() {
    Serial.println("MPU6050: Starting calibration - keep platform still and level...");

    long accelXSum = 0, accelYSum = 0, accelZSum = 0;
    long gyroXSum = 0, gyroYSum = 0, gyroZSum = 0;

    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        readRawData();

        accelXSum += _rawData.accelX;
        accelYSum += _rawData.accelY;
        accelZSum += _rawData.accelZ;
        gyroXSum += _rawData.gyroX;
        gyroYSum += _rawData.gyroY;
        gyroZSum += _rawData.gyroZ;

        delay(10);

        // Progress indicator
        if ((i + 1) % 50 == 0) {
            Serial.printf("Calibration: %d/%d samples\n", i + 1, CALIBRATION_SAMPLES);
        }
    }

    // Calculate averages
    _calibration.accelXOffset = accelXSum / CALIBRATION_SAMPLES;
    _calibration.accelYOffset = accelYSum / CALIBRATION_SAMPLES;
    // Z axis should read 1g when level, so offset is average - 16384
    _calibration.accelZOffset = (accelZSum / CALIBRATION_SAMPLES) - ACCEL_SCALE_FACTOR;
    _calibration.gyroXOffset = gyroXSum / CALIBRATION_SAMPLES;
    _calibration.gyroYOffset = gyroYSum / CALIBRATION_SAMPLES;
    _calibration.gyroZOffset = gyroZSum / CALIBRATION_SAMPLES;
    _calibration.isCalibrated = true;

    Serial.println("MPU6050: Calibration complete");
    Serial.printf("  Accel offsets: X=%d, Y=%d, Z=%d\n",
                  _calibration.accelXOffset, _calibration.accelYOffset, _calibration.accelZOffset);
    Serial.printf("  Gyro offsets: X=%d, Y=%d, Z=%d\n",
                  _calibration.gyroXOffset, _calibration.gyroYOffset, _calibration.gyroZOffset);

    // Reset filtered angles
    _data.pitch = 0;
    _data.roll = 0;

    return true;
}

bool MPU6050Handler::isMoving() const {
    return _isMoving;
}

bool MPU6050Handler::isLevel(float tolerance) const {
    return (fabs(_data.pitch) < tolerance) && (fabs(_data.roll) < tolerance);
}

void MPU6050Handler::readRawData() {
    uint8_t buffer[14];
    readRegisters(MPU6050_REG_ACCEL_XOUT_H, buffer, 14);

    // Combine high and low bytes (big-endian format)
    _rawData.accelX = (buffer[0] << 8) | buffer[1];
    _rawData.accelY = (buffer[2] << 8) | buffer[3];
    _rawData.accelZ = (buffer[4] << 8) | buffer[5];
    _rawData.temperature = (buffer[6] << 8) | buffer[7];
    _rawData.gyroX = (buffer[8] << 8) | buffer[9];
    _rawData.gyroY = (buffer[10] << 8) | buffer[11];
    _rawData.gyroZ = (buffer[12] << 8) | buffer[13];
}

void MPU6050Handler::processData() {
    // Apply calibration offsets
    int16_t ax = _rawData.accelX - _calibration.accelXOffset;
    int16_t ay = _rawData.accelY - _calibration.accelYOffset;
    int16_t az = _rawData.accelZ - _calibration.accelZOffset;
    int16_t gx = _rawData.gyroX - _calibration.gyroXOffset;
    int16_t gy = _rawData.gyroY - _calibration.gyroYOffset;
    int16_t gz = _rawData.gyroZ - _calibration.gyroZOffset;

    // Convert to physical units
    _data.accelX = ax / ACCEL_SCALE_FACTOR;
    _data.accelY = ay / ACCEL_SCALE_FACTOR;
    _data.accelZ = az / ACCEL_SCALE_FACTOR;
    _data.gyroX = gx / GYRO_SCALE_FACTOR;
    _data.gyroY = gy / GYRO_SCALE_FACTOR;
    _data.gyroZ = gz / GYRO_SCALE_FACTOR;

    // Temperature: (raw / 340) + 36.53
    _data.temperature = (_rawData.temperature / 340.0f) + 36.53f;

    // Calculate angles from accelerometer (absolute reference)
    // Pitch: rotation around X axis (nose up/down)
    // Roll: rotation around Y axis (left/right tilt)
    _accelPitch = atan2(_data.accelY, sqrt(_data.accelX * _data.accelX + _data.accelZ * _data.accelZ)) * RAD_TO_DEG;
    _accelRoll = atan2(-_data.accelX, _data.accelZ) * RAD_TO_DEG;

    // Apply axis inversion if configured
    if (INVERT_PITCH) _accelPitch = -_accelPitch;
    if (INVERT_ROLL) _accelRoll = -_accelRoll;
}

void MPU6050Handler::applyComplementaryFilter(float dt) {
    // Complementary filter combines:
    // - Accelerometer: absolute reference but noisy
    // - Gyroscope: smooth but drifts over time
    //
    // filtered_angle = alpha * accel_angle + (1 - alpha) * (prev_angle + gyro_rate * dt)

    // Integrate gyroscope rates (apply inversion to match accel axes)
    float gyroRatePitch = INVERT_PITCH ? -_data.gyroX : _data.gyroX;
    float gyroRateRoll = INVERT_ROLL ? -_data.gyroY : _data.gyroY;
    float gyroPitch = _data.pitch + gyroRatePitch * dt;
    float gyroRoll = _data.roll + gyroRateRoll * dt;

    // Apply complementary filter
    _data.pitch = COMPLEMENTARY_ALPHA * _accelPitch + (1.0f - COMPLEMENTARY_ALPHA) * gyroPitch;
    _data.roll = COMPLEMENTARY_ALPHA * _accelRoll + (1.0f - COMPLEMENTARY_ALPHA) * gyroRoll;
}

void MPU6050Handler::detectMotion() {
    // Calculate acceleration magnitude
    float accelMag = sqrt(_data.accelX * _data.accelX +
                         _data.accelY * _data.accelY +
                         _data.accelZ * _data.accelZ);

    // Check for acceleration magnitude change (should be ~1g when stationary)
    float accelChange = fabs(accelMag - _lastAccelMagnitude);
    _lastAccelMagnitude = accelMag;

    // Check gyroscope magnitude
    float gyroMag = sqrt(_data.gyroX * _data.gyroX +
                        _data.gyroY * _data.gyroY +
                        _data.gyroZ * _data.gyroZ);

    // Motion detected if acceleration changed significantly or significant rotation
    _isMoving = (accelChange > MOTION_ACCEL_THRESHOLD) ||
                (gyroMag > MOTION_GYRO_THRESHOLD);
}

void MPU6050Handler::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU6050_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void MPU6050Handler::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
    Wire.beginTransmission(MPU6050_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom((uint8_t)MPU6050_ADDRESS, length);
    for (uint8_t i = 0; i < length && Wire.available(); i++) {
        buffer[i] = Wire.read();
    }
}
