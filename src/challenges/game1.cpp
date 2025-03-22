#include <Arduino.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Game1.h"

// ---------------------------------------------------------------------------
// External references
// ---------------------------------------------------------------------------
extern Buzzer buzzer;
extern RGBLed rgbLed;
extern Whadda whadda;

// ---------------------------------------------------------------------------
// Configuration Constants
// ---------------------------------------------------------------------------
namespace Config
{
    constexpr int MAX_LEVEL = 8;
    constexpr int INITIAL_SEQUENCE_LENGTH = 2;
    constexpr int MAX_SEQUENCE_SIZE = 32;
    constexpr int NUM_LEDS = 8;

    // Timing constants (ms)
    constexpr unsigned long LED_ON_TIME = 300;
    constexpr unsigned long LED_OFF_TIME = 200;
    constexpr unsigned long INPUT_FEEDBACK_TIME = 200;
    constexpr unsigned long ERROR_DISPLAY_TIME = 1000;
    constexpr unsigned long FINISH_DISPLAY_TIME = 1000;
    constexpr unsigned long BUTTON_DEBOUNCE_DELAY = 200;
    constexpr unsigned long ROUND_CONFIG_BASE_DELAY = 1000;
    constexpr unsigned long STARTING_PAUSE_DELAY = 250;

    // Tone parameters
    constexpr unsigned long TONE_DURATION_SEQUENCE = 200;
    constexpr unsigned long TONE_DURATION_INPUT = 150;
    constexpr unsigned long ERROR_TONE_DURATION = 700;
    constexpr int ERROR_TONE_FREQUENCY = 300;
    constexpr int START_TONE_FREQUENCY = 1200;
    constexpr int START_TONE_DURATION = 300;

    // Start animation blink interval
    constexpr unsigned long START_ANIM_INTERVAL = 200;
}

namespace Frequencies
{
    constexpr int ledFrequencies[Config::NUM_LEDS] = {220, 262, 294, 330, 349, 392, 440, 494};
}

// ---------------------------------------------------------------------------
// Enumerations for State Machine
// ---------------------------------------------------------------------------
enum class GameState
{
    Idle,
    Init,
    StartAnimation,
    Pause,
    DisplaySequence,
    GetUserInput,
    WaitInputDelay,
    Error,
    RoundWinCheck,
    WaitNextLevel,
    Finish
};

enum class SeqPhase
{
    LedOn,
    LedOff
};

enum class StartAnimPhase
{
    Idle,
    BlinkOn,
    BlinkOff,
    Done
};

// ---------------------------------------------------------------------------
// MemoryGame Class Definition
// ---------------------------------------------------------------------------
class MemoryGame
{
public:
    MemoryGame();
    void init();
    bool run(); // Called repeatedly; returns true when the challenge is complete

private:
    // State variables
    GameState currentState;
    SeqPhase seqPhase;
    StartAnimPhase startAnimPhase;

    bool challengeInitialized;
    bool challengeComplete;
    bool displayStarted;

    int level;
    int seqLength;
    int sequence[Config::MAX_SEQUENCE_SIZE];
    int userIndex;
    int lastButtonPressed;
    int blinkCount;
    const int requiredBlinks = 2;
    uint16_t blinkMask;

    // Timing variables
    unsigned long lastStateChangeTime;
    unsigned long lastActionTime;
    unsigned long lastPressTime;
    unsigned long inputDelayStart;
    unsigned long errorDelayStart;
    unsigned long finishDelayStart;
    unsigned long levelDelayStart;

    // Sequence display index
    int seqDisplayIndex;

    // Private helper functions
    void setState(GameState newState);
    bool hasElapsed(unsigned long start, unsigned long delay) const;
    void resetSequenceDisplay();
    int getSequenceLengthForLevel(int lvl) const;
    void update7SegmentDisplay() const;
    void generateSequence(int length);
    bool updateStartAnimation();
    void startGame();
    void updateSequenceDisplay();
    void checkUserInput();
    void handleRoundWin();
};

// ---------------------------------------------------------------------------
// MemoryGame Class Implementation
// ---------------------------------------------------------------------------
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
        challengeInitialized = true;
        challengeComplete = false;
        whadda.clearDisplay();
        setState(GameState::Init);
    }
}

bool MemoryGame::hasElapsed(unsigned long start, unsigned long delay) const
{
    return (millis() - start) >= delay;
}

void MemoryGame::setState(GameState newState)
{
    currentState = newState;
    lastStateChangeTime = millis();
}

void MemoryGame::resetSequenceDisplay()
{
    seqDisplayIndex = 0;
    seqPhase = SeqPhase::LedOn;
    displayStarted = false;
}

int MemoryGame::getSequenceLengthForLevel(int lvl) const
{
    if (lvl <= 2)
        return Config::INITIAL_SEQUENCE_LENGTH;
    else if (lvl <= 4)
        return 3;
    else if (lvl <= 6)
        return 4;
    else
        return 5;
}

void MemoryGame::update7SegmentDisplay() const
{
    int dotCount = (level <= Config::MAX_LEVEL) ? level : Config::MAX_LEVEL;
    for (uint8_t pos = 0; pos < Config::MAX_LEVEL; pos++)
    {
        whadda.display7Seg(pos, (pos < dotCount) ? 1 : 0);
    }
}

