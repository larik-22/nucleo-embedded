#include "RGBLed.h"

/**
 * @brief Constructs an RGBLed object.
 *
 * Initializes internal pin references and current color values.
 *
 * @param redPin   Arduino pin connected to the red channel of the LED.
 * @param greenPin Arduino pin connected to the green channel of the LED.
 * @param bluePin  Arduino pin connected to the blue channel of the LED.
 */
RGBLed::RGBLed(int redPin, int greenPin, int bluePin)
    : _redPin(redPin),
      _greenPin(greenPin),
      _bluePin(bluePin),
      _currentRed(0),
      _currentGreen(0),
      _currentBlue(0) {}

/**
 * @brief Initializes the pins and turns the LED off.
 */
void RGBLed::begin()
{
    pinMode(_redPin, OUTPUT);
    pinMode(_greenPin, OUTPUT);
    pinMode(_bluePin, OUTPUT);
    off();
}

/**
 * @brief Sets the RGB LED to the specified color values.
 *
 * @param redValue   The intensity of the red channel (0-255).
 * @param greenValue The intensity of the green channel (0-255).
 * @param blueValue  The intensity of the blue channel (0-255).
 */
void RGBLed::setColor(int redValue, int greenValue, int blueValue)
{
    analogWrite(_redPin, redValue);
    analogWrite(_greenPin, greenValue);
    analogWrite(_bluePin, blueValue);
    _currentRed = redValue;
    _currentGreen = greenValue;
    _currentBlue = blueValue;
}

/**
 * @brief Sets the RGB LED color using a 24-bit hex value.
 *
 * @param colorHex A 24-bit color (0xRRGGBB).
 */
void RGBLed::setHexColor(uint32_t colorHex)
{
    int redValue = (colorHex >> 16) & 0xFF;
    int greenValue = (colorHex >> 8) & 0xFF;
    int blueValue = colorHex & 0xFF;
    setColor(redValue, greenValue, blueValue);
}

/**
 * @brief Turns off the RGB LED by setting all channels to zero.
 */
void RGBLed::off()
{
    setColor(0, 0, 0);
}

/**
 * @brief Gradually changes the current color to the specified target color.
 *
 * @param targetRed   The target red intensity (0-255).
 * @param targetGreen The target green intensity (0-255).
 * @param targetBlue  The target blue intensity (0-255).
 * @param delayMs     The delay (in milliseconds) for each step of the fade.
 */
void RGBLed::fadeColor(int targetRed, int targetGreen, int targetBlue, int delayMs)
{
    const int steps = 255;
    for (int i = 0; i <= steps; i++)
    {
        int newRed = _currentRed + ((targetRed - _currentRed) * i) / steps;
        int newGreen = _currentGreen + ((targetGreen - _currentGreen) * i) / steps;
        int newBlue = _currentBlue + ((targetBlue - _currentBlue) * i) / steps;

        setColor(newRed, newGreen, newBlue);
        delay(delayMs);
    }
}

/**
 * @brief Fades from the current color to off.
 *
 * @param delayMs The delay (in milliseconds) for each step of the fade.
 */
void RGBLed::fadeToOff(int delayMs)
{
    fadeColor(0, 0, 0, delayMs);
}

/**
 * @brief Blinks the current color a specified number of times.
 *
 * @param count The number of blink cycles to perform.
 */
void RGBLed::blink(int count)
{
    int savedRed = _currentRed;
    int savedGreen = _currentGreen;
    int savedBlue = _currentBlue;

    for (int i = 0; i < count; i++)
    {
        off();
        delay(BLINK_DELAY);
        setColor(savedRed, savedGreen, savedBlue);
        delay(BLINK_DELAY);
    }
}