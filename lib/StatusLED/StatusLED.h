#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

// RGB color (0-255 per channel)
struct RGBColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// Predefined colors for each state
namespace LEDColors {
    constexpr RGBColor OFF       = {0,   0,   0};
    constexpr RGBColor RED       = {255, 0,   0};
    constexpr RGBColor GREEN     = {0,   255, 0};
    constexpr RGBColor BLUE      = {0,   0,   255};
    constexpr RGBColor YELLOW    = {255, 180, 0};
    constexpr RGBColor CYAN      = {0,   200, 255};
    constexpr RGBColor PURPLE    = {180, 0,   255};
    constexpr RGBColor WHITE     = {255, 255, 255};
}

/**
 * StatusLED - Controls an RGB LED with blink patterns and state colors
 *
 * Uses ESP32 LEDC PWM for smooth color mixing.
 * Falls back to single-pin mode if constructed with one pin.
 *
 * Patterns:
 * - OFF: LED always off
 * - SOLID: LED always on (in current color)
 * - SLOW_BLINK: 1 Hz
 * - FAST_BLINK: 4 Hz
 * - DOUBLE_PULSE: Two quick pulses every 2 seconds
 * - ERROR_BLINK: 10 Hz
 */
class StatusLED {
public:
    // Single-pin constructor (backward compatible, onboard LED)
    StatusLED(uint8_t pin, bool activeLow = false);

    // RGB constructor (3-pin, common cathode)
    StatusLED(uint8_t pinRed, uint8_t pinGreen, uint8_t pinBlue);

    void begin();
    void update();

    void setPattern(LEDPattern pattern);
    LEDPattern getPattern() const { return _pattern; }

    // Set the color used for the current pattern
    void setColor(RGBColor color);
    RGBColor getColor() const { return _color; }

    void forceOn();
    void forceOff();
    void resumePattern();

private:
    // Pin config
    uint8_t _pin;          // Single-pin mode
    uint8_t _pinR, _pinG, _pinB;  // RGB mode
    bool _activeLow;
    bool _rgbMode;

    // State
    LEDPattern _pattern;
    RGBColor _color;
    bool _ledState;        // On or off (for blink timing)
    bool _forceOverride;
    unsigned long _lastToggleTime;
    uint8_t _pulsePhase;

    // Output control
    void setLED(bool on);
    void writeRGB(uint8_t r, uint8_t g, uint8_t b);

    // Pattern timing
    void updateBlink(unsigned long period);
    void updateDoublePulse();
};

#endif // STATUS_LED_H
