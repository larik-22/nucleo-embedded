#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Game1.h"

// TODO: button class usage
// TODO: comments
// TODO: fix pot value being too close every time

// External hardware references (defined elsewhere)
extern Buzzer buzzer;
extern RGBLed rgbLed;
extern Whadda whadda;
extern LiquidCrystal_I2C lcd;

//
// Constructor and Initialization
//
MemoryGame::MemoryGame() : currentState(GameState::Idle),
                           seqPhase(SeqPhase::LedOn),
                           startAnimPhase(StartAnimPhase::Idle),
                           challengeInitialized(false),
                           challengeComplete(false),
                           displayStarted(false),
                           level(0),
                           seqLength(0),
                           userIndex(0),
                           lastButtonPressed(-1),
                           blinkCount(0),
                           blinkMask(0xFF),
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

void MemoryGame::init()
{
    if (!challengeInitialized)
    {
        buzzer.playRoundStartMelody();
        lcd.setCursor(0, 0);
        lcd.print("Memory Mole!");
        lcd.setCursor(0, 1);
        lcd.print("Good luck");
        challengeInitialized = true;
        challengeComplete = false;
        whadda.clearDisplay();
        setState(GameState::Init);
    }
}

//
// State Transition Helper
//
void MemoryGame::setState(GameState newState)
{
    currentState = newState;
    lastStateChangeTime = millis();
}

//
// Sequence Display Helpers
//
void MemoryGame::resetSequenceDisplay()
{
    seqDisplayIndex = 0;
    seqPhase = SeqPhase::LedOn;
    displayStarted = false;
}

int MemoryGame::getSequenceLengthForLevel(int lvl) const
{
    if (lvl <= 2)
        return MemoryGameConfig::INITIAL_SEQUENCE_LENGTH;
    else if (lvl <= 4)
        return 3;
    else if (lvl <= 6)
        return 4;
    else
        return 5;
}

void MemoryGame::update7SegmentDisplay() const
{
    int dotCount = (level <= MemoryGameConfig::MAX_LEVEL) ? level : MemoryGameConfig::MAX_LEVEL;
    for (uint8_t pos = 0; pos < MemoryGameConfig::MAX_LEVEL; pos++)
    {
        whadda.display7Seg(pos, (pos < dotCount) ? 1 : 0);
    }
}

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

//
// Start Animation (Non-blocking)
//
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
        whadda.setLEDs(blinkMask << 8);
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
            if (blinkCount < requiredBlinks)
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

//
// Game Start and Sequence Display
//
void MemoryGame::startGame()
{
    challengeComplete = false;
    level = 1;
    seqLength = getSequenceLengthForLevel(level);
    userIndex = 0;
    lastButtonPressed = -1;
    lastPressTime = 0;

    generateSequence(seqLength);
    resetSequenceDisplay();
    update7SegmentDisplay();

    startAnimPhase = StartAnimPhase::Idle;
    setState(GameState::StartAnimation);
}

void MemoryGame::updateSequenceDisplay()
{
    if (seqDisplayIndex < seqLength)
    {
        if (seqPhase == SeqPhase::LedOn)
        {
            if (!displayStarted)
            {
                uint16_t ledMask = (1 << sequence[seqDisplayIndex]) << 8;
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
        setState(GameState::GetUserInput);
    }
}

//
// User Input Handling
//
void MemoryGame::checkUserInput()
{
    uint8_t buttons = whadda.readButtonsWithDebounce();
    if (buttons == 0)
    {
        lastButtonPressed = -1;
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
                whadda.setLEDs((1 << btnIndex) << 8);
                inputDelayStart = now;
                userIndex++;

                if (userIndex >= seqLength)
                    setState(GameState::RoundWinCheck);
                else
                    setState(GameState::WaitInputDelay);
            }
            else
            {
                buzzer.playTone(MemoryGameConfig::ERROR_TONE_FREQUENCY, MemoryGameConfig::ERROR_TONE_DURATION);
                whadda.displayText("ERROR!");
                errorDelayStart = now;
                setState(GameState::Error);
            }
            break; // Process one button press at a time
        }
    }
}

//
// Round Win Check
//
void MemoryGame::handleRoundWin()
{
    rgbLed.off();
    if (level >= MemoryGameConfig::MAX_LEVEL)
    {
        whadda.clearDisplay();
        whadda.displayText("GOOD");
        buzzer.playWinMelody();
        finishDelayStart = millis();
        setState(GameState::Finish);
    }
    else
    {
        level++;
        whadda.clearDisplay();
        update7SegmentDisplay();
        levelDelayStart = millis();
        setState(GameState::WaitNextLevel);
    }
}

//
// Main Game Loop (run() method)
//
bool MemoryGame::run()
{
    if (!challengeInitialized)
        init();
    if (challengeComplete)
        return true;

    switch (currentState)
    {
    case GameState::Idle:
        break;
    case GameState::Init:
        startGame();
        break;
    case GameState::StartAnimation:
        if (updateStartAnimation())
        {
            lcd.clear();
            showTimer = true;
            whadda.clearDisplay();
            whadda.clearLEDs();
            setState(GameState::Pause);
        }
        break;
    case GameState::Pause:
        if (hasElapsed(lastStateChangeTime, MemoryGameConfig::STARTING_PAUSE_DELAY))
        {
            resetSequenceDisplay();
            setState(GameState::DisplaySequence);
        }
        break;
    case GameState::DisplaySequence:
        updateSequenceDisplay();
        break;
    case GameState::GetUserInput:
        checkUserInput();
        break;
    case GameState::WaitInputDelay:
        if (hasElapsed(inputDelayStart, MemoryGameConfig::INPUT_FEEDBACK_TIME))
        {
            whadda.clearLEDs();
            setState(GameState::GetUserInput);
        }
        break;
    case GameState::Error:
        rgbLed.blinkColor(255, 0, 0, 3);
        showTimer = false;
        lcd.setCursor(0, 0);
        lcd.print("Watch carefully!");
        if (hasElapsed(errorDelayStart, MemoryGameConfig::ERROR_DISPLAY_TIME))
        {
            lcd.clear();
            showTimer = true;
            whadda.clearDisplay();
            resetSequenceDisplay();
            update7SegmentDisplay();
            rgbLed.off();
            setState(GameState::DisplaySequence);
        }
        break;
    case GameState::RoundWinCheck:
        handleRoundWin();
        break;
    case GameState::WaitNextLevel:
        if (hasElapsed(inputDelayStart, MemoryGameConfig::INPUT_FEEDBACK_TIME))
            whadda.clearLEDs();
        if (hasElapsed(levelDelayStart, MemoryGameConfig::ROUND_CONFIG_BASE_DELAY))
        {
            seqLength = getSequenceLengthForLevel(level);
            generateSequence(seqLength);
            resetSequenceDisplay();
            update7SegmentDisplay();
            setState(GameState::DisplaySequence);
        }
        break;
    case GameState::Finish:
        if (hasElapsed(finishDelayStart, MemoryGameConfig::FINISH_DISPLAY_TIME))
        {
            whadda.clearDisplay();
            whadda.clearLEDs();
            challengeComplete = true;
            setState(GameState::Idle);
        }
        break;
    }
    return challengeComplete;
}

MemoryGame memoryGame;

bool run()
{
    return memoryGame.run();
}
