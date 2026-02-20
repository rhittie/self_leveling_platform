# Feature: Configurable Stability Lockout Timer

## Metadata
- **Priority:** High
- **Complexity:** Low
- **Estimated Sessions:** 1 (partial session)
- **Dependencies:** None

## Description
Make the stability lockout timer (time the platform must be motionless before leveling starts) configurable at runtime via a serial test mode command. Currently `STABILITY_TIMEOUT_MS` is a compile-time `#define` set to 3000ms. This feature makes it a runtime value that can be adjusted without reflashing, while keeping 3 seconds as the default.

The lockout works in `WAIT_FOR_STABLE` state: the IMU's `isMoving()` must return false continuously for N seconds before transitioning to `LEVELING`. Any detected motion resets the timer.

## User Story
As a user, I want to adjust how long the platform waits after movement stops before it begins leveling, so I can tune the responsiveness vs. stability tradeoff during testing.

## Requirements
- [ ] Stability timeout defaults to 3 seconds (3000ms) — same as current behavior
- [ ] Stability timeout adjustable at runtime via serial command in test mode
- [ ] Current timeout value shown in system status (`s` command)
- [ ] Status printout in `printStatus()` shows the configured (not #define) value
- [ ] Existing `STABILITY_TIMEOUT_MS` becomes the default, not the active value

## Technical Approach

### 1. Add to SystemConfig struct
In `include/types.h`, add a `stabilityTimeoutMs` field to `SystemConfig`:
```cpp
struct SystemConfig {
    float kpPitch;
    float kiPitch;
    float kpRoll;
    float kiRoll;
    float levelTolerance;
    bool continuousLogging;
    unsigned long stabilityTimeoutMs;  // NEW: lockout timer (ms)
};
```

### 2. Initialize in setup()
In `main.cpp` `setup()`, alongside the other config defaults:
```cpp
config.stabilityTimeoutMs = STABILITY_TIMEOUT_MS;  // 3000ms default
```

### 3. Replace compile-time usage
In `handleWaitForStableState()` (main.cpp ~line 345), change:
```cpp
// Before:
if (currentTime - lastStableTime >= STABILITY_TIMEOUT_MS) {
// After:
if (currentTime - lastStableTime >= config.stabilityTimeoutMs) {
```

### 4. Add serial command
Add a `st <seconds>` command (parallels existing `t <deg>` for tolerance and `p <kp> <ki>` for PI gains). Available in both normal and test mode:
```
st 5     → Set stability timeout to 5 seconds
st 1.5   → Set stability timeout to 1.5 seconds
st       → Show current stability timeout
```
Parse as float seconds, convert to milliseconds: `config.stabilityTimeoutMs = (unsigned long)(seconds * 1000);`
Enforce a minimum of 500ms and maximum of 30000ms (30s) to prevent nonsensical values.

### 5. Show in status
In `printStatus()`, change the existing line:
```cpp
// Before:
Serial.printf("  Stability timeout: %d ms\n", STABILITY_TIMEOUT_MS);
// After:
Serial.printf("  Stability timeout: %lu ms (%.1f sec)\n", config.stabilityTimeoutMs, config.stabilityTimeoutMs / 1000.0f);
```

## Subtasks
1. [ ] Add `stabilityTimeoutMs` field to `SystemConfig` in `include/types.h`
2. [ ] Initialize `config.stabilityTimeoutMs = STABILITY_TIMEOUT_MS` in `setup()` in main.cpp
3. [ ] Replace `STABILITY_TIMEOUT_MS` with `config.stabilityTimeoutMs` in `handleWaitForStableState()` (line ~345)
4. [ ] Add `st <seconds>` serial command parsing (in the command handler section alongside `t` and `p`)
5. [ ] Update `printStatus()` to show runtime value instead of compile-time constant
6. [ ] Build and upload: `pio run -t upload`
7. [ ] Test: default behavior unchanged (3s lockout)
8. [ ] Test: `st 5` → verify 5 second lockout before leveling starts
9. [ ] Test: `st 1` → verify 1 second lockout
10. [ ] Test: `s` command shows updated timeout value

## Files to Create/Modify
- [ ] `include/types.h` — Add `stabilityTimeoutMs` to `SystemConfig` struct
- [ ] `src/main.cpp` — Initialize config field, replace #define usage, add `st` command, update status display

## Testing Plan
- [ ] Default behavior: press button, move platform, stop → leveling starts after 3 seconds
- [ ] `st 5`: leveling waits 5 seconds after motion stops
- [ ] `st 1`: leveling waits only 1 second
- [ ] `st 0.5`: minimum allowed (500ms)
- [ ] `st 60`: rejected or clamped to 30s max
- [ ] `s` status shows current stability timeout value
- [ ] Shake platform during WAIT_FOR_STABLE → timer resets, waits full duration again

## Status
- **Created:** 2026-02-20
- **Planned:** 2026-02-20

## Notes
- `STABILITY_TIMEOUT_MS` remains as the compile-time default — it's still useful for the initial value
- This value does NOT persist across reboots (lives in RAM). If persistence is wanted later, can add to NVS save/load.
- Motion detection uses `MOTION_ACCEL_THRESHOLD` (0.15g) and `MOTION_GYRO_THRESHOLD` (10 deg/s) — these could also be made configurable in a future feature but are out of scope here.
- The `st` command name parallels `t` (tolerance) and `p` (PI gains) — short, mnemonic.
- Future: the web dashboard (014) will also expose this setting via the UI.

## Session Log
