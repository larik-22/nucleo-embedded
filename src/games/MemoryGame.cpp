#include "MemoryGame.h"
#include "Globals.h"

#define DEBUG_MEMORY_GAME


/**
 * @brief Constructor for MemoryGame
 * 
 * Initializes all member variables to their default values.
 * The game starts in the Idle state and is not initialized.
 */
MemoryGame::MemoryGame() : currentState(MemoryGameState::Idle),
                           seqPhase(SeqPhase::LedOn),
                           startAnimPhase(StartAnimPhase::Idle),
                           challengeInitialized(false),
                           challengeComplete(false),
                           displayStarted(false),
                           level(0),
                           seqLength(0),
                           userIndex(0),
                           lastButtonPressed(MemoryGameConfig::NO_BUTTON_PRESSED),
                           blinkCount(0),
                           blinkMask(MemoryGameConfig::ALL_LEDS_MASK),
                           lastStateChangeTime(0),
                           lastActionTime(0),
                           lastPressTime(0),
                           inputDelayStart(0),
                           errorDelayStart(0),
                           finishDelayStart(0),
                           levelDelayStart(0),
                           seqDisplayIndex(0)
{
}

/**
 * @brief Initializes the game
 * 
 * Sets up the game state, displays the initial message, and prepares for the game to start.
 * This method is called once at the beginning of the game.
 */
void MemoryGame::init()
{
    if (!challengeInitialized)
    {
        buzzer.playRoundStartMelody();
        lcd.setCursor(0, 0);
        lcd.print(MemoryGameConfig::GAME_TITLE);
        lcd.setCursor(0, 1);
        lcd.print(MemoryGameConfig::GOOD_LUCK_MESSAGE);
        challengeInitialized = true;
        challengeComplete = false;
        whadda.clearDisplay();
        setState(MemoryGameState::Init);
    }
}

/**
 * @brief Sets the game state and updates the state change time
 * 
 * @param newState The new state to set
 */
void MemoryGame::setState(MemoryGameState newState)
{
    currentState = newState;
    lastStateChangeTime = millis();
}

/**
 * @brief Resets the sequence display variables
 * 
 * Resets the sequence display index, phase, and display started flag.
 */
void MemoryGame::resetSequenceDisplay()
{
    seqDisplayIndex = 0;
    seqPhase = SeqPhase::LedOn;
    displayStarted = false;
}

/**
 * @brief Gets the sequence length for a given level
 * 
 * @param lvl The level to get the sequence length for
 * @return The sequence length for the given level
 */
int MemoryGame::getSequenceLengthForLevel(int lvl) const
{
    if (lvl <= MemoryGameConfig::LEVEL_2_THRESHOLD)
        return MemoryGameConfig::INITIAL_SEQUENCE_LENGTH;
    else if (lvl <= MemoryGameConfig::LEVEL_4_THRESHOLD)
        return MemoryGameConfig::LEVEL_2_SEQUENCE_LENGTH;
    else if (lvl <= MemoryGameConfig::LEVEL_6_THRESHOLD)
        return MemoryGameConfig::LEVEL_4_SEQUENCE_LENGTH;
    else
        return MemoryGameConfig::LEVEL_6_SEQUENCE_LENGTH;
}

/**
 * @brief Updates the 7-segment display to show the current level
 * 
 * Displays dots on the 7-segment display corresponding to the current level.
 */
void MemoryGame::update7SegmentDisplay() const
{
    int dotCount = (level <= MemoryGameConfig::MAX_LEVEL) ? level : MemoryGameConfig::MAX_LEVEL;
    for (uint8_t pos = 0; pos < MemoryGameConfig::MAX_LEVEL; pos++)
    {
        whadda.display7Seg(pos, (pos < dotCount) ? 1 : 0);
    }
}

/**
 * @brief Generates a random sequence of the given length
 * 
 * @param length The length of the sequence to generate
 * 
 * The sequence is generated randomly, but with a constraint to prevent
 * four identical consecutive values, which would make the game too easy.
 */
