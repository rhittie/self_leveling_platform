/**
 * Self-Leveling Platform Firmware
 *
 * ESP32-based firmware for a three-legged self-leveling platform using
 * MPU6050 accelerometer and two 28BYJ-48 stepper motors.
 *
 * State Machine:
 * IDLE → [Button Press] → INITIALIZING → WAIT_FOR_STABLE → [No motion 3s] → LEVELING
 *                                                                   ↑           |
 *                                                                   └──[Motion]─┘
 *                                                                   ↓
 *                                                              LEVEL_OK
 */

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include "config.h"
#include "types.h"
#include "MPU6050Handler.h"
#include "StepperController.h"
#include "LevelingController.h"
#include "ButtonHandler.h"
#include "StatusLED.h"

// ============================================================================
// Global Objects
// ============================================================================

MPU6050Handler imu;
StepperController motors;
LevelingController leveling;
ButtonHandler button(PIN_BUTTON, true);  // Active low with pull-up
StatusLED statusLED(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE);  // RGB LED in push button
Preferences prefs;

// ============================================================================
// State Machine
// ============================================================================

SystemState currentState = SystemState::IDLE;
SystemConfig config;

unsigned long stateEnteredTime = 0;
unsigned long lastLevelCheckTime = 0;
unsigned long lastIMUUpdateTime = 0;
unsigned long lastStableTime = 0;
unsigned long levelSinceTime = 0;        // When platform first entered tolerance
bool withinTolerance = false;            // Currently within level tolerance

// ============================================================================
// Test Mode Variables
// ============================================================================

bool testModeIMUStreaming = false;       // Continuous IMU data streaming
bool testModeButtonTest = false;         // Button event printing mode
bool testModeMotor1Continuous = false;   // Motor 1 continuous rotation
bool testModeMotor2Continuous = false;   // Motor 2 continuous rotation
bool testModeLEDCycle = false;           // LED pattern cycling
int testModeMotorSpeed = MOTOR_SPEED_RPM; // Test mode motor speed
unsigned long testModeLastStreamTime = 0;
unsigned long testModeLastLEDCycleTime = 0;
int testModeLEDCycleIndex = 0;

// ============================================================================
// Function Prototypes
// ============================================================================

void changeState(SystemState newState);
void handleIdleState();
void handleInitializingState();
void handleWaitForStableState();
void handleLevelingState();
void handleLevelOkState();
void handleErrorState();
void handleTestModeState();
void handleSerialCommands();
void handleTestModeCommands(String& input);
void printHelp();
void printStatus();
void printIMUData();
void printTestModeMenu();
void scanI2CBus();
void printPinInfo();
void saveMotorPositions();
void loadMotorPositions();

// ============================================================================
// Motor Position Persistence
// ============================================================================

void saveMotorPositions() {
    prefs.begin("motors", false);  // read-write
    prefs.putLong("m1pos", motors.getPosition1());
    prefs.putLong("m2pos", motors.getPosition2());
    prefs.end();
    Serial.printf("[SAVE] Motor positions saved: M1=%ld M2=%ld\n",
                  motors.getPosition1(), motors.getPosition2());
}

void loadMotorPositions() {
    prefs.begin("motors", true);  // read-only
    long m1 = prefs.getLong("m1pos", 0);
    long m2 = prefs.getLong("m2pos", 0);
    prefs.end();
    motors.setPosition1(m1);
    motors.setPosition2(m2);
    Serial.printf("[LOAD] Motor positions restored: M1=%ld M2=%ld\n", m1, m2);
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);  // Wait for serial monitor

    Serial.println();
    Serial.println("===========================================");
    Serial.println("  Self-Leveling Platform Firmware v1.0");
    Serial.println("===========================================");
    Serial.println();

    // Initialize default configuration
    config.kpPitch = DEFAULT_KP_PITCH;
    config.kiPitch = DEFAULT_KI_PITCH;
    config.kpRoll = DEFAULT_KP_ROLL;
    config.kiRoll = DEFAULT_KI_ROLL;
    config.levelTolerance = LEVEL_TOLERANCE_DEG;
    config.continuousLogging = false;

    // Initialize components
    button.begin();
    statusLED.begin();
    motors.begin();
    loadMotorPositions();

    // Start in IDLE state
    changeState(SystemState::IDLE);

    Serial.println("System ready. Press button to start leveling.");
    Serial.println("Type 'h' for serial command help.");
    Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    unsigned long currentTime = millis();

    // Always update button and LED
    ButtonEvent buttonEvent = button.update();
    statusLED.update();

    // Handle serial commands
    handleSerialCommands();

    // Handle button events globally
    if (buttonEvent == ButtonEvent::LONG_PRESS) {
        // Long press triggers safe shutdown — save positions and halt
        Serial.println("Long press detected - safe shutdown");
        saveMotorPositions();
        changeState(SystemState::SAFE_SHUTDOWN);
        return;
    }

    // State-specific handling
    switch (currentState) {
        case SystemState::IDLE:
            handleIdleState();
            if (buttonEvent == ButtonEvent::SHORT_PRESS) {
                changeState(SystemState::INITIALIZING);
            }
            break;

        case SystemState::INITIALIZING:
            handleInitializingState();
            break;

        case SystemState::WAIT_FOR_STABLE:
            handleWaitForStableState();
            break;

        case SystemState::LEVELING:
            handleLevelingState();
            break;

        case SystemState::LEVEL_OK:
            handleLevelOkState();
            break;

        case SystemState::ERROR:
            handleErrorState();
            if (buttonEvent == ButtonEvent::SHORT_PRESS) {
                changeState(SystemState::INITIALIZING);
            }
            break;

        case SystemState::TEST_MODE:
            handleTestModeState();
            break;

        case SystemState::SAFE_SHUTDOWN:
            // Halted — short press wakes back to IDLE
            if (buttonEvent == ButtonEvent::SHORT_PRESS) {
                Serial.println("Short press detected - waking from shutdown");
                changeState(SystemState::IDLE);
            }
            break;
    }

    // Continuous logging if enabled
    if (config.continuousLogging && currentState != SystemState::IDLE) {
        static unsigned long lastLogTime = 0;
        if (currentTime - lastLogTime >= 100) {  // 10 Hz logging
            lastLogTime = currentTime;
            Serial.printf("P:%.2f R:%.2f M:%d M1:%ld M2:%ld\n",
                          imu.getPitch(), imu.getRoll(), imu.isMoving() ? 1 : 0,
                          motors.getPosition1(), motors.getPosition2());
        }
    }
}

