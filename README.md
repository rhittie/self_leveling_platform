# Self-Leveling Platform

ESP32-based firmware for a three-legged self-leveling platform using an MPU6050 accelerometer/gyroscope and two 28BYJ-48 stepper motors.

## Overview

This project creates an automatic leveling platform with three legs - one fixed front leg and two motorized back legs. The platform continuously monitors its orientation using an IMU and adjusts the back legs to maintain a level surface.

## Hardware Requirements

| Component | Quantity | Description |
|-----------|----------|-------------|
| ESP32 Dev Board | 1 | Main microcontroller |
| MPU6050 | 1 | 6-axis accelerometer/gyroscope |
| 28BYJ-48 Stepper Motor | 2 | With ULN2003 driver boards |
| Push Button | 1 | For user control (with built-in RGB LED) |
| RGB LED | 1 | Built into push button (common anode) |

## Wiring

### I2C (MPU6050)
| MPU6050 | ESP32 |
|---------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Motor 1 (Left Back Leg)
| ULN2003 | ESP32 |
|---------|-------|
| IN1 | GPIO 19 |
| IN2 | GPIO 18 |
| IN3 | GPIO 5 |
| IN4 | GPIO 17 |

### Motor 2 (Right Back Leg)
| ULN2003 | ESP32 |
|---------|-------|
| IN1 | GPIO 16 |
| IN2 | GPIO 13 |
| IN3 | GPIO 15 |
| IN4 | GPIO 4 |

### User Interface
| Component | ESP32 |
|-----------|-------|
| Button | GPIO 32 (with pull-up) |
| RGB LED Red | GPIO 25 |
| RGB LED Green | GPIO 26 |
| RGB LED Blue | GPIO 27 |

## Software Setup

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Python 3.x (for GUI tool)

### Build & Upload
```bash
# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Usage

### Normal Operation

1. **Power on** - Platform starts in IDLE state (LED off)
2. **Short press button** - Starts leveling sequence
   - LED blinks slowly during initialization
   - LED solid yellow while waiting for stability
   - LED blinks fast during active leveling
   - LED double-pulses green when level achieved
3. **Long press button (2+ sec)** - Saves motor positions and enters safe shutdown mode

### State Machine

```
IDLE → [Button Press] → INITIALIZING → WAIT_FOR_STABLE → LEVELING ↔ LEVEL_OK
                                                              ↓
                                                           ERROR
