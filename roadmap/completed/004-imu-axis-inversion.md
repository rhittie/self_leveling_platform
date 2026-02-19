# Feature: IMU Axis Inversion Flags

## Metadata
- **Priority:** High
- **Complexity:** Low
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Added configurable INVERT_PITCH and INVERT_ROLL flags in config.h to correct IMU orientation if mounted differently than expected. Inversion applied at sensor level in processData() and applyComplementaryFilter().

## Requirements
- [x] INVERT_PITCH flag in config.h
- [x] INVERT_ROLL flag in config.h
- [x] Inversion applied at sensor read level so all consumers get correct angles

## Files Modified
- `include/config.h` — INVERT_PITCH, INVERT_ROLL defines
- `lib/MPU6050Handler/MPU6050Handler.cpp` — Axis inversion in processData() and filter

## Status
- **Completed:** 2026-02-06

## Notes
- Both flags currently false — hardware test confirmed IMU reads correct signs
- The actual bug was in the PI controller error sign, not the IMU orientation