void MemoryGame::generateSequence(int length)
{
    if (length > MemoryGameConfig::MAX_SEQUENCE_SIZE)
        length = MemoryGameConfig::MAX_SEQUENCE_SIZE;

    sequence[0] = random(0, MemoryGameConfig::NUM_LEDS);
    for (int i = 1; i < length; i++)
    {
        // Prevent four identical consecutive values
        if (i >= 3 &&
            sequence[i - 1] == sequence[i - 2] &&
            sequence[i - 2] == sequence[i - 3])
        {
            int newVal;
            do
            {
                newVal = random(0, MemoryGameConfig::NUM_LEDS);
            } while (newVal == sequence[i - 1]);
            sequence[i] = newVal;
        }
        else
        {
            sequence[i] = random(0, MemoryGameConfig::NUM_LEDS);
        }
    }

    // Debug output
    #ifdef DEBUG_MEMORY_GAME
    Serial.print("Generated sequence: ");
    for (int i = 0; i < length; i++)
    {
        Serial.print("LED");
        Serial.print(sequence[i] + 1);
        if (i < length - 1)
            Serial.print(" -> ");
    }
    Serial.println();
    #endif
}

/**
 * @brief Updates the start animation
 * 
 * @return true when the animation is complete, false otherwise
 * 
 * This method implements a non-blocking animation that blinks all LEDs
 * a specified number of times before starting the game.
 */
bool MemoryGame::updateStartAnimation()
{
    switch (startAnimPhase)
    {
    case StartAnimPhase::Idle:
        blinkCount = 0;
        startAnimPhase = StartAnimPhase::BlinkOn;
        lastActionTime = millis();
        whadda.clearDisplay();
        return false;
    case StartAnimPhase::BlinkOn:
        whadda.setLEDs(blinkMask << MemoryGameConfig::LED_SHIFT_AMOUNT);
        if (hasElapsed(lastActionTime, MemoryGameConfig::START_ANIM_INTERVAL))
        {
            startAnimPhase = StartAnimPhase::BlinkOff;
            lastActionTime = millis();
        }
        return false;
    case StartAnimPhase::BlinkOff:
        whadda.clearLEDs();
        if (hasElapsed(lastActionTime, MemoryGameConfig::START_ANIM_INTERVAL))
        {
            blinkCount++;
            if (blinkCount < MemoryGameConfig::REQUIRED_BLINKS)
            {
                startAnimPhase = StartAnimPhase::BlinkOn;
                lastActionTime = millis();
            }
            else
            {
                buzzer.playTone(MemoryGameConfig::START_TONE_FREQUENCY, MemoryGameConfig::START_TONE_DURATION);
                startAnimPhase = StartAnimPhase::Done;
                lastActionTime = millis();
            }
        }
        return false;
    case StartAnimPhase::Done:
        return hasElapsed(lastActionTime, MemoryGameConfig::START_TONE_DURATION);
    }
    return false; // Fallback
}

/**
 * @brief Starts the game by initializing variables and generating the first sequence
 * 
 * Sets up the initial game state, generates the first sequence, and starts the game animation.
 */
void MemoryGame::startGame()
{
    challengeComplete = false;
    level = 1;
    seqLength = getSequenceLengthForLevel(level);
    userIndex = 0;
    lastButtonPressed = MemoryGameConfig::NO_BUTTON_PRESSED;
    lastPressTime = 0;

    generateSequence(seqLength);
    resetSequenceDisplay();
    update7SegmentDisplay();
    startAnimPhase = StartAnimPhase::Idle;
    setState(MemoryGameState::StartAnimation);
}

/**
 * @brief Updates the sequence display
 * 
 * This method handles the display of the sequence to the user.
 * It turns LEDs on and off according to the sequence and timing parameters.
 */
