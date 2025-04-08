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
 * @brief Blinks the current color a specified number of times.
 *
 * @param count The number of blink cycles to perform.
 */
void RGBLed::blinkCurrentColor(int count)
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

/**
 * @brief Blinks the LED with the specified color a given number of times.
 *
 * @param redValue   The intensity of the red channel (0-255).
 * @param greenValue The intensity of the green channel (0-255).
 * @param blueValue  The intensity of the blue channel (0-255).
 * @param count      The number of blink cycles to perform.
 */
void RGBLed::blinkColor(int redValue, int greenValue, int blueValue, int count)
{
    for (int i = 0; i < count; i++)
    {
        off();
        delay(BLINK_DELAY);
        setColor(redValue, greenValue, blueValue);
        delay(BLINK_DELAY);
    }
}

// Initiates a blink sequence using the current LED color.
void RGBLed::startBlinkCurrent(int count)
{
    startBlinkColor(_currentRed, _currentGreen, _currentBlue, count);
}

// Initiates a blink sequence with a specific color.
// 'count' indicates the number of full blink cycles.
void RGBLed::startBlinkColor(int redValue, int greenValue, int blueValue, int count)
{
    _isBlinking = true;
    // For count blink cycles, we need count*2 transitions (LED off and on)
    _blinkCount = count * 2;
    _blinkState = true; // Start with LED on
    _lastBlinkTime = millis();
    _blinkRed = redValue;
    _blinkGreen = greenValue;
    _blinkBlue = blueValue;
    setColor(redValue, greenValue, blueValue);
}

// Call this method in loop() to update the blinking state.
void RGBLed::update()
{
    if (_isBlinking)
    {
        unsigned long currentMillis = millis();
        if (currentMillis - _lastBlinkTime >= BLINK_DELAY)
        {
            _lastBlinkTime = currentMillis;
            if (_blinkState)
            {
                off();
            }
            else
            {
                setColor(_blinkRed, _blinkGreen, _blinkBlue);
            }
            _blinkState = !_blinkState;
            _blinkCount--;
            if (_blinkCount <= 0)
            {
                _isBlinking = false;
                // Optionally, leave the LED on after finishing:
                setColor(_blinkRed, _blinkGreen, _blinkBlue);
            }
        }
    }
}