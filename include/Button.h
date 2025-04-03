#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button
{
public:
    // Constructor: specify pin and optional debounce delay (milliseconds)
    Button(uint8_t pin, unsigned long debounceDelay = 50);

    // You have this already implemented elsewhere.
    // It's responsible for returning true if the button is pressed, false if not,
    // while applying debouncing logic internally.
    bool readWithDebounce();

    // 1) Updates internal press-tracking each loop
    //    Returns the *current* stable pressed/not pressed state.
    bool read();

    // 2) Returns true if the button was pressed since last check (rising edge).
    bool wasPressed();

private:
    uint8_t _pin;
    unsigned long _debounceDelay;

    // Internal state used for debouncing logic
    uint8_t _lastReading;
    uint8_t _buttonState;
    unsigned long _lastDebounceTime;

    // Press event tracking
    bool _prevPress;
    bool _wasPressedFlag;
};

#endif // BUTTON_H