void MemoryGame::updateSequenceDisplay()
{
    if (seqDisplayIndex < seqLength)
    {
        if (seqPhase == SeqPhase::LedOn)
        {
            if (!displayStarted)
            {
                uint16_t ledMask = (1 << sequence[seqDisplayIndex]) << MemoryGameConfig::LED_SHIFT_AMOUNT;
                whadda.setLEDs(ledMask);
                buzzer.playTone(Frequencies::ledFrequencies[sequence[seqDisplayIndex]], MemoryGameConfig::TONE_DURATION_SEQUENCE);
                lastActionTime = millis();
                displayStarted = true;
            }
            else if (hasElapsed(lastActionTime, MemoryGameConfig::LED_ON_TIME))
            {
                whadda.clearLEDs();
                lastActionTime = millis();
                displayStarted = false;
                seqPhase = SeqPhase::LedOff;
            }
        }
        else
        { // SeqPhase::LedOff
            if (hasElapsed(lastActionTime, MemoryGameConfig::LED_OFF_TIME))
            {
                seqDisplayIndex++;
                seqPhase = SeqPhase::LedOn;
            }
        }
    }
    else
    {
        whadda.clearLEDs();
        userIndex = 0;
        update7SegmentDisplay();
        setState(MemoryGameState::GetUserInput);
    }
}


/**
 * @brief Checks for user input and processes it
 * 
 * This method reads the button inputs, handles debouncing, and checks if the input
 * matches the expected sequence. It provides visual and audio feedback based on the input.
 */
void MemoryGame::checkUserInput()
{
    uint8_t buttons = whadda.readButtonsWithDebounce();
    if (buttons == 0)
    {
        lastButtonPressed = MemoryGameConfig::NO_BUTTON_PRESSED;
        return;
    }

    for (int btnIndex = 0; btnIndex < MemoryGameConfig::NUM_LEDS; btnIndex++)
    {
        if (buttons & (1 << btnIndex))
        {
            unsigned long now = millis();
            if (btnIndex == lastButtonPressed && (now - lastPressTime) < MemoryGameConfig::BUTTON_DEBOUNCE_DELAY)
                return;

            lastPressTime = now;
            lastButtonPressed = btnIndex;

            if (btnIndex == sequence[userIndex])
            {
                buzzer.playTone(Frequencies::ledFrequencies[btnIndex], MemoryGameConfig::TONE_DURATION_INPUT);
                whadda.setLEDs((1 << btnIndex) << MemoryGameConfig::LED_SHIFT_AMOUNT);
                inputDelayStart = now;
                userIndex++;

                if (userIndex >= seqLength)
                    setState(MemoryGameState::RoundWinCheck);
                else
                    setState(MemoryGameState::WaitInputDelay);
            }
            else
            {
                displayErrorFeedback();
                errorDelayStart = now;
                setState(MemoryGameState::Error);
            }
            break; // Process one button press at a time
        }
    }
}

//
// Visual Feedback Helpers
//

/**
 * @brief Displays error feedback to the user
 * 
 * Plays an error tone and displays an error message.
 */
void MemoryGame::displayErrorFeedback()
{
    buzzer.playTone(MemoryGameConfig::ERROR_TONE_FREQUENCY, MemoryGameConfig::ERROR_TONE_DURATION);
    whadda.displayText(MemoryGameConfig::ERROR_MESSAGE);
}

/**
 * @brief Displays success feedback to the user
 * 
 * Displays a success message and plays a win melody.
 */
void MemoryGame::displaySuccessFeedback()
{
    whadda.clearDisplay();
    whadda.displayText(MemoryGameConfig::SUCCESS_MESSAGE);
    buzzer.playWinMelody();
}

/**
 * @brief Displays the current level progress
 * 
 * Updates the 7-segment display to show the current level.
 */
void MemoryGame::displayLevelProgress()
{
    whadda.clearDisplay();
    update7SegmentDisplay();
}

/**
 * @brief Clears all visual feedback
 * 
 * Clears the display and turns off all LEDs.
 */
