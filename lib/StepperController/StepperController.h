#ifndef STEPPER_CONTROLLER_H
#define STEPPER_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

/**
 * StepperController - Controls two 28BYJ-48 stepper motors via ULN2003 drivers
 *
 * Uses half-step sequence for smoother operation.
 * Motors can run simultaneously or independently.
 */
class StepperController {
public:
    StepperController();

    /**
     * Initialize motor GPIO pins
     */
    void begin();

    /**
     * Move motor 1 (left back leg) by specified steps
     * @param steps Positive = raise leg, negative = lower leg
     */
    void moveMotor1(int steps);

    /**
     * Move motor 2 (right back leg) by specified steps
     * @param steps Positive = raise leg, negative = lower leg
     */
    void moveMotor2(int steps);

    /**
     * Move both motors simultaneously
     * @param steps1 Steps for motor 1
     * @param steps2 Steps for motor 2
     */
    void moveBoth(int steps1, int steps2);

    /**
     * Apply motor correction from leveling algorithm
     * @param correction Motor steps calculated by PI controller
     */
    void applyCorrection(const MotorCorrection& correction);

    /**
     * De-energize both motors to save power
     * Call when platform is level and stable
     */
    void release();

    /**
     * Set motor speed
     * @param rpm Revolutions per minute (1-15 recommended for 28BYJ-48)
     */
    void setSpeed(float rpm);

    /**
     * Get current step position for motor 1
     */
    long getPosition1() const { return _position1; }

    /**
     * Get current step position for motor 2
     */
    long getPosition2() const { return _position2; }

    /**
     * Reset position counters to zero
     */
    void resetPositions();

    /**
     * Check if motor 1 is at a position limit
     */
    bool isAtLimit1() const;

    /**
     * Check if motor 2 is at a position limit
     */
    bool isAtLimit2() const;

    /**
     * Get position limits
     */
    long getMinPosition() const { return _minPosition; }
    long getMaxPosition() const { return _maxPosition; }

    /**
     * Set position limits (for finding physical extents in test mode)
     */
    void setLimits(long minPos, long maxPos) { _minPosition = minPos; _maxPosition = maxPos; }

private:
    // Motor pin arrays
    const uint8_t _motor1Pins[4] = {MOTOR1_IN1, MOTOR1_IN2, MOTOR1_IN3, MOTOR1_IN4};
    const uint8_t _motor2Pins[4] = {MOTOR2_IN1, MOTOR2_IN2, MOTOR2_IN3, MOTOR2_IN4};

    // Half-step sequence for smoother operation (8 steps per sequence)
    const uint8_t _halfStepSequence[8] = {
        0b0001,  // Step 0
        0b0011,  // Step 1
        0b0010,  // Step 2
        0b0110,  // Step 3
        0b0100,  // Step 4
        0b1100,  // Step 5
        0b1000,  // Step 6
        0b1001   // Step 7
    };

    uint8_t _stepIndex1;  // Current step in sequence for motor 1
    uint8_t _stepIndex2;  // Current step in sequence for motor 2
    long _position1;      // Cumulative position for motor 1
    long _position2;      // Cumulative position for motor 2
    long _minPosition;    // Minimum allowed position
    long _maxPosition;    // Maximum allowed position
    unsigned long _stepDelayUs;  // Delay between steps in microseconds

    /**
     * Execute one step on motor 1
     * @param direction 1 = forward, -1 = reverse
     */
    void stepMotor1(int direction);

    /**
     * Execute one step on motor 2
     * @param direction 1 = forward, -1 = reverse
     */
    void stepMotor2(int direction);

    /**
     * Set coil states for a motor
     * @param pins Array of 4 GPIO pins
     * @param pattern 4-bit pattern for coil activation
     */
    void setCoils(const uint8_t* pins, uint8_t pattern);
};

#endif // STEPPER_CONTROLLER_H
