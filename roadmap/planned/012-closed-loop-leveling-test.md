# Feature: Full Closed-Loop Leveling Test

## Metadata
- **Priority:** High
- **Complexity:** Medium
- **Estimated Sessions:** 1-2
- **Dependencies:** 011-find-motor-limits

## Description
Run a complete closed-loop leveling test with the button press (or `level` serial command). Verify the platform actually levels itself, fine-tune PI gains if needed, and confirm the Motor 2 direction works correctly in the leveling controller.

## Requirements
- [ ] Start leveling via button or `level` command
- [ ] Verify both motors move in correct directions to level platform
- [ ] Confirm platform reaches LEVEL_OK state
- [ ] Fine-tune PI gains if needed
- [ ] Address Motor 2 direction reversal in firmware if needed (currently only reversed in GUI)

## Status
- **Created:** 2026-02-19

## Notes
- Motor 2 direction is flipped in the GUI but NOT in the leveling controller
- May need to add a MOTOR2_DIRECTION_INVERT flag in config.h