void MemoryGame::generateSequence(int length)
{
    if (length > Config::MAX_SEQUENCE_SIZE)
        length = Config::MAX_SEQUENCE_SIZE;

    sequence[0] = random(0, Config::NUM_LEDS);
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
                newVal = random(0, Config::NUM_LEDS);
            } while (newVal == sequence[i - 1]);
            sequence[i] = newVal;
        }
        else
        {
            sequence[i] = random(0, Config::NUM_LEDS);
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

bool MemoryGame::updateStartAnimation()
{
    // Non-blocking blink + beep animation; returns true when complete
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
        if (hasElapsed(lastActionTime, Config::START_ANIM_INTERVAL))
        {
            startAnimPhase = StartAnimPhase::BlinkOff;
            lastActionTime = millis();
        }
        return false;

    case StartAnimPhase::BlinkOff:
        whadda.clearLEDs();
        if (hasElapsed(lastActionTime, Config::START_ANIM_INTERVAL))
        {
            blinkCount++;
            if (blinkCount < requiredBlinks)
            {
                startAnimPhase = StartAnimPhase::BlinkOn;
                lastActionTime = millis();
            }
            else
            {
                buzzer.playTone(Config::START_TONE_FREQUENCY, Config::START_TONE_DURATION);
                startAnimPhase = StartAnimPhase::Done;
                lastActionTime = millis();
            }
        }
        return false;

    case StartAnimPhase::Done:
        return hasElapsed(lastActionTime, Config::START_TONE_DURATION);
    }
    return false; // fallback
}

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
                buzzer.playTone(Frequencies::ledFrequencies[sequence[seqDisplayIndex]], Config::TONE_DURATION_SEQUENCE);
                lastActionTime = millis();
                displayStarted = true;
            }
            else if (hasElapsed(lastActionTime, Config::LED_ON_TIME))
            {
                whadda.clearLEDs();
                lastActionTime = millis();
                displayStarted = false;
                seqPhase = SeqPhase::LedOff;
            }
        }
        else
        { // SeqPhase::LedOff
            if (hasElapsed(lastActionTime, Config::LED_OFF_TIME))
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

void MemoryGame::checkUserInput()
{
    uint8_t buttons = whadda.readButtonsWithDebounce();
    if (buttons == 0)
    {
        lastButtonPressed = -1;
        return;
    }

    for (int btnIndex = 0; btnIndex < Config::NUM_LEDS; btnIndex++)
    {
        if (buttons & (1 << btnIndex))
        {
            unsigned long now = millis();
            if (btnIndex == lastButtonPressed && (now - lastPressTime) < Config::BUTTON_DEBOUNCE_DELAY)
                return;

            lastPressTime = now;
            lastButtonPressed = btnIndex;

            if (btnIndex == sequence[userIndex])
            {
                buzzer.playTone(Frequencies::ledFrequencies[btnIndex], Config::TONE_DURATION_INPUT);
                whadda.setLEDs((1 << btnIndex) << 8);
                inputDelayStart = millis();
                userIndex++;

                if (userIndex >= seqLength)
                    setState(GameState::RoundWinCheck);
                else
                    setState(GameState::WaitInputDelay);
            }
            else
            {
                buzzer.playTone(Config::ERROR_TONE_FREQUENCY, Config::ERROR_TONE_DURATION);
                whadda.displayText("ERROR!");
                errorDelayStart = millis();
                setState(GameState::Error);
            }
            break; // process one button press at a time
        }
    }
}

void MemoryGame::handleRoundWin()
{
    if (level >= Config::MAX_LEVEL)
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

bool MemoryGame::run()
{
    // Ensure one-time initialization is done
    if (!challengeInitialized)
    {
        init();
    }
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
            whadda.clearDisplay();
            whadda.clearLEDs();
            setState(GameState::Pause);
        }
        break;

    case GameState::Pause:
        if (hasElapsed(lastStateChangeTime, Config::STARTING_PAUSE_DELAY))
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
        if (hasElapsed(inputDelayStart, Config::INPUT_FEEDBACK_TIME))
        {
            whadda.clearLEDs();
            setState(GameState::GetUserInput);
        }
        break;

    case GameState::Error:
        if (hasElapsed(errorDelayStart, Config::ERROR_DISPLAY_TIME))
        {
            whadda.clearDisplay();
            resetSequenceDisplay();
            update7SegmentDisplay();
            setState(GameState::DisplaySequence);
        }
        break;

    case GameState::RoundWinCheck:
        handleRoundWin();
        break;

    case GameState::WaitNextLevel:
        if (hasElapsed(inputDelayStart, Config::INPUT_FEEDBACK_TIME))
        {
            whadda.clearLEDs();
        }
        if (hasElapsed(levelDelayStart, Config::ROUND_CONFIG_BASE_DELAY))
        {
            seqLength = getSequenceLengthForLevel(level);
            generateSequence(seqLength);
            resetSequenceDisplay();
            update7SegmentDisplay();
            setState(GameState::DisplaySequence);
        }
        break;

    case GameState::Finish:
        if (hasElapsed(finishDelayStart, Config::FINISH_DISPLAY_TIME))
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

// ---------------------------------------------------------------------------
// Global instance and runner function
// ---------------------------------------------------------------------------
MemoryGame game;

bool runGame1()
{
    return game.run();
}
