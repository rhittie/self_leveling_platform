# Self-Leveling Prism - Wiring Diagram

## Components

| # | Component | Description |
|---|-----------|-------------|
| 1 | ESP32 Dev Kit V1 (DoIT) | Main microcontroller |
| 2 | MPU6050 IMU Module | 6-axis accelerometer + gyroscope |
| 3 | 28BYJ-48 Stepper Motor 1 + ULN2003 Driver | Left back leg |
| 4 | 28BYJ-48 Stepper Motor 2 + ULN2003 Driver | Right back leg |
| 5 | Momentary Push Button | User input |
| 6 | DC Power Converter (DWEII) | 5V power supply |

---

## ESP32 Dev Kit V1 (30-Pin) Physical Board Layout

Orientation: antenna/chip at top, USB connector at bottom, looking down at the top of the board.

```
                          +-----------+
                          | [ANTENNA] |
               +----------+-----------+----------+
               |                                  |
   Left Pin 1  | [ EN   ]            [ D23  ]  | Right Pin 1
   Left Pin 2  | [ VP/36 ]           [ D22  ]  | Right Pin 2
   Left Pin 3  | [ VN/39 ]           [ TX0  ]  | Right Pin 3
   Left Pin 4  | [ D34  ]            [ RX0  ]  | Right Pin 4
   Left Pin 5  | [ D35  ]            [ D21  ]  | Right Pin 5
   Left Pin 6  | [ D32  ]            [ D19  ]  | Right Pin 6
   Left Pin 7  | [ D33  ]            [ D18  ]  | Right Pin 7
   Left Pin 8  | [ D25  ]            [ D5   ]  | Right Pin 8
   Left Pin 9  | [ D26  ]            [ D17  ]  | Right Pin 9
   Left Pin 10 | [ D27  ]            [ D16  ]  | Right Pin 10
   Left Pin 11 | [ D14  ]            [ D4   ]  | Right Pin 11
   Left Pin 12 | [ D12  ]            [ D2   ]  | Right Pin 12
   Left Pin 13 | [ D13  ]            [ D15  ]  | Right Pin 13
   Left Pin 14 | [ GND  ]            [ GND  ]  | Right Pin 14
   Left Pin 15 | [ VIN  ]            [ 3V3  ]  | Right Pin 15
               |                                  |
               +----------+-----------+----------+
                          | [  USB  ] |
                          +-----------+
```

> **Reference:** Based on the ESP32 DEV KIT V1 pinout from mischianti.org.
> The "D" prefix on pin labels = GPIO number (D21 = GPIO 21).

---

## Physical Pin Reference Table

| Board Position | Silkscreen | GPIO # | Board Position | Silkscreen | GPIO # |
|----------------|-----------|--------|----------------|-----------|--------|
| Left 1 | EN | Reset | Right 1 | D23 | GPIO 23 |
| Left 2 | VP | GPIO 36 | Right 2 | D22 | GPIO 22 |
| Left 3 | VN | GPIO 39 | Right 3 | TX0 | GPIO 1 |
| Left 4 | D34 | GPIO 34 | Right 4 | RX0 | GPIO 3 |
| Left 5 | D35 | GPIO 35 | Right 5 | D21 | GPIO 21 |
| Left 6 | D32 | GPIO 32 | Right 6 | D19 | GPIO 19 |
| Left 7 | D33 | GPIO 33 | Right 7 | D18 | GPIO 18 |
| Left 8 | D25 | GPIO 25 | Right 8 | D5 | GPIO 5 |
| Left 9 | D26 | GPIO 26 | Right 9 | D17 | GPIO 17 |
| Left 10 | D27 | GPIO 27 | Right 10 | D16 | GPIO 16 |
| Left 11 | D14 | GPIO 14 | Right 11 | D4 | GPIO 4 |
| Left 12 | D12 | GPIO 12 | Right 12 | D2 | GPIO 2 |
| Left 13 | D13 | GPIO 13 | Right 13 | D15 | GPIO 15 |
| Left 14 | GND | Ground | Right 14 | GND | Ground |
| Left 15 | VIN | Power (5V) | Right 15 | 3V3 | Power (3.3V) |

---

## Wiring Connections (with Physical Pin Positions)

### 1. MPU6050 IMU --> ESP32

```
  MPU6050 Module          ESP32 Dev Kit V1
  +-----------+           +--------------------+
  |           |           |                    |
  |  VCC  o---+---------->| 3V3    (Right 15)  |
  |  GND  o---+---------->| GND    (Left 14)   |
  |  SDA  o---+---------->| D21    (Right 5)   |
  |  SCL  o---+---------->| D22    (Right 2)   |
  |  AD0  o---+---------->| GND    (Left 14)   |  (sets I2C address to 0x68)
  |  INT  o   |           |                    |  (not connected)
  |           |           |                    |
  +-----------+           +--------------------+
```

| MPU6050 Pin | ESP32 Physical Pin | Board Label | GPIO |
|-------------|--------------------|-------------|------|
| VCC | Right Pin 15 | 3V3 | 3.3V power |
| GND | Left Pin 14 | GND | Ground |
| SDA | Right Pin 5 | D21 | GPIO 21 |
| SCL | Right Pin 2 | D22 | GPIO 22 |
| AD0 | Left Pin 14 | GND | Ground (addr 0x68) |

