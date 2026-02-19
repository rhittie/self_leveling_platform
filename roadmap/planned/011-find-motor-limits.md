# Feature: Find and Set Physical Motor Limits

## Metadata
- **Priority:** High
- **Complexity:** Low
- **Estimated Sessions:** 1
- **Dependencies:** 010-motor-limits-gui-rewrite

## Description
Use the motor limits GUI to move both motors to their physical IN (fully retracted) and OUT (fully extended) positions, then update config.h with the real MOTOR_MIN_POSITION and MOTOR_MAX_POSITION values.

## Requirements
- [ ] Find Motor 1 IN and OUT positions
- [ ] Find Motor 2 IN and OUT positions
- [ ] Update config.h with measured values
- [ ] Verify limits prevent over-extension

## Status
- **Created:** 2026-02-19
