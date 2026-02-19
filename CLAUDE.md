# CLAUDE.md - Self-Leveling Prism Platform

## Project Overview

**Self-Leveling Prism Platform**: ESP32-based firmware for a three-legged self-leveling platform using an MPU6050 accelerometer/gyroscope and two 28BYJ-48 stepper motors with ULN2003 drivers.

## Tech Stack

| Component | Technology | Purpose |
|-----------|------------|---------|
| MCU | ESP32 DevKit | Main controller |
| IMU | MPU6050 | Tilt sensing (I2C) |
| Motors | 28BYJ-48 x2 | Platform leveling |
| Drivers | ULN2003 x2 | Stepper motor drivers |
| Framework | Arduino (PlatformIO) | Firmware framework |
| Build Tool | PlatformIO | Build, upload, serial monitor |
| AI Assistant | Claude Code | Development assistant |
| Sync Tool | notion_claude_sync | Notion integration |

## File Structure

```
self_leveing_prism/
├── CLAUDE.md                          # This file - project intelligence
├── platformio.ini                     # PlatformIO build config
├── .gitignore                         # Git ignore rules
├── src/
│   └── main.cpp                       # Main firmware (state machine, serial CLI)
├── include/
│   ├── config.h                       # Pin definitions, constants, tuning params
│   └── types.h                        # Shared structs and enums
├── lib/
│   ├── MPU6050Handler/                # IMU driver (I2C, calibration, filtering)
│   ├── StepperController/             # Dual stepper motor control
│   ├── LevelingController/            # PI control loop for leveling
│   ├── ButtonHandler/                 # Debounced button with short/long press
│   └── StatusLED/                     # LED patterns for state feedback
├── WIRING_DIAGRAM.md                  # Text-based wiring reference (311 lines)
├── wiring_diagram.pen                 # Visual wiring diagram (Pencil format)
├── hardware_components                # Hardware reference info
├── roadmap/                           # Feature planning system
│   ├── backlog/                       # Ideas not yet planned
│   ├── planned/                       # Ready to build
│   ├── in-progress/                   # Currently working on
│   └── completed/                     # Finished features
│       ├── 000-core-firmware.md
│       ├── 001-claude-code-integration.md
│       └── 002-wiring-documentation.md
└── .claude/
    ├── commands/                      # Slash commands (/review, /builder, etc.)
    ├── tools/
    │   └── notion_claude_sync/        # Notion Kanban sync tool
    └── settings.local.json            # Claude Code permissions
```

## Current State

**Last Updated:** 2026-02-06

**What's Working:**
- [x] State machine: IDLE → INITIALIZING → WAIT_FOR_STABLE → LEVELING → LEVEL_OK
- [x] MPU6050 IMU driver with complementary filter
- [x] Dual 28BYJ-48 stepper motor control
- [x] PI control loop for pitch/roll correction
- [x] Serial CLI with debug commands and test mode
- [x] Status LED patterns per state
- [x] Button handler with short/long press
- [x] Motor position safety limits (±2048 steps / ±1 revolution)
- [x] IMU axis inversion flags (INVERT_PITCH / INVERT_ROLL in config.h)
- [x] Motor positions in stream output and continuous logging
- [x] `mpos` and `mreset` serial commands
- [x] Dashboard GUI tab with bubble level and motor position bars

**What's In Progress:**
- (none)

**What's Broken:**
- (none)

## Completed Features

1. Core firmware with state machine - Initial implementation
2. Serial debug CLI with test/admin mode
3. Claude Code template integration - 2026-02-05
4. Wiring documentation (WIRING_DIAGRAM.md + wiring_diagram.pen) - 2026-02-05
5. Roadmap populated with completed milestones - 2026-02-05
6. Motor position safety limits (±1 revolution) - 2026-02-06
7. IMU axis inversion flags for orientation fix - 2026-02-06
8. Dashboard GUI with bubble level + motor position bars - 2026-02-06
9. Motor position reporting in stream/logging + mpos/mreset commands - 2026-02-06
10. PI controller error sign fix + gain tuning for hardware - 2026-02-06
11. Complementary filter alpha 0.02→0.15 for faster convergence - 2026-02-06
12. Motor 2 coil wiring fix (pin order 16,13,15,4) - 2026-02-19
13. Motor limits GUI rewrite: direct serial, both-motors control, M2 direction fix - 2026-02-19

## Next Steps

1. [ ] Find and set motor limits using GUI (IN/OUT positions for both motors)
2. [ ] Update config.h with final MOTOR_MIN/MAX_POSITION values
3. [ ] Full closed-loop leveling test with button press
4. [ ] Set up Notion sync (optional - requires API key)

## Decisions Made

| Decision | Reasoning | Date |
|----------|-----------|------|
| ESP32 + PlatformIO | Good I2C/GPIO support, Arduino ecosystem | Prior |
| PI controller (not PID) | Derivative term adds noise with stepper vibration | Prior |
| Claude Code template | Structured project management and session continuity | 2026-02-05 |
| Dual wiring docs (text + visual) | Text for quick reference, .pen for visual assembly guide | 2026-02-05 |
| Pin 1 = top (antenna end) | Matches DOIT schematic and mischianti.org pinout convention | 2026-02-05 |
| Motor limits at stepMotor level | Guards all call paths (manual, correction, continuous) | 2026-02-06 |
| ±2048 default limits (1 rev) | Prevents unscrewing legs; configurable in config.h | 2026-02-06 |
| IMU inversion at sensor level | All consumers get correct angles; flags in config.h | 2026-02-06 |
| Dashboard as first GUI tab | Most-used diagnostic view should be immediately visible | 2026-02-06 |
| Complementary alpha 0.02→0.15 | Old value too sluggish (15+s convergence); 0.15 gives 95% in ~4s | 2026-02-06 |

