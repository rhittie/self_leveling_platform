# Feature: Wiring Documentation

## Metadata
- **Priority:** High
- **Complexity:** Medium
- **Estimated Sessions:** 2
- **Dependencies:** 000-core-firmware

## Description
Created comprehensive wiring documentation for the self-leveling prism hardware. Includes both a detailed text-based reference (WIRING_DIAGRAM.md) and a visual diagram (wiring_diagram.pen) showing all connections between the ESP32, MPU6050 IMU, two ULN2003 stepper drivers, push button, and DC power converter.

## Requirements
- [x] Text-based wiring reference with physical pin positions
- [x] ASCII art board layout showing all 30 ESP32 pins
- [x] Per-component wiring tables (MPU6050, Motor 1, Motor 2, Button, Power)
- [x] Visual board map showing which pins are used
- [x] Platform leg layout diagram
- [x] Wiring tips and gotchas
- [x] Visual .pen diagram with color-coded components and wire connections
- [x] Verified against official DOIT ESP32 schematic

## Technical Approach
1. Extracted all GPIO assignments from `include/config.h`
2. Mapped GPIO numbers to physical board pin positions using the ESP32 Dev Kit V1 (DoIT) pinout
3. Created ASCII diagrams for each component connection
4. Built a visual `.pen` diagram using Pencil MCP tools with:
   - Green ESP32 board with all 30 labeled pins
   - Color-coded component cards (blue=IMU, green=Motor1, orange=Motor2, purple=Button, red=Power)
   - Wire connection indicators between components
   - Platform leg layout
   - Color legend and wiring tips
5. Cross-referenced against official DOIT schematic PDF to verify pin positions

## Files Created
- [x] `WIRING_DIAGRAM.md` — Text-based wiring reference (311 lines)
- [x] `wiring_diagram.pen` — Visual wiring diagram

## Corrected Pin Mapping (ESP32 Dev Kit V1, 30-pin)

Left side (top to bottom, antenna end = pin 1):
```
L1=EN, L2=VP(36), L3=VN(39), L4=D34, L5=D35, L6=D32, L7=D33,
L8=D25, L9=D26, L10=D27, L11=D14, L12=D12, L13=D13, L14=GND, L15=VIN
```

Right side (top to bottom):
```
R1=D23, R2=D22, R3=TX0, R4=RX0, R5=D21, R6=D19, R7=D18,
R8=D5, R9=D17, R10=D16, R11=D4, R12=D2, R13=D15, R14=GND, R15=3V3
```

## Key Connections
| Component | GPIO | Physical Pin |
|-----------|------|-------------|
| MPU6050 SDA | GPIO 21 | Right 5 |
| MPU6050 SCL | GPIO 22 | Right 2 |
| Motor 1 IN1-IN4 | GPIO 19,18,5,17 | Right 6-9 |
| Motor 2 IN1-IN4 | GPIO 16,4,13,15 | Right 10-11, **Left 13**, Right 13 |
| Push Button | GPIO 33 | Left 7 |
| Onboard LED | GPIO 2 | Right 12 |

## Status
- **Completed:** 2026-02-05

## Session Log
### 2026-02-05
- Created initial WIRING_DIAGRAM.md with GPIO-based connections
- Added physical pin positions (first attempt had incorrect mapping)
- User provided actual ESP32 pinout image — corrected all physical pin positions
- Created visual wiring_diagram.pen with Pencil MCP tools
- User provided official DOIT schematic PDF — verified all pins match
- Updated .pen diagram with all corrected pin positions
- Final verification via screenshots confirmed all components correct

## Notes
- **Critical gotcha:** Motor 2 IN3 (GPIO 13) is on the LEFT side of the board (Left Pin 13), while all other Motor 2 signals are on the right side
- GPIO 5 and GPIO 15 have boot-mode functions — work fine as outputs after boot but may cause issues if held HIGH/LOW during reset
- MPU6050 runs on 3.3V — must connect to 3V3 pin, NOT VIN (5V)
- I2C wires should be kept under 30cm for reliable communication
- Motor power should come from DC converter, not ESP32 USB (prevents brownouts)
