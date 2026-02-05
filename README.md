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
| Push Button | 1 | For user control |
| LED | 1 | Status indicator (or use onboard LED) |

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
| IN2 | GPIO 4 |
| IN3 | GPIO 13 |
| IN4 | GPIO 15 |

### User Interface
| Component | ESP32 |
|-----------|-------|
| Button | GPIO 33 (with pull-up) |
| LED | GPIO 2 (onboard) |

## Software Setup

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Python 3.x (for GUI tool)

### Build & Upload
```bash
# Clone the repository
git clone https://github.com/rhittie/self_leveling_platform.git
cd self_leveling_platform

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
   - LED solid while waiting for stability
   - LED blinks fast during active leveling
   - LED double-pulses when level achieved
3. **Long press button (2+ sec)** - Returns to IDLE state

### State Machine

```
IDLE → [Button Press] → INITIALIZING → WAIT_FOR_STABLE → LEVELING ↔ LEVEL_OK
                                                              ↓
                                                           ERROR
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
| `l` | Toggle continuous logging |

## Admin Test Mode

A dedicated test mode for testing individual hardware components before full assembly.

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
| `led on` | LED solid on |
| `led off` | LED off |
| `led slow` | Slow blink (1 Hz) |
| `led fast` | Fast blink (4 Hz) |
| `led pulse` | Double pulse pattern |
| `led error` | Error blink (10 Hz) |
| `led cycle` | Cycle through all patterns |

#### System
| Command | Description |
|---------|-------------|
| `info` | Show pin assignments and config |
| `exit` | Return to normal IDLE mode |

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
- Organized tabs for Motors, IMU, LED, and Button testing
- Real-time serial output display
- Toggle buttons for continuous operations

## Configuration

Key parameters in `include/config.h`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `LEVEL_TOLERANCE_DEG` | 0.5° | Acceptable angle deviation |
| `STABILITY_TIMEOUT_MS` | 3000 | Wait time before leveling |
| `DEFAULT_KP_PITCH` | 2.0 | PI proportional gain |
| `DEFAULT_KI_PITCH` | 0.1 | PI integral gain |
| `MOTOR_SPEED_RPM` | 10 | Default motor speed |

## Project Structure

```
self_leveling_platform/
├── include/
│   ├── config.h          # Pin definitions and constants
│   └── types.h           # Data structures and enums
├── lib/
│   ├── ButtonHandler/    # Button debouncing and events
│   ├── LevelingController/ # PI control algorithm
│   ├── MPU6050Handler/   # IMU communication and filtering
│   ├── StatusLED/        # LED pattern management
│   └── StepperController/ # Motor control
├── src/
│   └── main.cpp          # Main application and state machine
├── tools/
│   ├── test_mode_gui.py  # Python GUI for testing
│   ├── run_gui.bat       # Windows launcher
│   └── requirements.txt  # Python dependencies
└── platformio.ini        # PlatformIO configuration
```

## License

MIT License
