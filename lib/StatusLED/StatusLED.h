#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

/**
 * StatusLED - Controls onboard LED with various blink patterns
 *
 * Patterns:
 * - OFF: LED always off
 * - SOLID: LED always on
 * - SLOW_BLINK: 1 Hz (500ms on/off)
 * - FAST_BLINK: 4 Hz (125ms on/off)
 * - DOUBLE_PULSE: Two quick pulses every 2 seconds
 * - ERROR_BLINK: 10 Hz (50ms on/off)
 */
class StatusLED {
public:
    /**
     * Constructor
     * @param pin GPIO pin for LED
     * @param activeLow true if LED is on when pin is LOW
     */
    StatusLED(uint8_t pin, bool activeLow = false);

    /**
     * Initialize LED pin
     */
    void begin();

    /**
     * Update LED state - call frequently in main loop
     */
    void update();

    /**
     * Set the current LED pattern
     * @param pattern Pattern to display
     */
    void setPattern(LEDPattern pattern);

    /**
     * Get current pattern
     */
    LEDPattern getPattern() const { return _pattern; }

    /**
     * Force LED on (temporarily overrides pattern)
     */
    void forceOn();

    /**
     * Force LED off (temporarily overrides pattern)
     */
    void forceOff();

    /**
     * Resume pattern after force on/off
     */
    void resumePattern();

private:
    uint8_t _pin;
    bool _activeLow;
    LEDPattern _pattern;

    bool _ledState;           // Current LED state
    bool _forceOverride;      // Force mode active
    unsigned long _lastToggleTime;
    uint8_t _pulsePhase;      // For double pulse pattern

    /**
     * Set the physical LED state
     */
    void setLED(bool on);

    /**
     * Update blink pattern timing
     */
    void updateBlink(unsigned long period);

    /**
     * Update double pulse pattern
     */
    void updateDoublePulse();
};

#endif // STATUS_LED_H