// ============================================================================
// State Change Handler
// ============================================================================

void changeState(SystemState newState) {
    if (newState == currentState) return;

    Serial.printf("State: %s -> %s\n", stateToString(currentState), stateToString(newState));

    currentState = newState;
    stateEnteredTime = millis();

    // Set LED color + pattern for new state
    switch (newState) {
        case SystemState::IDLE:
            statusLED.setColor(LEDColors::OFF);
            statusLED.setPattern(LEDPattern::OFF);
            motors.release();
            saveMotorPositions();
            break;

        case SystemState::INITIALIZING:
            statusLED.setColor(LEDColors::BLUE);
            statusLED.setPattern(LEDPattern::SLOW_BLINK);
            break;

        case SystemState::WAIT_FOR_STABLE:
            statusLED.setColor(LEDColors::YELLOW);
            statusLED.setPattern(LEDPattern::SOLID);
            lastStableTime = millis();
            break;

        case SystemState::LEVELING:
            statusLED.setColor(LEDColors::CYAN);
            statusLED.setPattern(LEDPattern::FAST_BLINK);
            leveling.reset();  // Reset PI integrators
            withinTolerance = false;  // Reset level confirmation
            break;

        case SystemState::LEVEL_OK:
            statusLED.setColor(LEDColors::GREEN);
            statusLED.setPattern(LEDPattern::DOUBLE_PULSE);
            motors.release();  // Save power when level
            saveMotorPositions();
            break;

        case SystemState::ERROR:
            statusLED.setColor(LEDColors::RED);
            statusLED.setPattern(LEDPattern::ERROR_BLINK);
            motors.release();
            break;

        case SystemState::TEST_MODE:
            // Reset test mode flags
            testModeIMUStreaming = false;
            testModeButtonTest = false;
            testModeMotor1Continuous = false;
            testModeMotor2Continuous = false;
            testModeLEDCycle = false;
            testModeMotorSpeed = MOTOR_SPEED_RPM;
            statusLED.setColor(LEDColors::PURPLE);
            statusLED.setPattern(LEDPattern::SOLID);
            printTestModeMenu();
            break;

        case SystemState::SAFE_SHUTDOWN:
            statusLED.setColor(LEDColors::GREEN);
            statusLED.setPattern(LEDPattern::SOLID);
            motors.release();
            Serial.println("=== SAFE TO POWER OFF ===");
            Serial.println("Motor positions saved. Short press to wake.");
            break;
    }
}

// ============================================================================
// State Handlers
// ============================================================================

void handleIdleState() {
    // Nothing to do - waiting for button press
}

void handleInitializingState() {
    Serial.println("Initializing IMU...");

    if (!imu.begin()) {
        Serial.println("ERROR: Failed to initialize IMU!");
        changeState(SystemState::ERROR);
        return;
    }

    // Initialize leveling controller
    leveling.begin();
    leveling.setPitchGains(config.kpPitch, config.kiPitch);
    leveling.setRollGains(config.kpRoll, config.kiRoll);

    Serial.println("IMU initialized successfully.");
    Serial.println("Waiting for platform to stabilize...");

    changeState(SystemState::WAIT_FOR_STABLE);
}

void handleWaitForStableState() {
    unsigned long currentTime = millis();

    // Update IMU at regular intervals
    if (currentTime - lastIMUUpdateTime >= IMU_UPDATE_INTERVAL_MS) {
        lastIMUUpdateTime = currentTime;
        imu.update();

        // Check for motion
        if (imu.isMoving()) {
            lastStableTime = currentTime;  // Reset stability timer
        }
    }

    // Check if stable long enough
    if (currentTime - lastStableTime >= STABILITY_TIMEOUT_MS) {
        Serial.println("Platform stable. Starting leveling...");
        changeState(SystemState::LEVELING);
    }
}