void MemoryGame::clearVisualFeedback()
{
    whadda.clearDisplay();
    whadda.clearLEDs();
}

//
// Round Win Check
//

/**
 * @brief Handles the round win condition
 * 
 * This method is called when the user successfully completes a round.
 * It checks if the game is complete or if the user should advance to the next level.
 */
void MemoryGame::handleRoundWin()
{
    rgbLed.off();
    if (level >= MemoryGameConfig::MAX_LEVEL)
    {
        displaySuccessFeedback();
        finishDelayStart = millis();
        setState(MemoryGameState::Finish);
    }
    else
    {
        level++;
        displayLevelProgress();
        levelDelayStart = millis();
        setState(MemoryGameState::WaitNextLevel);
    }
}


/**
 * @brief Main game loop method
 * 
 * This method is called repeatedly by the game engine.
 * It handles the game state machine, user input, and visual feedback.
 * 
 * @return true when the game is complete, false otherwise
 */
bool MemoryGame::run()
{
    if (!challengeInitialized)
        init();
    if (challengeComplete)
        return true;

    switch (currentState)
    {
    case MemoryGameState::Idle:
        break;
    case MemoryGameState::Init:
        startGame();
        break;
    case MemoryGameState::StartAnimation:
        if (updateStartAnimation())
        {
            lcd.clear();
            showTimer = true;
            clearVisualFeedback();
            setState(MemoryGameState::Pause);
        }
        break;
    case MemoryGameState::Pause:
        if (hasElapsed(lastStateChangeTime, MemoryGameConfig::STARTING_PAUSE_DELAY))
        {
            resetSequenceDisplay();
            setState(MemoryGameState::DisplaySequence);
        }
        break;
    case MemoryGameState::DisplaySequence:
        updateSequenceDisplay();
        break;
    case MemoryGameState::GetUserInput:
        checkUserInput();
        break;
    case MemoryGameState::WaitInputDelay:
        if (hasElapsed(inputDelayStart, MemoryGameConfig::INPUT_FEEDBACK_TIME))
        {
            whadda.clearLEDs();
            setState(MemoryGameState::GetUserInput);
        }
        break;
    case MemoryGameState::Error:
        rgbLed.blinkColor(255, 0, 0, 3);
        showTimer = false;
        lcd.setCursor(0, 0);
        lcd.print(MemoryGameConfig::WATCH_CAREFULLY_MESSAGE);
        if (hasElapsed(errorDelayStart, MemoryGameConfig::ERROR_DISPLAY_TIME))
        {
            lcd.clear();
            showTimer = true;
            clearVisualFeedback();
            resetSequenceDisplay();
            update7SegmentDisplay();
            rgbLed.off();
            setState(MemoryGameState::DisplaySequence);
        }
        break;
    case MemoryGameState::RoundWinCheck:
        handleRoundWin();
        break;
    case MemoryGameState::WaitNextLevel:
        if (hasElapsed(inputDelayStart, MemoryGameConfig::INPUT_FEEDBACK_TIME))
            whadda.clearLEDs();
        if (hasElapsed(levelDelayStart, MemoryGameConfig::ROUND_CONFIG_BASE_DELAY))
        {
            seqLength = getSequenceLengthForLevel(level);
            generateSequence(seqLength);
            resetSequenceDisplay();
            update7SegmentDisplay();
            setState(MemoryGameState::DisplaySequence);
        }
        break;
    case MemoryGameState::Finish:
        if (hasElapsed(finishDelayStart, MemoryGameConfig::FINISH_DISPLAY_TIME))
        {
            clearVisualFeedback();
            challengeComplete = true;
            setState(MemoryGameState::Idle);
        }
        break;
    }
    return challengeComplete;
}

/**
 * @brief Global instance of the MemoryGame
 */
MemoryGame memoryGame;

/**
 * @brief Global run function that delegates to the MemoryGame instance
 * 
 * @return true when the game is complete, false otherwise
 */
bool run()
{
    return memoryGame.run();
}
