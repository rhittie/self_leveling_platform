# Feature: Full Closed-Loop Leveling Test

## Metadata
- **Priority:** High
- **Complexity:** Medium
- **Estimated Sessions:** 1-2
- **Dependencies:** 011-find-motor-limits

## Description
Run a complete closed-loop leveling test using the button press or `level` serial command. Verify the platform actually levels itself end-to-end, fine-tune PI gains if needed, and fix Motor 2 direction in the leveling controller (currently only reversed in the GUI, not in firmware).

## User Story
As a user, I want to press the button and have the platform automatically level itself so that the prism sits perfectly horizontal.

## Requirements
- [ ] Motor 2 direction handled correctly in leveling controller
- [ ] Platform moves from IDLE → INITIALIZING → WAIT_FOR_STABLE → LEVELING → LEVEL_OK
- [ ] Both motors move in correct directions to reduce pitch and roll error
- [ ] Platform reaches LEVEL_OK within reasonable time (~10-30 seconds)
- [ ] PI gains tuned for stable convergence (no oscillation)
- [ ] Level tolerance (0.5°) is achievable and the platform holds it

## Technical Approach
1. **Fix Motor 2 direction in firmware**: Add `MOTOR2_DIRECTION_INVERT` flag in config.h. Apply inversion in LevelingController when computing Motor 2 steps, OR invert at the StepperController level in `stepMotor2()`.
2. **Test leveling sequence**: Press button or send `level` to trigger the state machine. Monitor IMU output and motor movements via serial.
3. **Tune PI gains**: If platform oscillates, reduce Kp. If too slow, increase Kp. Watch for integral windup. Current gains: Kp_pitch=1.0, Kp_roll=0.5, Ki_pitch=0.05, Ki_roll=0.03.
4. **Verify convergence**: Platform should settle within 0.5° tolerance and transition to LEVEL_OK.

## Subtasks
1. [ ] Add `MOTOR2_DIRECTION_INVERT` define to config.h (default: true based on GUI testing)
2. [ ] Apply Motor 2 inversion in LevelingController or StepperController
3. [ ] Build and upload firmware
4. [ ] Place platform on a tilted surface (~5° off level)
5. [ ] Press button to start leveling sequence
6. [ ] Monitor serial output for pitch/roll convergence
7. [ ] If oscillating: reduce Kp values, rebuild, retest
8. [ ] If too slow: increase Kp values, rebuild, retest
9. [ ] If integral windup: check INTEGRAL_LIMIT, consider resetting on state transition
10. [ ] Verify LEVEL_OK state is reached and held
11. [ ] Test from multiple starting orientations
12. [ ] Record final tuned PI gains in config.h and Decisions Made

## Files to Create/Modify
- [ ] `include/config.h` — Add MOTOR2_DIRECTION_INVERT flag
- [ ] `lib/LevelingController/LevelingController.cpp` — Apply Motor 2 direction inversion in correction calculation
- [ ] `include/config.h` — Update PI gains if retuned
- [ ] `CLAUDE.md` — Record final gain values in Decisions Made

## Testing Plan
- [ ] Level from ~5° pitch tilt → verify converges to <0.5°
- [ ] Level from ~5° roll tilt → verify converges to <0.5°
- [ ] Level from combined pitch+roll tilt → verify both axes converge
- [ ] Verify no oscillation (motor hunting back and forth)
- [ ] Verify LEVEL_OK LED pattern activates
- [ ] Verify button press in LEVEL_OK returns to IDLE

## Status
- **Created:** 2026-02-19
- **Planned:** 2026-02-20

## Notes
- Motor 2 direction is flipped in the GUI but NOT in the leveling controller — this is a known gap
- Motor-to-axis mapping: M1 primarily affects roll (+), M2 primarily affects roll (-). Both contribute ~-0.2° pitch per 1000 steps.
- PI error sign: error = +actual (not -actual) because plant has negative gain
- Current motor position persistence (NVS) helps — positions survive reboots during testing
- May want to add a `msave` serial command to manually trigger position save after leveling

## Session Log
