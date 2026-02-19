# Feature: Complementary Filter Alpha Tuning

## Metadata
- **Priority:** High
- **Complexity:** Low
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Changed complementary filter alpha from 0.02 to 0.15 for faster convergence. Old value took 15+ seconds to settle; new value converges 95% in ~4 seconds while still smoothing motor vibration.

## Requirements
- [x] Faster IMU angle convergence
- [x] Still smooths stepper motor vibration noise

## Files Modified
- `include/config.h` — COMPLEMENTARY_ALPHA 0.02 → 0.15

## Status
- **Completed:** 2026-02-06
