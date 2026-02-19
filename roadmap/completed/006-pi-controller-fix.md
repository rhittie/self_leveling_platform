# Feature: PI Controller Error Sign Fix and Gain Tuning

## Metadata
- **Priority:** Critical
- **Complexity:** Medium
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Fixed critical bug where PI controller had wrong error sign (error = -actual instead of error = actual). The plant has negative gain (positive motor steps decrease angles), so error=actual gives correct net negative feedback. Tuned PI gains for hardware.

## Requirements
- [x] Fix error sign in PI controller
- [x] Tune Kp/Ki gains for pitch and roll
- [x] Verify correction direction with hardware test

## Technical Details
- Plant gain: positive motor steps → negative angle change
- Error = actual (not -actual) gives net negative feedback through the negative plant
- Kp_pitch=1.0, Ki_pitch=0.05 (pitch ~2x less effective per step)
- Kp_roll=0.5, Ki_roll=0.03
- stepsPerDegree=60

## Files Modified
- `lib/LevelingController/LevelingController.cpp` — Error sign fix
- `include/config.h` — PI gain defaults

## Status
- **Completed:** 2026-02-06

## Notes
- Verified with 20-cycle correction test: roll converged from 0.787 to 0.460 degrees
- Motor mapping: M1 dP=-0.22, dR=+0.42 | M2 dP=-0.20, dR=-0.45 per 1000 steps