void handleLevelingState() {
    unsigned long currentTime = millis();

    // Update IMU at regular intervals
    if (currentTime - lastIMUUpdateTime >= IMU_UPDATE_INTERVAL_MS) {
        lastIMUUpdateTime = currentTime;
        imu.update();

        // Check for motion - if moving, wait for stability
        if (imu.isMoving()) {
            Serial.println("Motion detected - waiting for stability...");
            changeState(SystemState::WAIT_FOR_STABLE);
            return;
        }
    }

    // Perform leveling correction at regular intervals
    if (currentTime - lastLevelCheckTime >= LEVEL_CHECK_INTERVAL_MS) {
        lastLevelCheckTime = currentTime;

        float pitch = imu.getPitch();
        float roll = imu.getRoll();

        // Check if within tolerance
        if (imu.isLevel(config.levelTolerance)) {
            if (!withinTolerance) {
                // Just entered tolerance - start timing
                withinTolerance = true;
                levelSinceTime = currentTime;
            } else if (currentTime - levelSinceTime >= LEVEL_CONFIRM_MS) {
                // Sustained level for required duration - confirm level
                Serial.printf("Level achieved! Pitch=%.2f, Roll=%.2f (stable for %dms)\n",
                              pitch, roll, LEVEL_CONFIRM_MS);
                changeState(SystemState::LEVEL_OK);
                return;
            }
            // Within tolerance but still confirming - don't apply correction
        } else {
            // Outside tolerance - reset confirmation timer
            withinTolerance = false;

            // Calculate and apply correction
            MotorCorrection correction = leveling.calculate(pitch, roll);

            // Only move motors if correction is significant
            if (abs(correction.motor1Steps) > 0 || abs(correction.motor2Steps) > 0) {
                motors.applyCorrection(correction);
            }
        }
    }
}

void handleLevelOkState() {
    unsigned long currentTime = millis();

    // Continue monitoring IMU
    if (currentTime - lastIMUUpdateTime >= IMU_UPDATE_INTERVAL_MS) {
        lastIMUUpdateTime = currentTime;
        imu.update();

        // Check if motion detected
        if (imu.isMoving()) {
            Serial.println("Motion detected - re-leveling...");
            changeState(SystemState::WAIT_FOR_STABLE);
            return;
        }

        // Check if still level (use 1.5x tolerance for hysteresis to avoid oscillation)
        if (!imu.isLevel(config.levelTolerance * 1.5f)) {
            Serial.println("Platform no longer level - adjusting...");
            changeState(SystemState::LEVELING);
        }
    }
}

void handleErrorState() {
    // Wait for button press to retry
}

// ============================================================================
// Serial Command Handler
// ============================================================================

void handleSerialCommands() {
    if (!Serial.available()) return;

    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() == 0) return;

    // Check for test mode entry commands
    if (input.equalsIgnoreCase("admin") || input.equalsIgnoreCase("test")) {
        changeState(SystemState::TEST_MODE);
        return;
    }

    // If in test mode, use the test mode command handler
    if (currentState == SystemState::TEST_MODE) {
        handleTestModeCommands(input);
        return;
    }

    char cmd = input.charAt(0);

    switch (cmd) {
        case 'h':
        case 'H':
        case '?':
            printHelp();
            break;

        case 's':
        case 'S':
            printStatus();
            break;

        case 'i':
        case 'I':
            printIMUData();
            break;

        case 'c':
        case 'C':
            if (currentState != SystemState::IDLE) {
                Serial.println("Calibration only available in IDLE state.");
            } else {
                Serial.println("Starting IMU...");
                if (imu.begin()) {
                    imu.calibrate();
                }
            }
            break;

        case 'r':
        case 'R':
            Serial.println("Resetting to IDLE state...");
            changeState(SystemState::IDLE);
            break;

        case 'l':
        case 'L':
            // "level" command: start leveling from IDLE (like button press)
            if (input.equalsIgnoreCase("level")) {
                if (currentState == SystemState::IDLE) {
                    Serial.println("Starting leveling via serial command...");
                    changeState(SystemState::INITIALIZING);
                } else {
                    Serial.printf("Cannot start leveling from %s state. Reset first.\n",
                                  stateToString(currentState));
                }
            } else {
                config.continuousLogging = !config.continuousLogging;
                Serial.printf("Continuous logging: %s\n", config.continuousLogging ? "ON" : "OFF");
            }
            break;

        case 'm':
        case 'M': {
            // Motor command: m1 <steps> or m2 <steps>
            if (input.length() < 3) {
                Serial.println("Usage: m1 <steps> or m2 <steps>");
                break;
            }

            char motorNum = input.charAt(1);
            int steps = input.substring(2).toInt();

            if (motorNum == '1') {
                Serial.printf("Moving motor 1 by %d steps...\n", steps);
                motors.moveMotor1(steps);
                Serial.println("Done.");
            } else if (motorNum == '2') {
                Serial.printf("Moving motor 2 by %d steps...\n", steps);
                motors.moveMotor2(steps);
                Serial.println("Done.");
            } else {
                Serial.println("Invalid motor number. Use m1 or m2.");
            }
            break;
        }

        case 'p':
        case 'P': {
            // Set PI gains: p <kp> <ki>
            int spaceIdx = input.indexOf(' ', 2);
            if (spaceIdx == -1) {
                float kp, ki;
                leveling.getPitchGains(kp, ki);
                Serial.printf("Current gains - Kp: %.2f, Ki: %.2f\n", kp, ki);
                Serial.println("Usage: p <kp> <ki>");
            } else {
                float kp = input.substring(2, spaceIdx).toFloat();
                float ki = input.substring(spaceIdx + 1).toFloat();
                config.kpPitch = kp;
                config.kiPitch = ki;
                config.kpRoll = kp;
                config.kiRoll = ki;
                leveling.setPitchGains(kp, ki);
                leveling.setRollGains(kp, ki);
            }
            break;
        }

        case 't':
        case 'T': {
            // Set tolerance: t <degrees>
            if (input.length() < 3) {
                Serial.printf("Current level tolerance: %.2f degrees\n", config.levelTolerance);
                Serial.println("Usage: t <degrees>");
            } else {
                float tol = input.substring(2).toFloat();
                if (tol > 0 && tol < 10) {
                    config.levelTolerance = tol;
                    Serial.printf("Level tolerance set to %.2f degrees\n", tol);
                } else {
                    Serial.println("Invalid tolerance (must be between 0 and 10)");
                }
            }
            break;
        }

        default:
            Serial.printf("Unknown command: '%c'. Type 'h' for help.\n", cmd);
            break;
    }
}

