#include "StatusLED.h"

// Single-pin constructor (backward compatible)
StatusLED::StatusLED(uint8_t pin, bool activeLow)
    : _pin(pin)
    , _pinR(0), _pinG(0), _pinB(0)
    , _activeLow(activeLow)
    , _rgbMode(false)
    , _pattern(LEDPattern::OFF)
    , _color(LEDColors::WHITE)
    , _ledState(false)
    , _forceOverride(false)
    , _lastToggleTime(0)
    , _pulsePhase(0)
{
}

// RGB constructor
StatusLED::StatusLED(uint8_t pinRed, uint8_t pinGreen, uint8_t pinBlue)
    : _pin(0)
    , _pinR(pinRed), _pinG(pinGreen), _pinB(pinBlue)
    , _activeLow(false)
    , _rgbMode(true)
    , _pattern(LEDPattern::OFF)
    , _color(LEDColors::WHITE)
    , _ledState(false)
    , _forceOverride(false)
    , _lastToggleTime(0)
    , _pulsePhase(0)
{
}

void StatusLED::begin() {
    if (_rgbMode) {
        // Set up LEDC PWM channels for each color
        ledcSetup(LEDC_CHANNEL_RED,   LEDC_FREQ, LEDC_RESOLUTION);
        ledcSetup(LEDC_CHANNEL_GREEN, LEDC_FREQ, LEDC_RESOLUTION);
        ledcSetup(LEDC_CHANNEL_BLUE,  LEDC_FREQ, LEDC_RESOLUTION);

        ledcAttachPin(_pinR, LEDC_CHANNEL_RED);
        ledcAttachPin(_pinG, LEDC_CHANNEL_GREEN);
        ledcAttachPin(_pinB, LEDC_CHANNEL_BLUE);

        writeRGB(0, 0, 0);
        Serial.printf("StatusLED: RGB mode on pins R=%d G=%d B=%d\n", _pinR, _pinG, _pinB);
    } else {
        pinMode(_pin, OUTPUT);
        setLED(false);
        Serial.printf("StatusLED: Single-pin mode on pin %d\n", _pin);
    }
}

void StatusLED::update() {
    if (_forceOverride) {
        return;
    }

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
        update();
    }
}

void StatusLED::setColor(RGBColor color) {
    _color = color;
    // If currently showing, update immediately
    if (_ledState && !_forceOverride) {
        writeRGB(_color.r, _color.g, _color.b);
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

    if (_rgbMode) {
        if (on) {
            writeRGB(_color.r, _color.g, _color.b);
        } else {
            writeRGB(0, 0, 0);
        }
    } else {
        if (_activeLow) {
            digitalWrite(_pin, on ? LOW : HIGH);
        } else {
            digitalWrite(_pin, on ? HIGH : LOW);
        }
    }
}

void StatusLED::writeRGB(uint8_t r, uint8_t g, uint8_t b) {
    if (_rgbMode) {
        ledcWrite(LEDC_CHANNEL_RED,   r);
        ledcWrite(LEDC_CHANNEL_GREEN, g);
        ledcWrite(LEDC_CHANNEL_BLUE,  b);
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
            if (elapsed >= (LED_DOUBLE_PULSE_PERIOD_MS - PULSE_ON_TIME * 2 - PULSE_GAP)) {
                _lastToggleTime = currentTime;
                _pulsePhase = 0;
            }
            break;
    }
}
