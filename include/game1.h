#ifndef MEMORY_GAME_H
#define MEMORY_GAME_H

#include "BaseGame.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Configuration Constants for MemoryGame
// ---------------------------------------------------------------------------
namespace MemoryGameConfig
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
    constexpr int ledFrequencies[MemoryGameConfig::NUM_LEDS] = {220, 262, 294, 330, 349, 392, 440, 494};
}

// ---------------------------------------------------------------------------
// Enumerations for the State Machine
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
// MemoryGame Class Definition (inherits from BaseGame)
// ---------------------------------------------------------------------------
class MemoryGame : public BaseGame
{
public:
    MemoryGame();
    void init();
    bool run() override; // Called repeatedly; returns true when the challenge is complete

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
    int sequence[MemoryGameConfig::MAX_SEQUENCE_SIZE];
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

#endif // MEMORY_GAME_H
