#include "StepperController.h"

StepperController::StepperController()
    : _stepIndex1(0)
    , _stepIndex2(0)
    , _position1(0)
    , _position2(0)
    , _minPosition(MOTOR_MIN_POSITION)
    , _maxPosition(MOTOR_MAX_POSITION)
    , _stepDelayUs(STEP_DELAY_US)
{
}

void StepperController::begin() {
    // Initialize motor 1 pins
    for (int i = 0; i < 4; i++) {
        pinMode(_motor1Pins[i], OUTPUT);
        digitalWrite(_motor1Pins[i], LOW);
    }

    // Initialize motor 2 pins
    for (int i = 0; i < 4; i++) {
        pinMode(_motor2Pins[i], OUTPUT);
        digitalWrite(_motor2Pins[i], LOW);
    }

    Serial.println("StepperController: Initialized");
}

void StepperController::moveMotor1(int steps) {
    if (steps == 0) return;

    int direction = (steps > 0) ? 1 : -1;
    int absSteps = abs(steps);

    for (int i = 0; i < absSteps; i++) {
        stepMotor1(direction);
        delayMicroseconds(_stepDelayUs);
    }
}

void StepperController::moveMotor2(int steps) {
    if (steps == 0) return;

    int direction = (steps > 0) ? 1 : -1;
    int absSteps = abs(steps);

    for (int i = 0; i < absSteps; i++) {
        stepMotor2(direction);
        delayMicroseconds(_stepDelayUs);
    }
}

void StepperController::moveBoth(int steps1, int steps2) {
    // Move both motors in an interleaved fashion for smoother operation
    int dir1 = (steps1 > 0) ? 1 : -1;
    int dir2 = (steps2 > 0) ? 1 : -1;
    int abs1 = abs(steps1);
    int abs2 = abs(steps2);

    int maxSteps = max(abs1, abs2);
    if (maxSteps == 0) return;

    // Bresenham-like algorithm to interleave steps
    int err1 = maxSteps / 2;
    int err2 = maxSteps / 2;

    for (int i = 0; i < maxSteps; i++) {
        err1 -= abs1;
        if (err1 < 0) {
            err1 += maxSteps;
            stepMotor1(dir1);
        }

        err2 -= abs2;
        if (err2 < 0) {
            err2 += maxSteps;
            stepMotor2(dir2);
        }

        delayMicroseconds(_stepDelayUs);
    }
}

void StepperController::applyCorrection(const MotorCorrection& correction) {
    // Clamp steps to maximum per cycle
    int steps1 = constrain(correction.motor1Steps, -MAX_CORRECTION_STEPS, MAX_CORRECTION_STEPS);
    int steps2 = constrain(correction.motor2Steps, -MAX_CORRECTION_STEPS, MAX_CORRECTION_STEPS);

    moveBoth(steps1, steps2);
}

void StepperController::release() {
    // Turn off all coils to save power
    for (int i = 0; i < 4; i++) {
        digitalWrite(_motor1Pins[i], LOW);
        digitalWrite(_motor2Pins[i], LOW);
    }
}

void StepperController::setSpeed(float rpm) {
    // Calculate step delay based on RPM
    // 28BYJ-48 has 2048 steps per revolution (half-step mode)
    // steps_per_second = (rpm * 2048) / 60
    // delay_us = 1000000 / steps_per_second

    if (rpm <= 0) rpm = 1;
    if (rpm > 15) rpm = 15;  // Max safe speed for 28BYJ-48

    float stepsPerSecond = (rpm * STEPS_PER_REVOLUTION) / 60.0f;
    _stepDelayUs = (unsigned long)(1000000.0f / stepsPerSecond);

    // Minimum delay to prevent missed steps
    if (_stepDelayUs < 1000) _stepDelayUs = 1000;
}

void StepperController::resetPositions() {
    _position1 = 0;
    _position2 = 0;
}

bool StepperController::isAtLimit1() const {
    return _position1 <= _minPosition || _position1 >= _maxPosition;
}

bool StepperController::isAtLimit2() const {
    return _position2 <= _minPosition || _position2 >= _maxPosition;
}

void StepperController::stepMotor1(int direction) {
    // Enforce position limits
    if (direction > 0 && _position1 >= _maxPosition) return;
    if (direction < 0 && _position1 <= _minPosition) return;

    // Update step index
    if (direction > 0) {
        _stepIndex1 = (_stepIndex1 + 1) % 8;
    } else {
        _stepIndex1 = (_stepIndex1 + 7) % 8;  // +7 is same as -1 for mod 8
    }

    // Set coil pattern
    setCoils(_motor1Pins, _halfStepSequence[_stepIndex1]);

    // Update position counter
    _position1 += direction;
}

void StepperController::stepMotor2(int direction) {
    // Enforce position limits
    if (direction > 0 && _position2 >= _maxPosition) return;
    if (direction < 0 && _position2 <= _minPosition) return;

    // Update step index
    if (direction > 0) {
        _stepIndex2 = (_stepIndex2 + 1) % 8;
    } else {
        _stepIndex2 = (_stepIndex2 + 7) % 8;
    }

    // Set coil pattern
    setCoils(_motor2Pins, _halfStepSequence[_stepIndex2]);

    // Update position counter
    _position2 += direction;
}

void StepperController::setCoils(const uint8_t* pins, uint8_t pattern) {
    // Set each coil based on the bit pattern
    digitalWrite(pins[0], (pattern & 0b0001) ? HIGH : LOW);
    digitalWrite(pins[1], (pattern & 0b0010) ? HIGH : LOW);
    digitalWrite(pins[2], (pattern & 0b0100) ? HIGH : LOW);
    digitalWrite(pins[3], (pattern & 0b1000) ? HIGH : LOW);
}
