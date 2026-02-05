#ifndef LEVELING_CONTROLLER_H
#define LEVELING_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

/**
 * LevelingController - PI control algorithm for platform leveling
 *
 * Uses two independent PI controllers for pitch and roll correction.
 * Maps angular errors to motor steps for the two back legs.
 *
 * Motor mapping (viewed from above):
 *           FRONT (Fixed Leg)
 *                *
 *               / \
 *              /   \
 *             /     \
 *            *-------*
 *       LEFT(M1)  RIGHT(M2)
 *
 * - Pitch correction: Both motors move same direction
 * - Roll correction: Motors move opposite directions
 */
class LevelingController {
public:
    LevelingController();

    /**
     * Initialize controller with default gains
     */
    void begin();

    /**
     * Calculate motor corrections based on current pitch and roll
     * @param pitch Current pitch angle in degrees
     * @param roll Current roll angle in degrees
     * @return Motor steps for each motor
     */
    MotorCorrection calculate(float pitch, float roll);

    /**
     * Set pitch PI gains
     * @param kp Proportional gain
     * @param ki Integral gain
     */
    void setPitchGains(float kp, float ki);

    /**
     * Set roll PI gains
     * @param kp Proportional gain
     * @param ki Integral gain
     */
    void setRollGains(float kp, float ki);

    /**
     * Get current pitch gains
     */
    void getPitchGains(float& kp, float& ki) const;

    /**
     * Get current roll gains
     */
    void getRollGains(float& kp, float& ki) const;

    /**
     * Reset integral accumulators
     * Call when starting a new leveling cycle or after disturbance
     */
    void reset();

    /**
     * Set the steps-per-degree conversion factor
     * @param factor Steps needed to correct one degree of tilt
     */
    void setStepsPerDegree(float factor);

private:
    PIController _pitchController;
    PIController _rollController;

    float _stepsPerDegree;  // Conversion factor from degrees to steps

    /**
     * Calculate PI output for one axis
     * @param controller PI controller state
     * @param error Current error (target - actual)
     * @return Control output
     */
    float calculatePI(PIController& controller, float error);
};

#endif // LEVELING_CONTROLLER_H