void printHelp() {
    Serial.println();
    Serial.println("=== Serial Debug Commands ===");
    Serial.println("  h         - Show this help");
    Serial.println("  s         - Print current state");
    Serial.println("  i         - Print IMU data");
    Serial.println("  m1 <N>    - Move motor 1 by N steps");
    Serial.println("  m2 <N>    - Move motor 2 by N steps");
    Serial.println("  c         - Run IMU calibration (IDLE only)");
    Serial.println("  r         - Reset to IDLE state");
    Serial.println("  p <kp> <ki> - Set PI gains");
    Serial.println("  t <deg>   - Set level tolerance");
    Serial.println("  l         - Toggle continuous logging");
    Serial.println("  level     - Start leveling (same as button press)");
    Serial.println();
    Serial.println("  admin     - Enter ADMIN TEST MODE");
    Serial.println("  test      - Enter ADMIN TEST MODE");
    Serial.println();
}

void printStatus() {
    Serial.println();
    Serial.println("=== System Status ===");
    Serial.printf("  State: %s\n", stateToString(currentState));
    Serial.printf("  Time in state: %lu ms\n", millis() - stateEnteredTime);
    Serial.printf("  Level tolerance: %.2f deg\n", config.levelTolerance);
    Serial.printf("  PI gains: Kp=%.2f, Ki=%.2f\n", config.kpPitch, config.kiPitch);
    Serial.printf("  Motor positions: M1=%ld, M2=%ld\n", motors.getPosition1(), motors.getPosition2());
    Serial.printf("  Continuous logging: %s\n", config.continuousLogging ? "ON" : "OFF");
    Serial.println();
}

void printIMUData() {
    if (currentState == SystemState::IDLE) {
        Serial.println("IMU not active in IDLE state. Start leveling first.");
        return;
    }

    const IMUData& data = imu.getData();
    const IMURawData& raw = imu.getRawData();

    Serial.println();
    Serial.println("=== IMU Data ===");
    Serial.printf("  Pitch: %.2f deg\n", data.pitch);
    Serial.printf("  Roll:  %.2f deg\n", data.roll);
    Serial.printf("  Accel: X=%.3fg, Y=%.3fg, Z=%.3fg\n", data.accelX, data.accelY, data.accelZ);
    Serial.printf("  Gyro:  X=%.2f, Y=%.2f, Z=%.2f deg/s\n", data.gyroX, data.gyroY, data.gyroZ);
    Serial.printf("  Temp:  %.1f C\n", data.temperature);
    Serial.printf("  Moving: %s\n", imu.isMoving() ? "YES" : "NO");
    Serial.printf("  Level:  %s\n", imu.isLevel(config.levelTolerance) ? "YES" : "NO");
    Serial.println();
    Serial.println("  Raw values:");
    Serial.printf("    Accel: X=%d, Y=%d, Z=%d\n", raw.accelX, raw.accelY, raw.accelZ);
    Serial.printf("    Gyro:  X=%d, Y=%d, Z=%d\n", raw.gyroX, raw.gyroY, raw.gyroZ);
    Serial.println();
}

// ============================================================================
// Test Mode Functions
// ============================================================================

