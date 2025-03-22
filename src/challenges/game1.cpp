#include <Arduino.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "game1.h"

// TODO clean up code (put in functions, remove debug prints, etc.)
// TODO: Make sure game is non-blocking and never uses delay(). Game-state mindset

// ---------------------------------------------------------------------------
// External references to utility objects from main.cpp
// ---------------------------------------------------------------------------
extern Buzzer buzzer;
extern RGBLed rgbLed;
extern Whadda whadda;

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static const int MAX_LEVEL = 8;               // How many rounds to complete
static const int INITIAL_SEQUENCE_LENGTH = 2; // Sequence length at early levels
static const int MAX_SEQUENCE_SIZE = 32;      // Safety for storing generated data
static const int BASE_DELAY = 500;            // Base delay for round transitions

// Frequency table for each LED (0..7)
static const int ledFrequencies[8] = {220, 262, 294, 330, 349, 392, 440, 494};

// ---------------------------------------------------------------------------
// Internal State Variables
// ---------------------------------------------------------------------------
static bool challengeInitialized = false; // One-time init flag
static bool challengeComplete = false;    // Set true upon winning

// Memory-sequence data
static int level = 0;
static int seqLength = 0;
static int sequence[MAX_SEQUENCE_SIZE];
static int userIndex = 0;

// Simple state machine
enum GameState
{
    IDLE,
    DISPLAY_SEQUENCE,
    GET_USER_INPUT,
    ROUND_WIN_CHECK,
};
static GameState currentState = IDLE;

// For basic button debouncing
static unsigned long lastPressTime = 0;
static int lastButtonPressed = -1;

// ---------------------------------------------------------------------------
// Forward Declarations of Internal Functions
// ---------------------------------------------------------------------------
static void startGame();
static void generateSequence(int length);
static void displaySequence();
static void checkUserInput();
static void nextLevel();
static void finishGame();
static int getSequenceLengthForLevel(int lvl);
static void updateDisplay();
static int getRoundDelay(int lvl);

// ---------------------------------------------------------------------------
// Public Function: runChallenge()
//    - Called repeatedly from the main program to handle the puzzle’s logic.
//    - Returns true once the player completes all required rounds (MAX_LEVEL).
// ---------------------------------------------------------------------------
bool runGame1()
{
    // Initialize once
    if (!challengeInitialized)
    {
        buzzer.playRoundStartMelody();
        challengeInitialized = true;
        challengeComplete = false;
        currentState = IDLE;

        whadda.clearDisplay();
    }

    // If already complete, just return true every time
    if (challengeComplete)
    {
        return true;
    }

    // State machine
    switch (currentState)
    {
    case IDLE:
        // Automatically start once
        startGame();
        break;

    case DISPLAY_SEQUENCE:
        displaySequence();
        break;

    case GET_USER_INPUT:
        checkUserInput();
        break;

    case ROUND_WIN_CHECK:
        if (level >= MAX_LEVEL)
        {
            // Completed all rounds => puzzle solved
            finishGame();
        }
        else
        {
            // Advance to next level
            nextLevel();
        }
        break;
    }

    return challengeComplete;
}

// ---------------------------------------------------------------------------
// Internal Function Definitions
// ---------------------------------------------------------------------------
/**
 * Reset puzzle state and generate initial sequence
 */
static void startGame()
{
    level = 1;
    seqLength = getSequenceLengthForLevel(level);
    userIndex = 0;

    lastPressTime = 0;
    lastButtonPressed = -1;

    // Small "start" effect
    whadda.blinkLEDs(0xFF, 2, 200);
    buzzer.playTone(1200, 300);

    generateSequence(seqLength);
    currentState = DISPLAY_SEQUENCE;
    updateDisplay();
}

/**
 * generateSequence():
 *   - Randomly fill sequence[] for the given length
 *   - Avoid 4 identical consecutive entries
 */
static void generateSequence(int length)
{
    if (length > MAX_SEQUENCE_SIZE)
    {
        length = MAX_SEQUENCE_SIZE; // Safety clamp
    }

    // First random element
    sequence[0] = random(0, 8);

    // Fill the remainder
    for (int i = 1; i < length; i++)
    {
        if (i >= 3 &&
            sequence[i - 1] == sequence[i - 2] &&
            sequence[i - 2] == sequence[i - 3])
        {
            // Avoid 4 identical in a row
            int newVal;
            do
            {
                newVal = random(0, 8);
            } while (newVal == sequence[i - 1]);
            sequence[i] = newVal;
        }
        else
        {
            sequence[i] = random(0, 8);
        }
    }

    // print the sequence for debugging
    Serial.print("Generated sequence: ");
    for (int i = 0; i < length; i++)
    {
        Serial.print("LED");
        Serial.print(sequence[i] + 1);
        if (i < length - 1)
            Serial.print(" -> ");
    }
    Serial.println();
}

