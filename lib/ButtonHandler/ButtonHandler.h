#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

/**
 * ButtonHandler - Debounced button input with short and long press detection
 *
 * Features:
 * - Hardware debouncing via software timing
 * - Short press detection (release within threshold)
 * - Long press detection (held beyond threshold)
 * - Internal pull-up support
 */
class ButtonHandler {
public:
    /**
     * Constructor
     * @param pin GPIO pin number
     * @param activeLow true if button connects pin to GND when pressed
     */
    ButtonHandler(uint8_t pin, bool activeLow = true);

    /**
     * Initialize the button pin
     */
    void begin();

    /**
     * Update button state - call frequently in main loop
     * @return Button event if one occurred, NONE otherwise
     */
    ButtonEvent update();

    /**
     * Check if button is currently pressed (after debounce)
     */
    bool isPressed() const { return _isPressed; }

    /**
     * Get how long button has been held (ms)
     * Returns 0 if not pressed
     */
    unsigned long getHoldTime() const;

private:
    uint8_t _pin;
    bool _activeLow;

    bool _lastRawState;      // Last raw reading
    bool _isPressed;         // Debounced pressed state
    bool _wasPressed;        // Previous debounced state
    bool _longPressTriggered;// Long press already reported

    unsigned long _lastDebounceTime;
    unsigned long _pressStartTime;
};

#endif // BUTTON_HANDLER_H
