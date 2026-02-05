#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// Pin Definitions
// ============================================================================

// I2C (MPU6050)
#define PIN_SDA 21
#define PIN_SCL 22
#define MPU6050_ADDRESS 0x68

// Stepper Motor 1 (Left Back Leg)
#define MOTOR1_IN1 19
#define MOTOR1_IN2 18
#define MOTOR1_IN3 5
#define MOTOR1_IN4 17

// Stepper Motor 2 (Right Back Leg)
#define MOTOR2_IN1 16
#define MOTOR2_IN2 4
#define MOTOR2_IN3 13
#define MOTOR2_IN4 15

// User Interface
#define PIN_BUTTON 33
#define PIN_LED 2  // Onboard LED

// ============================================================================
// Timing Constants
// ============================================================================

#define STABILITY_TIMEOUT_MS 3000    // Wait time before leveling starts
#define LEVEL_CHECK_INTERVAL_MS 50   // How often to check level (20 Hz)
#define BUTTON_DEBOUNCE_MS 50        // Button debounce time
#define BUTTON_LONG_PRESS_MS 2000    // Long press threshold
#define IMU_UPDATE_INTERVAL_MS 10    // IMU update rate (100 Hz)

// ============================================================================
// LED Blink Patterns (half-period in ms)
// ============================================================================

#define LED_SLOW_BLINK_MS 500        // 1 Hz (INITIALIZING)
#define LED_FAST_BLINK_MS 125        // 4 Hz (LEVELING)
#define LED_ERROR_BLINK_MS 50        // 10 Hz (ERROR)
#define LED_DOUBLE_PULSE_PERIOD_MS 2000  // Double pulse every 2s (LEVEL_OK)

// ============================================================================
// Leveling Parameters
// ============================================================================

#define LEVEL_TOLERANCE_DEG 0.5f     // Acceptable angle deviation
#define MAX_CORRECTION_STEPS 50      // Max steps per correction cycle

// PI Controller Defaults
#define DEFAULT_KP_PITCH 2.0f
#define DEFAULT_KI_PITCH 0.1f
#define DEFAULT_KP_ROLL 2.0f
#define DEFAULT_KI_ROLL 0.1f

// Integral windup limits
#define INTEGRAL_LIMIT 100.0f

// ============================================================================
// Motor Parameters
// ============================================================================

// 28BYJ-48 specifications
#define STEPS_PER_REVOLUTION 2048    // Full steps per revolution (with gearbox)
#define MOTOR_SPEED_RPM 10           // Default speed
#define STEP_DELAY_US 2000           // Microseconds between steps at default speed

// Direction definitions (1 = raise leg, -1 = lower leg)
#define MOTOR_DIR_RAISE 1
#define MOTOR_DIR_LOWER -1

// ============================================================================
// IMU Parameters
// ============================================================================

// Complementary filter coefficient (0-1, higher = trust accelerometer more)
#define COMPLEMENTARY_ALPHA 0.02f

// Motion detection thresholds
#define MOTION_ACCEL_THRESHOLD 0.15f  // g units
#define MOTION_GYRO_THRESHOLD 10.0f   // degrees/second

// Calibration samples
#define CALIBRATION_SAMPLES 200

// ============================================================================
// Serial Debug
// ============================================================================

#define SERIAL_BAUD_RATE 115200

#endif // CONFIG_H
