#pragma once
#include <Arduino.h>

#define FADE_DELAY 10
#define BLINK_DELAY 150

/**
 * @class RGBLed
 * @brief Controls an RGB LED through three PWM-enabled pins.
 *
 * The RGBLed class allows you to set the LED to a specific color, fade between colors,
 * and blink the LED a given number of times.
 */
class RGBLed
{
public:
    /**
     * @class RGBLed
     * @brief Controls an RGB LED through three PWM-enabled pins.
     */
    RGBLed(int redPin, int greenPin, int bluePin);

    /**
     * @brief Initializes the RGB LED pins and turns the LED off.
     *
     * This method must be called in setup() to initialize the LED pins before use.
     */
    void begin();

    /**
     * @brief Sets the RGB LED to the specified red, green, and blue intensities.
     *
     * @param redValue   Intensity of the red channel (0-255).
     * @param greenValue Intensity of the green channel (0-255).
     * @param blueValue  Intensity of the blue channel (0-255).
     */
    void setColor(int redValue, int greenValue, int blueValue);

    /**
     * @brief Sets the RGB LED color using a 24-bit hexadecimal value.
     *
     * @param colorHex A 24-bit color in the format 0xRRGGBB.
     */
    void setHexColor(uint32_t colorHex);

    /**
     * @brief Turns off the RGB LED by setting all channels to zero.
     */
    void off();

    /**
     * @brief Blinks the LED a specified number of times.
     *
     * @param count The number of times to blink the LED.
     */
    void blinkCurrentColor(int count);

    /**
     * @brief Blinks the LED with the specified color a given number of times.
     *
     * @param redValue   Intensity of the red channel (0-255).
     * @param greenValue Intensity of the green channel (0-255).
     * @param blueValue  Intensity of the blue channel (0-255).
     * @param count      The number of times to blink the LED.
     */
    void blinkColor(int redValue, int greenValue, int blueValue, int count = 1);

    // Non-blocking blink routines:
    // Call one of these to start a blink sequence.
    void startBlinkCurrent(int count);
    void startBlinkColor(int redValue, int greenValue, int blueValue, int count = 1);
    // Call update() repeatedly (e.g. in loop()) to process blink transitions.
    void update();

private:
    int _redPin, _greenPin, _bluePin;
    int _currentRed, _currentGreen, _currentBlue;

    bool _isBlinking;
    int _blinkCount;  // Remaining transitions (on/off)
    bool _blinkState; // Current state: true means LED is on, false means off
    unsigned long _lastBlinkTime;
    int _blinkRed, _blinkGreen, _blinkBlue;
};