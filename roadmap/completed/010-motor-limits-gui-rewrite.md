# Feature: Motor Limits GUI Rewrite

## Metadata
- **Priority:** High
- **Complexity:** Medium
- **Estimated Sessions:** 1
- **Dependencies:** 009-motor2-wiring-fix

## Description
Rewrote the motor limits GUI serial communication to fix intermittent Motor 2 failures. Replaced the worker thread queue system with direct serial writes and a threading lock. Added "Both Motors" simultaneous control, Motor 2 direction reversal, resizable window, and per-motor reset.

## Problem
Motor 2 worked reliably from serial monitor but failed intermittently through the GUI. Root cause: the worker thread queue was interfering with serial communication, causing garbled commands that disrupted the stepper coil sequence.

## Requirements
- [x] Reliable serial communication for both motors
- [x] Both Motors +/- control for simultaneous movement
- [x] Motor 2 direction reversal (lead screw physically reversed)
- [x] Correct position display (both motors show positive when extending)
- [x] Resizable window with expanding log panel
- [x] Set Zero button per motor
- [x] mreset1/mreset2 individual reset commands in firmware

## Files Modified
- `tools/motor_limits_gui.py` — Full serial rewrite, new UI controls
- `src/main.cpp` — mreset1, mreset2 commands
- `lib/StepperController/StepperController.h` — resetPosition1(), resetPosition2()
- `tools/motor_test_minimal.py` — Minimal serial test script (diagnostic)

## Status
- **Completed:** 2026-02-19

## Notes
- Worker thread queue removed entirely; all serial goes through _send_direct() with threading.Lock
- Motor 2 direction flip is GUI-only; firmware/leveling controller may need a flag too
