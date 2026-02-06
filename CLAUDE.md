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

## Next Steps

1. [ ] Add serial command to start leveling (currently requires button press)
2. [ ] Full closed-loop leveling test with button press
3. [ ] Set up Notion sync (optional - requires API key)

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

## Known Issues & Bugs

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|

## Session Handoff Notes

**Last Session:** 2026-02-06

**What We Were Working On:**
Motor safety limits, IMU orientation fix, and Dashboard GUI

**Where We Left Off:**
Fixed PI controller and tuned gains. Full hardware testing done.

1. **PI error sign fix** — Original code had `error = -actual` which caused positive feedback. Fixed to `error = actual` to account for negative plant gain (positive motor steps decrease angles).
2. **PI gains tuned** — Kp_pitch=1.0, Ki_pitch=0.05, Kp_roll=0.5, Ki_roll=0.03, stepsPerDegree=60
3. **INVERT flags** — Both false. IMU reads correct signs; the bug was in the controller.
4. **Verified** — 20-cycle correction test with M1+2000 initial tilt showed roll correctly converging (0.787 to 0.460 over cycles). Motor positions moving in expected directions.
5. **Motor mapping** — M1: dP=-0.22, dR=+0.42 | M2: dP=-0.20, dR=-0.45 per 1000 steps

**What Needs to Happen Next:**
- Full closed-loop test (requires button press to enter LEVELING state)
- Consider adding serial command to start leveling for easier testing
- Fine-tune gains based on closed-loop behavior

**Important Context:**
This is a PlatformIO ESP32 project. Build with `pio run`, upload with `pio run -t upload`.

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
