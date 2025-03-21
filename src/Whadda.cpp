#include "Whadda.h"

/**
 * @brief Constructs a new Whadda object and initializes the TM1638plus instance.
 *
 * @param strobe Arduino digital pin used for the strobe (STB) connection.
 * @param clock  Arduino digital pin used for the clock (CLK) connection.
 * @param data   Arduino digital pin used for the data (DIO) connection.
 * @param highfreq Optional flag to indicate high frequency operation.
 */
Whadda::Whadda(uint8_t strobe, uint8_t clock, uint8_t data, bool highfreq)
    : tm(strobe, clock, data, highfreq)
{
}

/**
 * @brief Initializes the display module.
 */
void Whadda::displayBegin()
{
    tm.displayBegin();
}

/**
 * @brief Sets the state of a single LED at the specified position.
 *
 * @param position The LED position (0-7).
 * @param value The LED state (0 for off, non-zero for on).
 */
void Whadda::setLED(uint8_t position, uint8_t value)
{
    tm.setLED(position, value);
}

/**
 * @brief Sets the state of all LEDs at once.
 *
 * @param greenred Bit mask representing the state of the LEDs.
 */
void Whadda::setLEDs(uint16_t greenred)
{
    tm.setLEDs(greenred);
}

/**
 * @brief Displays a text string on the module's display.
 *
 * @param text Pointer to a null-terminated character array with the text.
 */
void Whadda::displayText(const char *text)
{
    tm.displayText(text);
}

/**
 * @brief Displays an ASCII value on the display at the given position.
 *
 * @param position The digit position.
 * @param ascii The ASCII value to display.
 */
void Whadda::displayASCII(uint8_t position, uint8_t ascii)
{
    tm.displayASCII(position, ascii);
}

/**
 * @brief Displays an ASCII value along with a decimal point.
 *
 * @param position The digit position.
 * @param ascii The ASCII value to display.
 */
void Whadda::displayASCIIwDot(uint8_t position, uint8_t ascii)
{
    tm.displayASCIIwDot(position, ascii);
}

/**
 * @brief Displays a hexadecimal value on the display.
 *
 * @param position The digit position.
 * @param hex The hexadecimal value to display.
 */
void Whadda::displayHex(uint8_t position, uint8_t hex)
{
    tm.displayHex(position, hex);
}

/**
 * @brief Displays a value on the 7-segment display.
 *
 * @param position The digit position.
 * @param value The value to display.
 */
void Whadda::display7Seg(uint8_t position, uint8_t value)
{
    tm.display7Seg(position, value);
}

/**
 * @brief Displays an integer number on the display.
 *
 * @param number The number to display.
 * @param leadingZeros If true, leading zeros will be displayed.
 * @param alignment The alignment specification (left or right).
 */
void Whadda::displayIntNum(unsigned long number, boolean leadingZeros, AlignTextType_e alignment)
{
    tm.displayIntNum(number, leadingZeros, alignment);
}

/**
 * @brief Displays two decimal numbers in separate nibbles of the display.
 *
 * @param numberUpper The number for the upper nibble.
 * @param numberLower The number for the lower nibble.
 * @param leadingZeros If true, display leading zeros.
 * @param alignment The alignment specification (left or right).
 */
void Whadda::DisplayDecNumNibble(uint16_t numberUpper, uint16_t numberLower, boolean leadingZeros, AlignTextType_e alignment)
{
    tm.DisplayDecNumNibble(numberUpper, numberLower, leadingZeros, alignment);
}

/**
 * @brief Reads the current state of the buttons.
 *
 * @return uint8_t A bitmask representing the button states.
 */
uint8_t Whadda::readButtons()
{
    return tm.readButtons();
}

/**
 * @brief Reads the current state of the buttons with debounce.
 *
 * This function reads the button state and resets the debounce timer
 * every time a change is detected. It returns the state once it remains
 * unchanged for the specified debounce period.
 *
 * @param debounceDelayMs The debounce delay time in milliseconds.
 * @return uint8_t A bitmask representing the stable button states.
 */
uint8_t Whadda::readButtonsWithDebounce(int debounceDelayMs)
{
    unsigned long t0 = millis();
    uint8_t last = tm.readButtons();

    while (millis() - t0 < debounceDelayMs)
    {
        uint8_t current = tm.readButtons();
        if (current != last)
        {
            t0 = millis();
            last = current;
        }
    }

    return last;
}
/**
 * @brief Clears the display by turning off all segments.
 *
 * Iterates over all 8 positions and sets each 7-segment display to blank (0x00).
 */
void Whadda::clearDisplay()
{
    for (int i = 0; i <= 7; i++)
    {
        tm.display7Seg(i, 0x00);
    }
}

// TODO: RM blocking call
/**
 * @brief Blinks the LEDs a specified number of times.
 *
 * This function takes a bitmask representing the LEDs to blink, the number of times to blink,
 * and the delay between blinks. The LEDs are turned on and off in a loop.
 */
void Whadda::blinkLEDs(uint16_t num, int count, int blinkDelay)
{
    num <<= 8; // shift to red LED region
    for (int i = 0; i < count; i++)
    {
        tm.setLEDs(num);
        delay(blinkDelay);
        tm.setLEDs(0x0000);
        delay(blinkDelay);
    }
}

// TODO: RM blocking call
/**
 * @brief Displays a message on the display for a specified duration.
 *
 * This function clears the display, shows the message, waits for the specified duration,
 * and then clears the display again.
 *
 * @param msg The message to display.
 * @param durationMs The duration in milliseconds to show the message.
 */
void Whadda::showTemporaryMessage(const char *msg, int durationMs)
{
    clearDisplay();
    displayText(msg);
    delay(durationMs);
    clearDisplay();
}