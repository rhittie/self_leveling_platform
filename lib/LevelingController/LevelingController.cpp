#include "LevelingController.h"

LevelingController::LevelingController()
    : _stepsPerDegree(60.0f)  // Steps per degree of PI output (~1/deg-per-step for roll axis)
{
    _pitchController.kp = DEFAULT_KP_PITCH;
    _pitchController.ki = DEFAULT_KI_PITCH;
    _pitchController.integral = 0;
    _pitchController.lastError = 0;

    _rollController.kp = DEFAULT_KP_ROLL;
    _rollController.ki = DEFAULT_KI_ROLL;
    _rollController.integral = 0;
    _rollController.lastError = 0;
}

void LevelingController::begin() {
    reset();
    Serial.println("LevelingController: Initialized");
    Serial.printf("  Pitch gains: Kp=%.2f, Ki=%.2f\n", _pitchController.kp, _pitchController.ki);
    Serial.printf("  Roll gains: Kp=%.2f, Ki=%.2f\n", _rollController.kp, _rollController.ki);
}

MotorCorrection LevelingController::calculate(float pitch, float roll) {
    MotorCorrection correction;

    // Error sign: positive error means positive angle needs correction.
    // Our motor mapping has NEGATIVE plant gain (positive steps decrease angles),
    // so we use error = +actual to get net negative feedback.
    //
    // Measured: M1 +steps → pitch -0.22, roll +0.42
    //           M2 +steps → pitch -0.20, roll -0.45
    // Motor mapping below: M1 = pitch - roll, M2 = pitch + roll
    // For roll>0: M1 goes negative (dRoll negative ✓), M2 goes positive (dRoll negative ✓)
    // For pitch>0: both go positive (dPitch negative ✓)
    float pitchError = pitch;
    float rollError = roll;

    // Calculate PI outputs
    float pitchOutput = calculatePI(_pitchController, pitchError);
    float rollOutput = calculatePI(_rollController, rollError);

    // Convert to steps
    pitchOutput *= _stepsPerDegree;
    rollOutput *= _stepsPerDegree;

    // Map to motors:
    // Motor 1 (left back): responds to pitch and negative roll
    // Motor 2 (right back): responds to pitch and positive roll
    //
    // Pitch positive = platform tilted back = both motors need to raise (positive steps)
    // Roll positive = platform tilted right = M1 raises, M2 lowers

    correction.motor1Steps = (int)(pitchOutput - rollOutput);
    correction.motor2Steps = -(int)(pitchOutput + rollOutput);  // Negated: M2 lead screw is physically reversed

    return correction;
}

void LevelingController::setPitchGains(float kp, float ki) {
    _pitchController.kp = kp;
    _pitchController.ki = ki;
    Serial.printf("LevelingController: Pitch gains set to Kp=%.2f, Ki=%.2f\n", kp, ki);
}

void LevelingController::setRollGains(float kp, float ki) {
    _rollController.kp = kp;
    _rollController.ki = ki;
    Serial.printf("LevelingController: Roll gains set to Kp=%.2f, Ki=%.2f\n", kp, ki);
}

void LevelingController::getPitchGains(float& kp, float& ki) const {
    kp = _pitchController.kp;
    ki = _pitchController.ki;
}

void LevelingController::getRollGains(float& kp, float& ki) const {
    kp = _rollController.kp;
    ki = _rollController.ki;
}

void LevelingController::reset() {
    _pitchController.integral = 0;
    _pitchController.lastError = 0;
    _rollController.integral = 0;
    _rollController.lastError = 0;
}

void LevelingController::setStepsPerDegree(float factor) {
    _stepsPerDegree = factor;
}

float LevelingController::calculatePI(PIController& controller, float error) {
    // Proportional term
    float pTerm = controller.kp * error;

    // Integral term with anti-windup
    controller.integral += error;
    controller.integral = constrain(controller.integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);
    float iTerm = controller.ki * controller.integral;

    // Store error for potential derivative term (not used in PI)
    controller.lastError = error;

    // Calculate output
    float output = pTerm + iTerm;

    return output;
}