void printTestModeMenu() {
    Serial.println();
    Serial.println("===========================================");
    Serial.println("       ADMIN TEST MODE");
    Serial.println("===========================================");
    Serial.println("Commands:");
    Serial.println("  Motors:  m1/m2 <steps>, m1c, m2c, mstop, mspeed <rpm>");
    Serial.println("           mpos (query positions), mreset (reset to zero)");
    Serial.println("  IMU:     scan, imu, read, stream, cal, raw");
    Serial.println("  Button:  btn (then press button to see events)");
    Serial.println("  LED:     led on/off/slow/fast/pulse/error/cycle");
    Serial.println("           led red/green/blue/yellow/cyan/purple/white");
    Serial.println("  System:  info, pins");
    Serial.println("  Exit:    exit (return to normal mode)");
    Serial.println("===========================================");
    Serial.println();
}

void scanI2CBus() {
    Serial.println("Scanning I2C bus...");
    Wire.begin(PIN_SDA, PIN_SCL);

    int devicesFound = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("  Found device at 0x%02X", addr);
            if (addr == MPU6050_ADDRESS) {
                Serial.print(" (MPU6050)");
            }
            Serial.println();
            devicesFound++;
        }
    }

    if (devicesFound == 0) {
        Serial.println("  No I2C devices found!");
    } else {
        Serial.printf("  Total: %d device(s) found\n", devicesFound);
    }
}

void printPinInfo() {
    Serial.println();
    Serial.println("=== Pin Assignments ===");
    Serial.println("  I2C:");
    Serial.printf("    SDA: GPIO %d\n", PIN_SDA);
    Serial.printf("    SCL: GPIO %d\n", PIN_SCL);
    Serial.printf("    MPU6050 Address: 0x%02X\n", MPU6050_ADDRESS);
    Serial.println();
    Serial.println("  Motor 1 (Left Back):");
    Serial.printf("    IN1: GPIO %d\n", MOTOR1_IN1);
    Serial.printf("    IN2: GPIO %d\n", MOTOR1_IN2);
    Serial.printf("    IN3: GPIO %d\n", MOTOR1_IN3);
    Serial.printf("    IN4: GPIO %d\n", MOTOR1_IN4);
    Serial.println();
    Serial.println("  Motor 2 (Right Back):");
    Serial.printf("    IN1: GPIO %d\n", MOTOR2_IN1);
    Serial.printf("    IN2: GPIO %d\n", MOTOR2_IN2);
    Serial.printf("    IN3: GPIO %d\n", MOTOR2_IN3);
    Serial.printf("    IN4: GPIO %d\n", MOTOR2_IN4);
    Serial.println();
    Serial.println("  User Interface:");
    Serial.printf("    Button: GPIO %d (Active LOW with pull-up)\n", PIN_BUTTON);
    Serial.printf("    RGB LED: R=GPIO %d, G=GPIO %d, B=GPIO %d\n", PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE);
    Serial.println();
    Serial.println("=== Configuration ===");
    Serial.printf("  Steps per revolution: %d\n", STEPS_PER_REVOLUTION);
    Serial.printf("  Default motor speed: %d RPM\n", MOTOR_SPEED_RPM);
    Serial.printf("  Level tolerance: %.2f deg\n", LEVEL_TOLERANCE_DEG);
    Serial.printf("  Stability timeout: %d ms\n", STABILITY_TIMEOUT_MS);
    Serial.println();
}

void handleTestModeState() {
    unsigned long currentTime = millis();

    // Handle button test mode - print button events
    if (testModeButtonTest) {
        ButtonEvent event = button.update();
        if (event == ButtonEvent::SHORT_PRESS) {
            Serial.println("[BUTTON] SHORT_PRESS detected");
        } else if (event == ButtonEvent::LONG_PRESS) {
            Serial.println("[BUTTON] LONG_PRESS detected");
        }
    }

    // Handle continuous IMU streaming (10 Hz)
    if (testModeIMUStreaming && (currentTime - testModeLastStreamTime >= 100)) {
        testModeLastStreamTime = currentTime;
        imu.update();
        const IMUData& data = imu.getData();
        Serial.printf("[IMU] P:%.2f R:%.2f | Ax:%.3f Ay:%.3f Az:%.3f | Gx:%.1f Gy:%.1f Gz:%.1f | M1:%ld M2:%ld\n",
                      data.pitch, data.roll,
                      data.accelX, data.accelY, data.accelZ,
                      data.gyroX, data.gyroY, data.gyroZ,
                      motors.getPosition1(), motors.getPosition2());
    }

    // Handle continuous motor rotation
    if (testModeMotor1Continuous) {
        motors.moveMotor1(10);  // Small increments for smooth rotation
    }
    if (testModeMotor2Continuous) {
        motors.moveMotor2(10);
    }

    // Handle LED cycle test (2s per pattern)
    if (testModeLEDCycle && (currentTime - testModeLastLEDCycleTime >= 2000)) {
        testModeLastLEDCycleTime = currentTime;

        const LEDPattern patterns[] = {
            LEDPattern::SOLID,
            LEDPattern::SLOW_BLINK,
            LEDPattern::FAST_BLINK,
            LEDPattern::DOUBLE_PULSE,
            LEDPattern::ERROR_BLINK,
            LEDPattern::OFF
        };
        const char* patternNames[] = {
            "SOLID", "SLOW_BLINK", "FAST_BLINK", "DOUBLE_PULSE", "ERROR_BLINK", "OFF"
        };
        const int numPatterns = 6;

        statusLED.setPattern(patterns[testModeLEDCycleIndex]);
        Serial.printf("[LED] Pattern: %s\n", patternNames[testModeLEDCycleIndex]);

        testModeLEDCycleIndex = (testModeLEDCycleIndex + 1) % numPatterns;
    }
}

