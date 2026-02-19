# Feature: Motor Position Safety Limits

## Metadata
- **Priority:** High
- **Complexity:** Medium
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Added position safety limits to prevent motors from over-extending threaded rod legs. Limits are enforced at the lowest level (stepMotor1/stepMotor2) so all callers are guarded.

## Requirements
- [x] Position tracking for both motors
- [x] Configurable min/max position limits in config.h
- [x] Limits enforced at stepMotor level (guards all call paths)
- [x] mpos command to query positions and limits
- [x] mreset command to reset position counters
- [x] munlock/mlock commands for test mode

## Files Modified
- `include/config.h` — MOTOR_MIN_POSITION, MOTOR_MAX_POSITION
- `lib/StepperController/StepperController.h` — Position tracking, limit methods
- `lib/StepperController/StepperController.cpp` — Limit enforcement in stepMotor1/2
- `src/main.cpp` — mpos, mreset, munlock, mlock commands

## Status
- **Completed:** 2026-02-06

## Notes
- Default limits: MIN=0, MAX=70000 (placeholder until physical limits measured)
- ±2048 steps = 1 full revolution of 28BYJ-48
