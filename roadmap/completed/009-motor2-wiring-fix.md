# Feature: Motor 2 Coil Wiring Fix

## Metadata
- **Priority:** Critical
- **Complexity:** Low
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Fixed Motor 2 (right back leg) coil firing order. Original pin order (16,4,15,13) caused the motor to hum without rotating — coils were energized in the wrong sequence for proper magnetic field rotation. Correct order is (16,13,15,4).

## Diagnosis Process
1. Added `coiltest` command to energize each Motor 2 coil individually
2. All 4 coils responded (clicked) — ruled out wiring/ULN2003 issues
3. Tried swapping IN2↔IN3 — still hummed
4. Tried swapping IN2↔IN4 — motor turned successfully
5. Confirmed via serial monitor with `m2 500` command

## Requirements
- [x] Identify correct coil firing order for Motor 2
- [x] Add coiltest diagnostic command
- [x] Verify motor rotates in both directions

## Files Modified
- `include/config.h` — MOTOR2_IN2=13, MOTOR2_IN3=15, MOTOR2_IN4=4
- `src/main.cpp` — Added coiltest command

## Status
- **Completed:** 2026-02-19

## Notes
- Physical wiring confirmed: IN3 soldered to GPIO 15 (D15)
- Motor 1 pin order (19,18,5,17) was already correct