void handleTestModeCommands(String& input) {
    // Exit command
    if (input.equalsIgnoreCase("exit")) {
        Serial.println("Exiting test mode...");
        motors.release();
        testModeIMUStreaming = false;
        testModeButtonTest = false;
        testModeMotor1Continuous = false;
        testModeMotor2Continuous = false;
        testModeLEDCycle = false;
        changeState(SystemState::IDLE);
        return;
    }

    // Help/menu
    if (input.equalsIgnoreCase("help") || input.equalsIgnoreCase("menu") || input == "?") {
        printTestModeMenu();
        return;
    }

    // ==================== IMU Commands ====================

    if (input.equalsIgnoreCase("scan")) {
        scanI2CBus();
        return;
    }

    if (input.equalsIgnoreCase("imu")) {
        Serial.println("Initializing IMU...");
        if (imu.begin()) {
            Serial.println("IMU initialized successfully!");
            // Read WHO_AM_I register
            Wire.beginTransmission(MPU6050_ADDRESS);
            Wire.write(0x75);  // WHO_AM_I register
            Wire.endTransmission(false);
            Wire.requestFrom((uint8_t)MPU6050_ADDRESS, (uint8_t)1);
            if (Wire.available()) {
                uint8_t whoAmI = Wire.read();
                Serial.printf("  WHO_AM_I: 0x%02X (expected 0x68)\n", whoAmI);
            }
        } else {
            Serial.println("ERROR: Failed to initialize IMU!");
        }
        return;
    }

    if (input.equalsIgnoreCase("read")) {
        imu.update();
        const IMUData& data = imu.getData();
        Serial.println();
        Serial.println("=== Single IMU Reading ===");
        Serial.printf("  Pitch: %.2f deg\n", data.pitch);
        Serial.printf("  Roll:  %.2f deg\n", data.roll);
        Serial.printf("  Accel: X=%.3fg Y=%.3fg Z=%.3fg\n", data.accelX, data.accelY, data.accelZ);
        Serial.printf("  Gyro:  X=%.1f Y=%.1f Z=%.1f deg/s\n", data.gyroX, data.gyroY, data.gyroZ);
        Serial.printf("  Temp:  %.1f C\n", data.temperature);
        Serial.println();
        return;
    }

    if (input.equalsIgnoreCase("stream")) {
        testModeIMUStreaming = !testModeIMUStreaming;
        Serial.printf("IMU streaming: %s\n", testModeIMUStreaming ? "ON (10 Hz)" : "OFF");
        if (testModeIMUStreaming) {
            Serial.println("  Format: P:pitch R:roll | Ax Ay Az | Gx Gy Gz");
        }
        return;
    }

    if (input.equalsIgnoreCase("cal")) {
        Serial.println("Starting IMU calibration...");
        Serial.println("Keep the platform STILL and LEVEL!");
        imu.calibrate();
        Serial.println("Calibration complete.");
        return;
    }

    if (input.equalsIgnoreCase("raw")) {
        imu.update();
        const IMURawData& raw = imu.getRawData();
        Serial.println();
        Serial.println("=== Raw IMU Values ===");
        Serial.printf("  Accel: X=%d Y=%d Z=%d\n", raw.accelX, raw.accelY, raw.accelZ);
        Serial.printf("  Gyro:  X=%d Y=%d Z=%d\n", raw.gyroX, raw.gyroY, raw.gyroZ);
        Serial.printf("  Temp:  %d (raw)\n", raw.temperature);
        Serial.println();
        return;
    }

    // ==================== Button Commands ====================

    if (input.equalsIgnoreCase("btn")) {
        testModeButtonTest = !testModeButtonTest;
        Serial.printf("Button test mode: %s\n", testModeButtonTest ? "ON" : "OFF");
        if (testModeButtonTest) {
            Serial.println("  Press the button to see events...");
        }
        return;
    }

    // ==================== LED Commands ====================

    if (input.startsWith("led ") || input.startsWith("LED ")) {
        String ledCmd = input.substring(4);
        ledCmd.trim();
        testModeLEDCycle = false;  // Stop cycling if running

        if (ledCmd.equalsIgnoreCase("on")) {
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: SOLID (on)");
        } else if (ledCmd.equalsIgnoreCase("off")) {
            statusLED.setPattern(LEDPattern::OFF);
            Serial.println("LED: OFF");
        } else if (ledCmd.equalsIgnoreCase("slow")) {
            statusLED.setPattern(LEDPattern::SLOW_BLINK);
            Serial.println("LED: SLOW_BLINK (1 Hz)");
        } else if (ledCmd.equalsIgnoreCase("fast")) {
            statusLED.setPattern(LEDPattern::FAST_BLINK);
            Serial.println("LED: FAST_BLINK (4 Hz)");
        } else if (ledCmd.equalsIgnoreCase("pulse")) {
            statusLED.setPattern(LEDPattern::DOUBLE_PULSE);
            Serial.println("LED: DOUBLE_PULSE");
        } else if (ledCmd.equalsIgnoreCase("error")) {
            statusLED.setPattern(LEDPattern::ERROR_BLINK);
            Serial.println("LED: ERROR_BLINK (10 Hz)");
        } else if (ledCmd.equalsIgnoreCase("cycle")) {
            testModeLEDCycle = true;
            testModeLEDCycleIndex = 0;
            testModeLastLEDCycleTime = millis();
            Serial.println("LED: Cycling through all patterns (2s each)...");
        // Color commands — set color and turn on solid
        } else if (ledCmd.equalsIgnoreCase("red")) {
            statusLED.setColor(LEDColors::RED);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: RED");
        } else if (ledCmd.equalsIgnoreCase("green")) {
            statusLED.setColor(LEDColors::GREEN);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: GREEN");
        } else if (ledCmd.equalsIgnoreCase("blue")) {
            statusLED.setColor(LEDColors::BLUE);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: BLUE");
        } else if (ledCmd.equalsIgnoreCase("yellow")) {
            statusLED.setColor(LEDColors::YELLOW);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: YELLOW");
        } else if (ledCmd.equalsIgnoreCase("cyan")) {
            statusLED.setColor(LEDColors::CYAN);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: CYAN");
        } else if (ledCmd.equalsIgnoreCase("purple")) {
            statusLED.setColor(LEDColors::PURPLE);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: PURPLE");
        } else if (ledCmd.equalsIgnoreCase("white")) {
            statusLED.setColor(LEDColors::WHITE);
            statusLED.setPattern(LEDPattern::SOLID);
            Serial.println("LED: WHITE");
        } else {
            Serial.println("Unknown LED command.");
            Serial.println("  Patterns: on, off, slow, fast, pulse, error, cycle");
            Serial.println("  Colors:   red, green, blue, yellow, cyan, purple, white");
        }
        return;
    }

    // ledtest - raw GPIO test bypassing LEDC, tests each pin individually
    if (input.equalsIgnoreCase("ledtest")) {
        Serial.println("LED raw GPIO test - bypassing LEDC PWM:");
        uint8_t pins[] = {PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE};
        const char* names[] = {"RED", "GREEN", "BLUE"};

        // Detach LEDC first so we can use raw GPIO
        ledcDetachPin(PIN_LED_RED);
        ledcDetachPin(PIN_LED_GREEN);
        ledcDetachPin(PIN_LED_BLUE);

        for (int i = 0; i < 3; i++) {
            pinMode(pins[i], OUTPUT);
        }

        // Test each color HIGH
        for (int i = 0; i < 3; i++) {
            Serial.printf("  %s (GPIO %d) HIGH... ", names[i], pins[i]);
            digitalWrite(pins[i], HIGH);
            delay(1500);
            digitalWrite(pins[i], LOW);
            Serial.println("done");
            delay(300);
        }

        // Now test inverted (LOW = on for common anode)
        Serial.println("  Now testing INVERTED (for common anode):");
        for (int i = 0; i < 3; i++) {
            // All HIGH first (off for common anode)
            for (int j = 0; j < 3; j++) digitalWrite(pins[j], HIGH);
            Serial.printf("  %s (GPIO %d) LOW... ", names[i], pins[i]);
            digitalWrite(pins[i], LOW);
            delay(1500);
            digitalWrite(pins[i], HIGH);
            Serial.println("done");
            delay(300);
        }

        // All off
        for (int i = 0; i < 3; i++) digitalWrite(pins[i], LOW);

        // Re-attach LEDC
        ledcAttachPin(PIN_LED_RED, LEDC_CHANNEL_RED);
        ledcAttachPin(PIN_LED_GREEN, LEDC_CHANNEL_GREEN);
        ledcAttachPin(PIN_LED_BLUE, LEDC_CHANNEL_BLUE);

        Serial.println("LED test complete. If inverted worked, your LED is common anode.");
        return;
    }

    // ==================== Motor Commands ====================

    // m1 <steps> or m2 <steps>
    if ((input.startsWith("m1 ") || input.startsWith("M1 ")) && input.length() > 3) {
        int steps = input.substring(3).toInt();
        Serial.printf("Moving motor 1 by %d steps...\n", steps);
        motors.moveMotor1(steps);
        Serial.println("Done.");
        return;
    }

    if ((input.startsWith("m2 ") || input.startsWith("M2 ")) && input.length() > 3) {
        int steps = input.substring(3).toInt();
        Serial.printf("Moving motor 2 by %d steps...\n", steps);
        motors.moveMotor2(steps);
        Serial.println("Done.");
        return;
    }

    // m1c - toggle continuous motor 1
    if (input.equalsIgnoreCase("m1c")) {
        testModeMotor1Continuous = !testModeMotor1Continuous;
        Serial.printf("Motor 1 continuous: %s\n", testModeMotor1Continuous ? "ON" : "OFF");
        if (!testModeMotor1Continuous) {
            motors.release();
        }
        return;
    }

    // m2c - toggle continuous motor 2
    if (input.equalsIgnoreCase("m2c")) {
        testModeMotor2Continuous = !testModeMotor2Continuous;
        Serial.printf("Motor 2 continuous: %s\n", testModeMotor2Continuous ? "ON" : "OFF");
        if (!testModeMotor2Continuous) {
            motors.release();
        }
        return;
    }

    // mstop - stop all motors
    if (input.equalsIgnoreCase("mstop")) {
        testModeMotor1Continuous = false;
        testModeMotor2Continuous = false;
        motors.release();
        Serial.println("All motors stopped.");
        return;
    }

    // mpos - query motor positions and limits
    if (input.equalsIgnoreCase("mpos")) {
        Serial.printf("[MPOS] M1:%ld M2:%ld MIN:%ld MAX:%ld\n",
                      motors.getPosition1(), motors.getPosition2(),
                      motors.getMinPosition(), motors.getMaxPosition());
        return;
    }

    // mreset - reset motor position counters to zero
    if (input.equalsIgnoreCase("mreset")) {
        motors.resetPositions();
        Serial.println("[MRESET] Motor positions reset to 0");
        return;
    }
    if (input.equalsIgnoreCase("mreset1")) {
        motors.resetPosition1();
        Serial.printf("[MRESET] M1 reset to 0 (M2 still at %ld)\n", motors.getPosition2());
        return;
    }
    if (input.equalsIgnoreCase("mreset2")) {
        motors.resetPosition2();
        Serial.printf("[MRESET] M2 reset to 0 (M1 still at %ld)\n", motors.getPosition1());
        return;
    }

    // mset <pos> - set both motor position counters to a value
    if (input.startsWith("mset ") || input.startsWith("MSET ")) {
        long pos = input.substring(5).toInt();
        motors.setPosition1(pos);
        motors.setPosition2(pos);
        saveMotorPositions();
        Serial.printf("[MSET] Both motors set to %ld\n", pos);
        return;
    }

    // coiltest - energize each Motor 2 coil one at a time to verify wiring
    if (input.equalsIgnoreCase("coiltest")) {
        Serial.println("Motor 2 coil test - energizing each pin for 1 second:");
        uint8_t pins[] = {MOTOR2_IN1, MOTOR2_IN2, MOTOR2_IN3, MOTOR2_IN4};
        const char* names[] = {"IN1", "IN2", "IN3", "IN4"};

        // Make sure all are LOW first
        for (int i = 0; i < 4; i++) digitalWrite(pins[i], LOW);

        for (int i = 0; i < 4; i++) {
            Serial.printf("  %s (GPIO %d) HIGH... ", names[i], pins[i]);
            digitalWrite(pins[i], HIGH);
            delay(1000);
            digitalWrite(pins[i], LOW);
            Serial.println("done");
            delay(300);
        }
        Serial.println("Coil test complete.");
        Serial.println("Expected order: feel 4 distinct click/hums in sequence.");
        Serial.println("If two adjacent coils feel the same, those pins may be swapped.");
        return;
    }

    // munlock - remove position limits (for finding physical extents)
    if (input.equalsIgnoreCase("munlock")) {
        motors.setLimits(-99999, 99999);
        Serial.println("[MUNLOCK] Position limits removed (-99999 to 99999)");
        return;
    }

    // mlock - restore default position limits
    if (input.equalsIgnoreCase("mlock")) {
        motors.setLimits(MOTOR_MIN_POSITION, MOTOR_MAX_POSITION);
        Serial.printf("[MLOCK] Position limits restored (%d to %d)\n", MOTOR_MIN_POSITION, MOTOR_MAX_POSITION);
        return;
    }

    // mspeed <rpm> - set motor speed
    if (input.startsWith("mspeed ") || input.startsWith("MSPEED ")) {
        int rpm = input.substring(7).toInt();
        if (rpm >= 1 && rpm <= 15) {
            testModeMotorSpeed = rpm;
            motors.setSpeed(rpm);
            Serial.printf("Motor speed set to %d RPM\n", rpm);
        } else {
            Serial.println("Invalid speed. Use 1-15 RPM.");
        }
        return;
    }

    // ==================== System Commands ====================

    if (input.equalsIgnoreCase("info") || input.equalsIgnoreCase("pins")) {
        printPinInfo();
        return;
    }

    // Unknown command
    Serial.printf("Unknown command: '%s'. Type 'help' for menu.\n", input.c_str());
}
