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