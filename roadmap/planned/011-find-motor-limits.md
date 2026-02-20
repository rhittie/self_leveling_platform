# Feature: Find and Set Physical Motor Limits

## Metadata
- **Priority:** High
- **Complexity:** Low
- **Estimated Sessions:** 1
- **Dependencies:** 010-motor-limits-gui-rewrite

## Description
Use the motor limits GUI to move both motors to their physical IN (fully retracted) and OUT (fully extended) positions, then update config.h with the real MOTOR_MIN_POSITION and MOTOR_MAX_POSITION values. Current values (0 and 70000) are placeholders that don't reflect real travel range.

## User Story
As a developer, I want to know the exact step counts for motor travel limits so that the firmware never drives the threaded rods beyond their physical range.

## Requirements
- [ ] Find Motor 1 IN (fully retracted) position in steps
- [ ] Find Motor 1 OUT (fully extended) position in steps
- [ ] Find Motor 2 IN (fully retracted) position in steps
- [ ] Find Motor 2 OUT (fully extended) position in steps
- [ ] Update MOTOR_MIN_POSITION in config.h with measured value
- [ ] Update MOTOR_MAX_POSITION in config.h with measured value
- [ ] Consider per-motor limits if M1 and M2 differ significantly
- [ ] Verify limits prevent over-extension/retraction in both directions

## Technical Approach
1. Run `python tools/motor_limits_gui.py` to open the motor control GUI
2. Use `mreset` to zero both motor positions
3. Move each motor IN (retract) until it physically stops — note the step count
4. Move each motor OUT (extend) until it physically stops — note the step count
5. The total travel range = |IN steps| + |OUT steps|
6. Set MIN = 0 (or negative if centering at 0) and MAX = total travel
7. If M1 and M2 have different ranges, may need per-motor limits (separate defines)

## Subtasks
1. [ ] Start motor limits GUI: `python tools/motor_limits_gui.py`
2. [ ] Reset both motor positions to zero with `mreset`
3. [ ] Slowly move Motor 1 inward until physical stop — record step count
4. [ ] Slowly move Motor 1 outward until physical stop — record step count
5. [ ] Repeat for Motor 2
6. [ ] Calculate safe MIN/MAX values (leave ~50 step margin from physical stops)
7. [ ] Update config.h: `MOTOR_MIN_POSITION` and `MOTOR_MAX_POSITION`
8. [ ] If limits differ per motor, add `MOTOR1_MIN_POSITION`, `MOTOR1_MAX_POSITION`, `MOTOR2_MIN_POSITION`, `MOTOR2_MAX_POSITION` and update StepperController
9. [ ] Build and upload: `pio run -t upload`
10. [ ] Verify with `mpos` command that limits are enforced

## Files to Create/Modify
- [ ] `include/config.h` — Update MOTOR_MIN_POSITION and MOTOR_MAX_POSITION with real values
- [ ] `lib/StepperController/StepperController.h` — If per-motor limits needed
- [ ] `lib/StepperController/StepperController.cpp` — If per-motor limits needed

## Testing Plan
- [ ] Move each motor to extremes and verify `mpos` reports correct position
- [ ] Try to move past limits and verify firmware stops the motor
- [ ] Confirm platform doesn't hit mechanical stops during normal operation

## Status
- **Created:** 2026-02-19
- **Planned:** 2026-02-20

## Notes
- Current limits: MIN=0, MAX=70000 — these are placeholders
- Motor 2 has reversed lead screw orientation (GUI flips direction, firmware doesn't yet)
- Be gentle when finding physical stops — don't strip the threaded rod
- Motor position persistence (NVS) is now in uncommitted code — positions survive reboots
- 2048 steps = 1 full revolution of the 28BYJ-48

## Session Log
