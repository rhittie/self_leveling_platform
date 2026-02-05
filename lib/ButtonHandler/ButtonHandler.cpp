#include "ButtonHandler.h"

ButtonHandler::ButtonHandler(uint8_t pin, bool activeLow)
    : _pin(pin)
    , _activeLow(activeLow)
    , _lastRawState(false)
    , _isPressed(false)
    , _wasPressed(false)
    , _longPressTriggered(false)
    , _lastDebounceTime(0)
    , _pressStartTime(0)
{
}

void ButtonHandler::begin() {
    // Use internal pull-up if active low
    if (_activeLow) {
        pinMode(_pin, INPUT_PULLUP);
    } else {
        pinMode(_pin, INPUT);
    }

    // Initialize state
    bool rawState = digitalRead(_pin);
    _lastRawState = rawState;
    _isPressed = _activeLow ? !rawState : rawState;
    _wasPressed = _isPressed;

    Serial.printf("ButtonHandler: Initialized on pin %d\n", _pin);
}

ButtonEvent ButtonHandler::update() {
    ButtonEvent event = ButtonEvent::NONE;
    unsigned long currentTime = millis();

    // Read current state
    bool rawState = digitalRead(_pin);
    bool currentPressed = _activeLow ? !rawState : rawState;

    // Check if state changed
    if (rawState != _lastRawState) {
        _lastDebounceTime = currentTime;
        _lastRawState = rawState;
    }

    // Apply debounce
    if ((currentTime - _lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
        // State has been stable long enough
        if (currentPressed != _isPressed) {
            _isPressed = currentPressed;

            if (_isPressed) {
                // Button just pressed
                _pressStartTime = currentTime;
                _longPressTriggered = false;
            } else {
                // Button just released
                if (!_longPressTriggered) {
                    // Only report short press if long press wasn't triggered
                    event = ButtonEvent::SHORT_PRESS;
                }
            }
        }
    }

    // Check for long press while button is held
    if (_isPressed && !_longPressTriggered) {
        if ((currentTime - _pressStartTime) >= BUTTON_LONG_PRESS_MS) {
            _longPressTriggered = true;
            event = ButtonEvent::LONG_PRESS;
        }
    }

    _wasPressed = _isPressed;
    return event;
}

unsigned long ButtonHandler::getHoldTime() const {
    if (!_isPressed) {
        return 0;
    }
    return millis() - _pressStartTime;
}
