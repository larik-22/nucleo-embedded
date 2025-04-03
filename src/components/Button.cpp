#include "Button.h"

Button::Button(uint8_t pin, unsigned long debounceDelay)
    : _pin(pin), _debounceDelay(debounceDelay), _lastReading(HIGH) // Default to HIGH if using INPUT_PULLUP
      ,
      _buttonState(HIGH), _lastDebounceTime(0), _prevPress(false), _wasPressedFlag(false)
{
    // Configure the pin; often INPUT_PULLUP is used for a button
    pinMode(_pin, INPUT_PULLUP);
}

// -----------------------------------------------------------------------------
// read() updates the internal press state each loop. It returns current pressed state.
// -----------------------------------------------------------------------------
bool Button::read()
{
    // Use your debounced read
    bool currentPress = readWithDebounce();

    // Rising edge check: previously not pressed, now pressed => set flag
    if (!_prevPress && currentPress)
    {
        _wasPressedFlag = true;
    }

    // Update previous press state
    _prevPress = currentPress;

    // Return current stable pressed/not pressed state
    return currentPress;
}

// -----------------------------------------------------------------------------
// wasPressed() returns true exactly once for each new press event
// (rising edge). After returning true, it resets the flag.
// -----------------------------------------------------------------------------
bool Button::wasPressed()
{
    if (_wasPressedFlag)
    {
        _wasPressedFlag = false;
        return true;
    }
    return false;
}

// Debounced read

bool Button::readWithDebounce()
{
    // Example, minimal approach (replace with your actual existing code):
    bool reading = digitalRead(_pin);

    // If the reading has changed, reset debounce timer
    if (reading != _lastReading)
    {
        _lastDebounceTime = millis();
    }

    // If it's been longer than the debounce delay, accept the new reading
    if ((millis() - _lastDebounceTime) > _debounceDelay)
    {
        _buttonState = reading;
    }

    _lastReading = reading;

    // Return true if pressed. With pull-up, pressed = LOW
    return (_buttonState == LOW);
}