> **Note:** The MPU6050 runs on 3.3V - do NOT connect VCC to the 5V/VIN pin.

---

### 2. Stepper Motor 1 (Left Back Leg) - ULN2003 Driver --> ESP32

```
  ULN2003 Driver Board         ESP32 Dev Kit V1
  +-------------------+        +--------------------+
  |                   |        |                    |
  |  IN1  o-----------+------->| D19    (Right 6)   |
  |  IN2  o-----------+------->| D18    (Right 7)   |
  |  IN3  o-----------+------->| D5     (Right 8)   |
  |  IN4  o-----------+------->| D17    (Right 9)   |
  |                   |        |                    |
  |  VCC  o-----------+--> 5V from DC Converter     |
  |  GND  o-----------+--> GND (common ground)      |
  |                   |        |                    |
  |  [Motor Plug]     |        +--------------------+
  |   to 28BYJ-48 #1  |
  +-------------------+
```

| ULN2003 #1 Pin | ESP32 Physical Pin | Board Label | GPIO |
|----------------|--------------------|-------------|------|
| IN1 | Right Pin 6 | D19 | GPIO 19 |
| IN2 | Right Pin 7 | D18 | GPIO 18 |
| IN3 | Right Pin 8 | D5 | GPIO 5 |
| IN4 | Right Pin 9 | D17 | GPIO 17 |
| VCC | DC Converter 5V | -- | -- |
| GND | Common ground | -- | -- |

> All four Motor 1 signal pins are consecutive on the **right side** of the ESP32
> (pins 6-9). The 28BYJ-48 motor plugs into the white connector on the ULN2003.

---

### 3. Stepper Motor 2 (Right Back Leg) - ULN2003 Driver --> ESP32

```
  ULN2003 Driver Board         ESP32 Dev Kit V1
  +-------------------+        +--------------------+
  |                   |        |                    |
  |  IN1  o-----------+------->| D16    (Right 10)  |
  |  IN2  o-----------+------->| D4     (Right 11)  |
  |  IN3  o-----------+------->| D13    (Left 13)   |
  |  IN4  o-----------+------->| D15    (Right 13)  |
  |                   |        |                    |
  |  VCC  o-----------+--> 5V from DC Converter     |
  |  GND  o-----------+--> GND (common ground)      |
  |                   |        |                    |
  |  [Motor Plug]     |        +--------------------+
  |   to 28BYJ-48 #2  |
  +-------------------+
```

| ULN2003 #2 Pin | ESP32 Physical Pin | Board Label | GPIO |
|----------------|--------------------|-------------|------|
| IN1 | Right Pin 10 | D16 | GPIO 16 |
| IN2 | Right Pin 11 | D4 | GPIO 4 |
| IN3 | **Left Pin 13** | D13 | GPIO 13 |
| IN4 | Right Pin 13 | D15 | GPIO 15 |
| VCC | DC Converter 5V | -- | -- |
| GND | Common ground | -- | -- |

> **Watch out:** Motor 2 IN3 (GPIO 13) is on the **LEFT side** of the board (Left Pin 13),
> while the other three Motor 2 pins are on the right side. Don't miss this one!

---

### 4. Push Button --> ESP32

```
  Button                  ESP32 Dev Kit V1
  +------+                +--------------------+
  |      |                |                    |
  | Leg1 o--------------->| D33    (Left 7)    |
  | Leg2 o--------------->| GND    (Left 14)   |
  |      |                |                    |
  +------+                +--------------------+
```

| Button Pin | ESP32 Physical Pin | Board Label | GPIO |
|------------|--------------------|-------------|------|
| Leg 1 | Left Pin 7 | D33 | GPIO 33 |
| Leg 2 | Left Pin 14 | GND | Ground |

> No external resistor needed - uses the ESP32's internal pull-up.
> Button pressed = LOW, released = HIGH.

---

### 5. Onboard LED (built into ESP32)

No external wiring. The onboard LED is hard-wired to GPIO 2 (Right Pin 12 / D2).

---

### 6. Power Distribution

```
                DC Power Converter (DWEII)
                +---------------------+
                |                     |
  Wall Power -->|  IN+    OUT+ (5V) o-+--+--> ESP32 VIN  (Left Pin 15)
                |  IN-    OUT- (GND)o-+--+--> ESP32 GND  (Left Pin 14)
                |                     |  |
                +---------------------+  +--> ULN2003 #1 VCC
                                         +--> ULN2003 #2 VCC
                                         |
                                   Common GND bus:
                                         +--> ULN2003 #1 GND
                                         +--> ULN2003 #2 GND
                                         +--> MPU6050 GND
                                         +--> Button Leg2
```

| Power Connection | ESP32 Physical Pin | Board Label |
|------------------|--------------------|-------------|
| 5V input | Left Pin 15 | VIN |
| Ground | Left Pin 14 | GND |
| Ground (alt) | Right Pin 14 | GND |
| 3.3V out (to MPU6050) | Right Pin 15 | 3V3 |

