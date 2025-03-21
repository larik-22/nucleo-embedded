#ifndef WHADDA_H
#define WHADDA_H

// BASE parameters
#define MESSAGE_DELAY 250 // Delay for showing messages
#define DEBOUNCE_DELAY 50 // Delay for button debounce
#define BLINK_DELAY 150   // Delay for blinking LEDs
#define BLINK_COUNT 3     // Number of blinks for LED effects
#define BLINK_ALL 0xFF    // Bitmask to blink all LEDs

#include "TM1638plus.h"

/**
 * @class Whadda
 * @brief A wrapper around the TM1638plus library.
 *
 * The Whadda class encapsulates a TM1638plus instance and provides utilities.
 */
class Whadda
{
public:
    /**
     * @brief Constructs a new Whadda object.
     *
     * @param strobe Arduino digital pin used for the strobe (STB) connection.
     * @param clock  Arduino digital pin used for the clock (CLK) connection.
     * @param data   Arduino digital pin used for the data (DIO) connection.
     * @param highfreq Optional flag to indicate high frequency operation.
     */
    Whadda(uint8_t strobe, uint8_t clock, uint8_t data, bool highfreq = false);

    /**
     * @brief Initializes the display.
     */
    void displayBegin();

    /**
     * @brief Sets the state of a single LED.
     *
     * @param position The LED position (0-7).
     * @param value The LED state, where 0 is off and non-zero is on.
     */
    void setLED(uint8_t position, uint8_t value);

    /**
     * @brief Sets all the LEDs at once.
     *
     * @param greenred Bit mask representing the LEDs state.
     */
    void setLEDs(uint16_t greenred);

    /**
     * @brief Displays a text string on the display.
     *
     * @param text Pointer to a null-terminated character array containing the text.
     */
    void displayText(const char *text);

    /**
     * @brief Displays an ASCII symbol at a specific position.
     *
     * @param position The digit position.
     * @param ascii The ASCII value to display.
     */
    void displayASCII(uint8_t position, uint8_t ascii);

    /**
     * @brief Displays an ASCII symbol with an additional decimal dot.
     *
     * @param position The digit position.
     * @param ascii The ASCII value to display.
     */
    void displayASCIIwDot(uint8_t position, uint8_t ascii);

    /**
     * @brief Displays a hexadecimal value at a specific position.
     *
     * @param position The digit position.
     * @param hex The hexadecimal value to display.
     */
    void displayHex(uint8_t position, uint8_t hex);

    /**
     * @brief Displays a value on a 7-segment display.
     *
     * @param position The digit position.
     * @param value The value to display.
     */
    void display7Seg(uint8_t position, uint8_t value);

    /**
     * @brief Displays an integer number.
     *
     * @param number The number to display.
     * @param leadingZeros If true, display leading zeros.
     * @param alignment Alignment type for the display.
     */
    void displayIntNum(unsigned long number, boolean leadingZeros = true, AlignTextType_e alignment = TMAlignTextLeft);

    /**
     * @brief Displays two decimal numbers, one in each nibble of the display.
     *
     * @param numberUpper The upper nibble number (0-9999).
     * @param numberLower The lower nibble number (0-9999).
     * @param leadingZeros If true, display leading zeros.
     * @param alignment Alignment type for the display.
     */
    void DisplayDecNumNibble(uint16_t numberUpper, uint16_t numberLower, boolean leadingZeros = true, AlignTextType_e alignment = TMAlignTextLeft);

    /**
     * @brief Reads the status of the buttons.
     *
     * @return uint8_t A bit mask representing button states.
     */
    uint8_t readButtons();

    /**
     * @brief Reads the status of the buttons with debounce.
     *
     * @return uint8_t A bit mask representing button states.
     */
    uint8_t readButtonsWithDebounce(int debounceDelay = DEBOUNCE_DELAY);

    /**
     * @brief Clears the 7-segment display.
     *
     * Iterates through all 8 digits and clears their display.
     */
    void clearDisplay();

    /**
     * @brief blink whadda LEDs
     */
    void blinkLEDs(uint16_t num, int count, int blinkDelay);

    /**
     * @brief shows a message on the display for a specified duration.
     */
    void showTemporaryMessage(const char *msg, int durationMs = MESSAGE_DELAY);

private:
    TM1638plus tm;
};

#endif