/**
 * @brief Shows the current round’s sequence to the player,
 * Then transitions to GET_USER_INPUT
 */
static void displaySequence()
{
    whadda.displayText("");

    for (int i = 0; i < seqLength; i++)
    {
        // shift to red LED region
        uint16_t ledMask = (1 << sequence[i]) << 8;
        whadda.setLEDs(ledMask);

        buzzer.playTone(ledFrequencies[sequence[i]], 200);
        delay(300);

        whadda.setLEDs(0x0000);
        delay(200);
    }

    userIndex = 0;
    currentState = GET_USER_INPUT;
    updateDisplay();
}

/**
 * @brief Check user input against the current sequence.
 *
 * Polls Whadda buttons using a debounced read, compares input with current sequence.
 * If correct => proceed
 * If incorrect => show "ERR" and replay the same sequence.
 */
static void checkUserInput()
{
    // Use the debounced read function with a 200ms debounce delay.
    uint8_t buttons = whadda.readButtonsWithDebounce();

    // If no button is pressed, reset lastButtonPressed and exit.
    if (buttons == 0)
    {
        lastButtonPressed = -1; // no button pressed
        return;
    }

    // Identify which button was pressed.
    for (int btnIndex = 0; btnIndex < 8; btnIndex++)
    {
        if (buttons & (1 << btnIndex))
        {
            unsigned long now = millis();

            // Ignore repeated rapid presses of the same button.
            if (btnIndex == lastButtonPressed && (now - lastPressTime) < 200)
            {
                return;
            }

            lastPressTime = now;
            lastButtonPressed = btnIndex;

            // Compare with the next required element in the sequence.
            if (btnIndex == sequence[userIndex])
            {
                // Correct guess: Play tone and flash the corresponding LED.
                buzzer.playTone(ledFrequencies[btnIndex], 150);

                // Light the LED for a brief period.
                uint16_t ledMask = (1 << btnIndex) << 8;
                whadda.setLEDs(ledMask);
                delay(200);
                whadda.setLEDs(0x0000);

                userIndex++;

                // If the entire sequence has been correctly matched.
                if (userIndex >= seqLength)
                {
                    currentState = ROUND_WIN_CHECK;
                }
            }
            else
            {
                // Wrong guess: Signal error and replay the sequence.
                buzzer.playTone(300, 700);
                whadda.showTemporaryMessage("ERROR!", 1000);
                updateDisplay();

                // Restart sequence playback.
                userIndex = 0;
                currentState = DISPLAY_SEQUENCE;
            }
            break;
        }
    }
}

/**
 * @brief Move to the next level, build new sequence, show it.
 */
static void nextLevel()
{
    level++;
    delay(getRoundDelay(level));

    seqLength = getSequenceLengthForLevel(level);
    generateSequence(seqLength);
    updateDisplay();
    currentState = DISPLAY_SEQUENCE;
}

/**
 *  @brief The user has completed all rounds => puzzle solved.
 *
 *  Show a success animation, then set challengeComplete = true.
 */
static void finishGame()
{
    currentState = IDLE;
    challengeComplete = true;

    // Flash and play a “win” melody
    whadda.blinkLEDs(0xFF, 3, 200);
    buzzer.playWinMelody();
    whadda.clearDisplay();
    whadda.displayText("GOOD");
    delay(1000);

    whadda.clearDisplay();
    whadda.setLEDs(0x0000);
}

/**
 * @brief Customize how the sequence length ramps up per level.
 */
static int getSequenceLengthForLevel(int lvl)
{
    // Simple example progression
    if (lvl <= 2)
        return INITIAL_SEQUENCE_LENGTH; // e.g., 2
    else if (lvl <= 4)
        return 3;
    else if (lvl <= 6)
        return 4;
    else
        return 5;
}

/**
 *  @brief Show the current level (or round) on Whadda display and fill dots on 7-segment displays.
 */
static void updateDisplay()
{
    // Determine how many dots to fill based on the current level.
    // Assume the 7-seg display has 8 positions and we fill one dot per level (max 8).
    int dotCount = (level <= 8) ? level : 8;

    // Fill the 7-seg display dots.
    // For each position, if it's within dotCount, display a "filled" indicator (using value 1),
    // otherwise clear it by displaying 0.
    for (uint8_t pos = 0; pos < 8; pos++)
    {
        if (pos < dotCount)
        {
            whadda.display7Seg(pos, 1);
        }
        else
        {
            whadda.display7Seg(pos, 0);
        }
    }
}

/**
 * Returns a decreasing delay as level increases (never < 200ms).
 *
 * Used to create shorter pauses between levels at higher difficulty.
 */
static int getRoundDelay(int lvl)
{
    int decrement = 100 * (lvl - 1);
    int result = BASE_DELAY - decrement;
    if (result < 200)
    {
        result = 200;
    }
    return result;
}