---

## Complete Pin Summary (GPIO + Physical Position)

| GPIO | Board Side | Pin # | Board Label | Connected To | Function |
|------|-----------|-------|-------------|-------------|----------|
| 21 | Right | 5 | D21 | MPU6050 SDA | I2C Data |
| 22 | Right | 2 | D22 | MPU6050 SCL | I2C Clock |
| 19 | Right | 6 | D19 | ULN2003 #1 IN1 | Motor 1 coil A |
| 18 | Right | 7 | D18 | ULN2003 #1 IN2 | Motor 1 coil B |
| 5 | Right | 8 | D5 | ULN2003 #1 IN3 | Motor 1 coil C |
| 17 | Right | 9 | D17 | ULN2003 #1 IN4 | Motor 1 coil D |
| 16 | Right | 10 | D16 | ULN2003 #2 IN1 | Motor 2 coil A |
| 4 | Right | 11 | D4 | ULN2003 #2 IN2 | Motor 2 coil B |
| 13 | **Left** | **13** | D13 | ULN2003 #2 IN3 | Motor 2 coil C |
| 15 | Right | 13 | D15 | ULN2003 #2 IN4 | Motor 2 coil D |
| 33 | Left | 7 | D33 | Push Button | User input |
| 2 | Right | 12 | D2 | Onboard LED | Status indicator |
| -- | Right | 15 | 3V3 | MPU6050 VCC | 3.3V sensor power |
| -- | Left | 15 | VIN | DC Converter 5V | Board power input |
| -- | Left | 14 | GND | Common ground | Ground bus |
| -- | Right | 14 | GND | Common ground | Ground bus |

---

## Visual Board Map - Which Pins Are Used

Pins marked with `[*]` are used in this project. Unused pins are `[ ]`.

```
                          +-----------+
                          | [ANTENNA] |
               +----------+-----------+----------+
               |                                  |
   Left 1  [ ] | [ EN   ]            [ D23  ] | [ ]  Right 1
   Left 2  [ ] | [ VP/36 ]           [ D22  ] | [*]  Right 2      MPU6050 SCL
   Left 3  [ ] | [ VN/39 ]           [ TX0  ] | [ ]  Right 3
   Left 4  [ ] | [ D34  ]            [ RX0  ] | [ ]  Right 4
   Left 5  [ ] | [ D35  ]            [ D21  ] | [*]  Right 5      MPU6050 SDA
   Left 6  [ ] | [ D32  ]            [ D19  ] | [*]  Right 6      Motor1 IN1
   Left 7  [*] | [ D33  ]            [ D18  ] | [*]  Right 7      Motor1 IN2
   Left 8  [ ] | [ D25  ]            [ D5   ] | [*]  Right 8      Button
   Left 9  [ ] | [ D26  ]            [ D17  ] | [*]  Right 9      Motor1 IN3
   Left 10 [ ] | [ D27  ]            [ D16  ] | [*]  Right 10     Motor1 IN4
   Left 11 [ ] | [ D14  ]            [ D4   ] | [*]  Right 11     Motor2 IN1
   Left 12 [ ] | [ D12  ]            [ D2   ] | [*]  Right 12     Motor2 IN2
   Left 13 [*] | [ D13  ]            [ D15  ] | [*]  Right 13     LED, Motor2 IN3
   Left 14 [*] | [ GND  ]            [ GND  ] | [*]  Right 14     Motor2 IN4
   Left 15 [*] | [ VIN  ]            [ 3V3  ] | [*]  Right 15     GND bus x2
               |                                  |                VIN + 3V3
               +----------+-----------+----------+
                          | [  USB  ] |
                          +-----------+
```

---

## Platform Leg Layout

```
              FRONT
                *  <-- Fixed leg (no motor, passive)
               / \
              /   \
             /     \
            /       \
           *---------*
     LEFT BACK     RIGHT BACK
     (Motor 1)     (Motor 2)
     Right 6-9     Right 10-11,
                   Left 13,
                   Right 13
```

- **Pitch correction:** Both motors move in the same direction
- **Roll correction:** Motors move in opposite directions

---

## Tips

1. **Motor 2 IN3 crosses sides** - GPIO 13 is on the left side of the board (Left Pin 13) while the rest of Motor 2 is on the right side. Route this wire carefully.
2. **Motor 1 is easy** - all four signal wires go to consecutive right-side pins 6, 7, 8, 9.
3. **Power pins are at the bottom** - VIN is at the bottom-left (Left 15) and 3V3 is at the bottom-right (Right 15). GND is on both sides at position 14.
4. **Double-check GPIO 5 and GPIO 15** - these pins have boot-mode functions on the ESP32. They work fine as outputs after boot, but may cause issues if held HIGH/LOW during reset.
5. **Keep I2C wires short** (under 30cm) for reliable MPU6050 communication.
6. **Use the DC converter for motor power** - drawing motor current through the ESP32's USB can cause brownouts and resets.
7. **Common ground is critical** - every component must share the same GND reference.
