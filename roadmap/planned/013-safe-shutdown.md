# Feature: Safe Shutdown on Long Press

## Metadata
- **Priority:** High
- **Complexity:** Low-Medium
- **Estimated Sessions:** 1
- **Dependencies:** None (NVS persistence code already exists in uncommitted working tree)

## Description
When the user presses and holds the button (long press, 2+ seconds), the system should save the current motor positions to NVS flash storage and enter a "safe shutdown" mode. This allows the user to turn off power without losing the motor position tracking. On next boot, the saved positions are restored automatically.

Currently, long press returns to IDLE state. This feature replaces that behavior with a save-and-shutdown sequence.

## User Story
As a user, I want to long-press the button to safely prepare for power-off so that when I turn the system back on, it remembers where the motors were positioned.

## Requirements
- [ ] Long press (2+ seconds) triggers safe shutdown sequence
- [ ] Motor positions saved to ESP32 NVS (flash) before shutdown
- [ ] LED provides visual feedback during save (e.g., solid color or special pattern)
- [ ] Serial output confirms save and shutdown state
- [ ] System enters low-power / halted state after save (motors disabled, minimal activity)
- [ ] On next boot, motor positions are restored from NVS (already implemented in uncommitted code)
- [ ] User can turn off power safely once in shutdown mode
- [ ] Short press or reset exits shutdown mode (or just power cycle)

## Technical Approach

### 1. Add SAFE_SHUTDOWN state to the state machine
Add `SAFE_SHUTDOWN` to the `SystemState` enum in `types.h`. This state represents "positions saved, safe to power off."

### 2. Change long press handler in main loop
Currently at line 162 in main.cpp, long press returns to IDLE. Change this to:
- Call `saveMotorPositions()` (already exists in uncommitted code)
- Disable motors (stop all coil drive to prevent current draw)
- Set LED to a distinctive pattern (e.g., solid green = "safe to power off")
- Change state to SAFE_SHUTDOWN
- Print confirmation to serial

### 3. Add SAFE_SHUTDOWN state handler
In the state machine switch, add a SAFE_SHUTDOWN case that:
- Keeps LED pattern active (solid green)
- Ignores short presses (or optionally: short press wakes back to IDLE)
- Does nothing else — system is halted, waiting for power off

### 4. Motor disable
Call `motors.stop()` or set all motor pins LOW to de-energize coils. This prevents the motors from holding position (which draws current) and lets the threaded rods free-wheel. The 28BYJ-48 has enough friction to hold position when powered off.

### 5. LED pattern for SAFE_SHUTDOWN
Add a new LED pattern in StatusLED — solid green (or slow breathing green) to indicate "safe to power off." This should be visually distinct from all other states.

## Subtasks
1. [ ] Add `SAFE_SHUTDOWN` to `SystemState` enum in `types.h`
2. [ ] Add LED pattern for SAFE_SHUTDOWN state in `StatusLED` library
3. [ ] Modify long press handler in `loop()` (main.cpp ~line 162):
   - Call `saveMotorPositions()`
   - Call `motors.stop()` or de-energize motor coils
   - Change state to `SAFE_SHUTDOWN`
   - Print save confirmation to serial
4. [ ] Add `SAFE_SHUTDOWN` case to state machine switch in main.cpp
5. [ ] Ensure `loadMotorPositions()` runs on boot (already in uncommitted `setup()`)
6. [ ] Add `msave` serial command for manual position save (test mode)
7. [ ] Build and upload: `pio run -t upload`
8. [ ] Test: long press → verify serial shows save message → verify LED shows shutdown pattern
9. [ ] Test: power cycle → verify positions restored on boot via serial output
10. [ ] Test: verify leveling still works correctly with restored positions

## Files to Create/Modify
- [ ] `include/types.h` — Add `SAFE_SHUTDOWN` to `SystemState` enum
- [ ] `lib/StatusLED/StatusLED.cpp` — Add LED pattern for SAFE_SHUTDOWN (solid green)
- [ ] `src/main.cpp` — Modify long press handler, add SAFE_SHUTDOWN state case, add `msave` command
- [ ] `lib/StepperController/StepperController.cpp` — Verify `stop()` method de-energizes coils (may need to add pin LOW logic)

## Testing Plan
- [ ] Long press → serial shows `[SAVE] Motor positions saved: M1=xxx M2=xxx`
- [ ] LED turns solid green after save
- [ ] Motors are de-energized (can turn shafts by hand)
- [ ] Power cycle → serial shows `[LOAD] Motor positions restored: M1=xxx M2=xxx`
- [ ] `mpos` after reboot shows correct positions
- [ ] Leveling works correctly with restored positions

## Status
- **Created:** 2026-02-20
- **Planned:** 2026-02-20

## Notes
- NVS persistence code (`saveMotorPositions()`/`loadMotorPositions()`) already exists in uncommitted main.cpp — just needs integration with button handler
- `setPosition1()`/`setPosition2()` already added to StepperController.h (uncommitted)
- Current long press behavior (return to IDLE) will be replaced
- Consider: should IDLE state also save on entry? Or only on explicit long press? Start with long press only.
- NVS has ~100,000 write cycle limit — saving only on shutdown (not every step) is fine
- 28BYJ-48 gearbox has enough self-locking friction to hold position without power
- If user wants to re-level after power cycle, they can short-press to start leveling from the saved position

## Session Log