| PI error sign = +actual (not -actual) | Plant has negative gain (positive steps decrease angles); error=actual gives net negative feedback | 2026-02-06 |
| PI gains: Kp_pitch=1.0, Kp_roll=0.5 | Pitch ~2x less effective per step than roll; stepsPerDegree=60 | 2026-02-06 |
| INVERT_PITCH/ROLL both false | Hardware test confirmed IMU reads correct signs; fix was in PI error sign | 2026-02-06 |
| Motor 2 pin order: 16,13,15,4 | Original order (16,4,15,13) caused humming; coils needed reordering | 2026-02-19 |
| Direct serial writes in GUI | Worker thread queue caused intermittent Motor 2 failures; direct writes work reliably | 2026-02-19 |
| Motor 2 GUI direction reversed | Lead screw orientation means positive firmware steps = retract; GUI flips for intuitive +/- | 2026-02-19 |

## Known Issues & Bugs

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|

## Session Handoff Notes

**Last Session:** 2026-02-19

**What We Were Working On:**
Motor 2 wiring fix and motor limits GUI improvements

**Where We Left Off:**
Both motors working correctly through the GUI. Ready to find physical motor limits.

1. **Motor 2 coil fix** — Original pin order (16,4,15,13) caused humming. Correct order is (16,13,15,4) — confirmed via coiltest command and serial monitor testing.
2. **GUI serial rewrite** — Worker thread queue caused intermittent Motor 2 failures. Replaced with direct serial writes + threading lock. All motor commands now reliable.
3. **Motor 2 direction** — Lead screw is physically reversed vs Motor 1. GUI flips direction so +/- is intuitive (+ = extend out, - = retract in for both motors).
4. **Both-motors control** — Added to GUI for simultaneous movement when finding limits.
5. **Screw was blocking** — User found a screw blocking Motor 2's lead screw rotation; removed it.

**What Needs to Happen Next:**
- Use motor limits GUI to find IN (min) and OUT (max) positions for both motors
- Update MOTOR_MIN_POSITION and MOTOR_MAX_POSITION in config.h with actual values
- Full closed-loop leveling test
- Note: Motor 2 direction reversal is only in the GUI — firmware and leveling controller may need a direction flag too

**Important Context:**
- PlatformIO ESP32 project. Build with `pio run`, upload with `pio run -t upload`.
- Motor limits GUI: `python tools/motor_limits_gui.py`
- Current config.h limits: MIN=0, MAX=70000 (placeholder, needs real values from testing)

---

## Commands Reference

### Development
```bash
# Build firmware
pio run

# Upload to ESP32
pio run -t upload

# Serial monitor (115200 baud)
pio device monitor

# Build + Upload + Monitor
pio run -t upload && pio device monitor
```

### Serial CLI Commands (at runtime)
```
h         - Help menu
s         - System status
i         - IMU data
m1/m2 <N> - Move motor by N steps
c         - IMU calibration (IDLE only)
r         - Reset to IDLE
p <kp> <ki> - Set PI gains
t <deg>   - Set level tolerance
l         - Toggle continuous logging
admin     - Enter test mode
```

### Test Mode Commands (additional)
```
mpos      - Query motor positions and limits
mreset    - Reset motor position counters to zero
```

---

## Roadmap System

Feature planning lives in the `/roadmap` folder:
- `backlog/` - Ideas and requests not yet planned
- `planned/` - Detailed plans ready to build
- `in-progress/` - Currently being worked on (1-2 max)
- `completed/` - Finished features for reference

### When I Request a New Feature

1. Create a new .md file in `roadmap/backlog/` using the template
2. Ask clarifying questions about requirements
3. When I say "plan this feature", move to `planned/` and fill in technical approach
4. Show me the plan and wait for approval
5. Move to `in-progress/` only when I say to start building
6. Move to `completed/` when done

### Organizing Work Priority

Before starting a new task, consider:
1. **Dependencies** - What must be built first?
2. **Continuity** - What shares code with recent work?
3. **Quick wins** - Any small tasks to knock out?
4. **Blockers** - What's blocking other features?

---

## Instructions for Claude

### After EVERY Task (Do This Automatically)

1. Update "Current State" with what's working now
2. Move completed items from "Next Steps" to "Completed Features"
3. Add any new decisions to "Decisions Made"
4. Note any bugs discovered in "Known Issues"
5. Update file structure section if new folders/files were created

### When I Request a New Feature

1. Add it to "Next Steps" before starting work
2. Break complex features into smaller subtasks
3. Update "Current State" as you make progress

### Before Context Gets Full or Session Ends

1. Write detailed "Session Handoff Notes"
2. Ensure "Current State" is accurate
3. List specific next actions in "Next Steps"

### When Starting a New Session

1. Read this entire file first
2. Check "Session Handoff Notes" for context
3. Confirm "Current State" matches actual project
4. Ask me if anything is unclear before proceeding

### Code Changes

1. Build with `pio run` after changes to verify compilation
2. Update this file to reflect changes
3. Commit with descriptive messages

### If You Get Confused

1. Stop and re-read this file
2. Check git history for recent changes
3. Ask me for clarification
4. Don't guess - verify by reading actual files

---

## Troubleshooting Log

---

## Notes

- Motor step conversion: 2048 steps = 1 full revolution (28BYJ-48)
- MPU6050 I2C address: 0x68
- Platform has 3 legs: 1 fixed front, 2 adjustable rear (one per motor)
