# Feature: Core Firmware

## Metadata
- **Priority:** High
- **Complexity:** High
- **Estimated Sessions:** Multiple
- **Dependencies:** None (foundational)

## Description
Complete ESP32 firmware for a self-leveling prism platform. Includes IMU sensing, dual stepper motor control, PI leveling algorithm, state machine, serial debug CLI, status LED feedback, and button input handling.

## Requirements
- [x] MPU6050 IMU driver with I2C communication
- [x] Complementary filter for pitch/roll angle estimation
- [x] IMU calibration routine (200-sample averaging)
- [x] Motion detection (accel + gyro thresholds)
- [x] Dual 28BYJ-48 stepper motor control via ULN2003 drivers
- [x] Half-step sequencing for smooth motor operation
- [x] Bresenham-interleaved dual motor coordination
- [x] PI controller for pitch and roll correction
- [x] Integral windup protection
- [x] System state machine (IDLE, INITIALIZING, WAIT_FOR_STABLE, LEVELING, LEVEL_OK, ERROR, TEST_MODE)
- [x] Serial CLI with debug commands
- [x] Test/admin mode with full hardware exercising
- [x] Status LED patterns (off, solid, slow blink, fast blink, double pulse, error)
- [x] Debounced button handler with short/long press detection

## Technical Approach
- **Architecture:** Modular libraries under `lib/`, each handling one hardware subsystem
- **IMU Fusion:** Complementary filter (alpha=0.02, favoring gyro for fast response)
- **Control:** PI controller (not PID — derivative term adds noise with stepper vibration)
- **Motor Mapping:** Pitch = both motors same direction; Roll = motors opposite directions
- **State Machine:** Event-driven transitions with button and sensor inputs

## Files Created
- [x] `src/main.cpp` — Main firmware (895 lines), state machine, serial CLI
- [x] `include/config.h` — Pin definitions, timing constants, tuning parameters
- [x] `include/types.h` — Shared enums (SystemState, ButtonEvent, LEDPattern) and structs
- [x] `lib/MPU6050Handler/MPU6050Handler.h` + `.cpp` — IMU driver
- [x] `lib/StepperController/StepperController.h` + `.cpp` — Dual motor control
- [x] `lib/LevelingController/LevelingController.h` + `.cpp` — PI control algorithm
- [x] `lib/ButtonHandler/ButtonHandler.h` + `.cpp` — Button input handling
- [x] `lib/StatusLED/StatusLED.h` + `.cpp` — LED pattern output
- [x] `platformio.ini` — PlatformIO build configuration

## Status
- **Completed:** 2026-02-05

## Notes
- Motor step conversion: 2048 steps = 1 full revolution (28BYJ-48 with gearbox)
- MPU6050 I2C address: 0x68 (AD0 tied to GND)
- Platform geometry: 3 legs — 1 fixed front, 2 adjustable rear (one per motor)
- Serial CLI includes both normal mode commands and an admin/test mode for raw hardware access
- Default PI gains: Kp=2.0, Ki=0.1 for both pitch and roll (adjustable via serial)
