#include "StatusLED.h"

StatusLED::StatusLED(uint8_t pin, bool activeLow)
    : _pin(pin)
    , _activeLow(activeLow)
    , _pattern(LEDPattern::OFF)
    , _ledState(false)
    , _forceOverride(false)
    , _lastToggleTime(0)
    , _pulsePhase(0)
{
}

void StatusLED::begin() {
    pinMode(_pin, OUTPUT);
    setLED(false);
    Serial.printf("StatusLED: Initialized on pin %d\n", _pin);
}

void StatusLED::update() {
    if (_forceOverride) {
        return;  // Don't update if in force mode
    }

    unsigned long currentTime = millis();

    switch (_pattern) {
        case LEDPattern::OFF:
            setLED(false);
            break;

        case LEDPattern::SOLID:
            setLED(true);
            break;

        case LEDPattern::SLOW_BLINK:
            updateBlink(LED_SLOW_BLINK_MS);
            break;

        case LEDPattern::FAST_BLINK:
            updateBlink(LED_FAST_BLINK_MS);
            break;

        case LEDPattern::ERROR_BLINK:
            updateBlink(LED_ERROR_BLINK_MS);
            break;

        case LEDPattern::DOUBLE_PULSE:
            updateDoublePulse();
            break;
    }
}

void StatusLED::setPattern(LEDPattern pattern) {
    if (pattern != _pattern) {
        _pattern = pattern;
        _lastToggleTime = millis();
        _pulsePhase = 0;
        _forceOverride = false;

        // Immediately update to new pattern
        update();
    }
}

void StatusLED::forceOn() {
    _forceOverride = true;
    setLED(true);
}

void StatusLED::forceOff() {
    _forceOverride = true;
    setLED(false);
}

void StatusLED::resumePattern() {
    _forceOverride = false;
    _lastToggleTime = millis();
}

void StatusLED::setLED(bool on) {
    _ledState = on;
    if (_activeLow) {
        digitalWrite(_pin, on ? LOW : HIGH);
    } else {
        digitalWrite(_pin, on ? HIGH : LOW);
    }
}

void StatusLED::updateBlink(unsigned long period) {
    unsigned long currentTime = millis();

    if ((currentTime - _lastToggleTime) >= period) {
        _lastToggleTime = currentTime;
        setLED(!_ledState);
    }
}

void StatusLED::updateDoublePulse() {
    // Double pulse pattern:
    // Phase 0: LED on for 100ms
    // Phase 1: LED off for 100ms
    // Phase 2: LED on for 100ms
    // Phase 3: LED off for remaining time (~1700ms)

    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - _lastToggleTime;

    const unsigned long PULSE_ON_TIME = 100;
    const unsigned long PULSE_GAP = 100;

    switch (_pulsePhase) {
        case 0:  // First pulse on
            setLED(true);
            if (elapsed >= PULSE_ON_TIME) {
                _lastToggleTime = currentTime;
                _pulsePhase = 1;
            }
            break;

        case 1:  // Gap between pulses
            setLED(false);
            if (elapsed >= PULSE_GAP) {
                _lastToggleTime = currentTime;
                _pulsePhase = 2;
            }
            break;

        case 2:  // Second pulse on
            setLED(true);
            if (elapsed >= PULSE_ON_TIME) {
                _lastToggleTime = currentTime;
                _pulsePhase = 3;
            }
            break;

        case 3:  // Long off period
            setLED(false);
            // Total period is 2000ms, subtract pulse times
            if (elapsed >= (LED_DOUBLE_PULSE_PERIOD_MS - PULSE_ON_TIME * 2 - PULSE_GAP)) {
                _lastToggleTime = currentTime;
                _pulsePhase = 0;
            }
            break;
    }
}
