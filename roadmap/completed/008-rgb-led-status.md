# Feature: RGB LED Status Indicator

## Metadata
- **Priority:** Medium
- **Complexity:** Low
- **Estimated Sessions:** 1
- **Dependencies:** 000-core-firmware

## Description
Added RGB LED support for the push button, replacing the single onboard LED. Each system state has a distinct color and blink pattern for clear visual feedback.

## Requirements
- [x] RGB LED control via ESP32 LEDC PWM
- [x] Color per state (blue=init, yellow=waiting, cyan=leveling, green=level OK, red=error, purple=test)
- [x] Blink patterns (solid, slow, fast, double pulse, error)
- [x] LED color commands in test mode

## Files Modified
- `include/config.h` — PIN_LED_RED/GREEN/BLUE, LEDC channel defines
- `lib/StatusLED/StatusLED.h` — RGB LED class
- `lib/StatusLED/StatusLED.cpp` — LED patterns and color control
- `src/main.cpp` — LED color commands in test mode

## Status
- **Completed:** 2026-02-06