[Long Press from any state] → SAFE_SHUTDOWN (saves positions, safe to power off)
```

### Serial Commands (Normal Mode)

| Command | Description |
|---------|-------------|
| `h` | Show help |
| `s` | Print current status |
| `i` | Print IMU data |
| `m1 <N>` | Move motor 1 by N steps |
| `m2 <N>` | Move motor 2 by N steps |
| `c` | Run IMU calibration (IDLE only) |
| `r` | Reset to IDLE state |
| `p <kp> <ki>` | Set PI controller gains |
| `t <deg>` | Set level tolerance |
| `st <sec>` | Set stability timeout (0.5-30 sec) |
| `l` | Toggle continuous logging |
| `level` | Start leveling (same as button press) |
| `admin` / `test` | Enter admin test mode |

### Runtime-Configurable Settings

These settings can be changed at runtime via serial commands. They reset to defaults on reboot.

| Setting | Command | Default | Range | Description |
|---------|---------|---------|-------|-------------|
| PI Gains | `p <kp> <ki>` | Kp=1.0, Ki=0.05 | — | PI controller proportional and integral gains |
| Level Tolerance | `t <deg>` | 0.5° | 0-10° | Max acceptable angle deviation from level |
| Stability Timeout | `st <sec>` | 3.0 sec | 0.5-30 sec | How long platform must be still before leveling starts |
| Continuous Logging | `l` | OFF | ON/OFF | Toggle 10 Hz pitch/roll/motor position logging |

## Admin Test Mode

A dedicated test mode for testing individual hardware components.

### Enter Test Mode
```
admin    # or 'test'
```

### Test Mode Commands

#### Motors
| Command | Description |
|---------|-------------|
| `m1 <steps>` | Move motor 1 by N steps |
| `m2 <steps>` | Move motor 2 by N steps |
| `m1c` | Toggle motor 1 continuous rotation |
| `m2c` | Toggle motor 2 continuous rotation |
| `mstop` | Stop all motors |
| `mspeed <rpm>` | Set motor speed (1-15 RPM) |
| `mpos` | Query motor positions and limits |
| `mreset` | Reset both motor positions to zero |
| `mreset1` | Reset motor 1 position to zero |
| `mreset2` | Reset motor 2 position to zero |
| `mset <pos>` | Set both motor position counters to a value |
| `munlock` | Remove position safety limits (for finding physical extents) |
| `mlock` | Restore default position safety limits |
| `coiltest` | Energize each Motor 2 coil individually to verify wiring |

#### IMU / MPU6050
| Command | Description |
|---------|-------------|
| `scan` | I2C bus scan (find MPU6050 at 0x68) |
| `imu` | Initialize IMU and show WHO_AM_I |
| `read` | Single IMU reading |
| `stream` | Toggle continuous streaming (10 Hz) |
| `cal` | Run calibration routine |
| `raw` | Show raw sensor values |

#### Button
| Command | Description |
|---------|-------------|
| `btn` | Toggle button test mode (shows press events) |

#### LED
| Command | Description |
|---------|-------------|
| `led on/off` | LED solid on / off |
| `led slow` | Slow blink (1 Hz) |
| `led fast` | Fast blink (4 Hz) |
| `led pulse` | Double pulse pattern |
| `led error` | Error blink (10 Hz) |
| `led cycle` | Cycle through all patterns |
| `led red/green/blue` | Set LED color |
| `led yellow/cyan/purple/white` | Set LED color |
| `ledtest` | Raw GPIO LED test (bypasses PWM, tests common anode) |

#### System
| Command | Description |
|---------|-------------|
| `info` / `pins` | Show pin assignments and config |
| `exit` | Return to normal IDLE mode |

## Configuration

### Compile-Time Settings (`include/config.h`)

These are defaults that can be overridden at runtime via serial commands.

| Parameter | Default | Serial Command | Description |
|-----------|---------|----------------|-------------|
| `LEVEL_TOLERANCE_DEG` | 0.5° | `t <deg>` | Acceptable angle deviation |
| `STABILITY_TIMEOUT_MS` | 3000 ms | `st <sec>` | Wait time before leveling starts |
| `DEFAULT_KP_PITCH` | 1.0 | `p <kp> <ki>` | PI proportional gain (pitch) |
| `DEFAULT_KI_PITCH` | 0.05 | `p <kp> <ki>` | PI integral gain (pitch) |
| `DEFAULT_KP_ROLL` | 0.5 | `p <kp> <ki>` | PI proportional gain (roll) |
| `DEFAULT_KI_ROLL` | 0.03 | `p <kp> <ki>` | PI integral gain (roll) |

### Hardware Constants (`include/config.h`)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `MOTOR_SPEED_RPM` | 10 | Default motor speed |
| `MOTOR_MIN_POSITION` | 0 | Minimum motor position (steps) |
| `MOTOR_MAX_POSITION` | 70000 | Maximum motor position (steps) |
| `COMPLEMENTARY_ALPHA` | 0.15 | IMU filter coefficient (higher = faster response) |
| `MOTION_ACCEL_THRESHOLD` | 0.15 g | Motion detection sensitivity (accelerometer) |
| `MOTION_GYRO_THRESHOLD` | 10 °/s | Motion detection sensitivity (gyroscope) |
| `MAX_CORRECTION_STEPS` | 50 | Max motor steps per leveling correction cycle |
| `INTEGRAL_LIMIT` | 100.0 | PI integral windup limit |

## GUI Test Tool

A Python GUI for easier hardware testing.

### Setup
```bash
cd tools
pip install pyserial
```

### Run
```bash
python test_mode_gui.py
# Or double-click run_gui.bat on Windows
```

### Features
- Serial port selection and connection
- Dashboard tab with bubble level and motor position bars
- Organized tabs for Motors, IMU, LED, and Button testing
- Real-time serial output display
- Auto-enters test mode and queries motor positions on connect

## Project Structure

```
self_leveling_platform/
├── include/
│   ├── config.h              # Pin definitions and constants
│   └── types.h               # Data structures and enums
├── lib/
│   ├── ButtonHandler/        # Button debouncing and events
│   ├── LevelingController/   # PI control algorithm
│   ├── MPU6050Handler/       # IMU communication and filtering
│   ├── StatusLED/            # RGB LED pattern management
│   └── StepperController/    # Dual motor control with position limits
├── src/
│   └── main.cpp              # Main application and state machine
├── tools/
│   ├── test_mode_gui.py      # Python GUI for testing
│   ├── motor_limits_gui.py   # GUI for finding motor travel limits
│   ├── run_gui.bat           # Windows launcher
│   └── requirements.txt      # Python dependencies
├── roadmap/                  # Feature planning and tracking
├── WIRING_DIAGRAM.md         # Detailed text-based wiring reference
└── platformio.ini            # PlatformIO configuration
```

## License

MIT License
