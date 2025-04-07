#ifndef MEMORY_GAME_H
#define MEMORY_GAME_H

#include "BaseGame.h"
#include <Arduino.h>

/**
 * @brief Configuration constants for the Memory Game.
 *
 * This namespace contains all the configuration parameters for the Memory Game,
 * including maximum levels, timing constants, LED configurations, and tone parameters.
 * These constants are used throughout the game logic to control the behavior of the game.
 */
namespace MemoryGameConfig
{
    /** @brief Maximum level that can be reached in the game */
    constexpr int MAX_LEVEL = 8;
    
    /** @brief Initial sequence length at level 1 */
    constexpr int INITIAL_SEQUENCE_LENGTH = 2;
    
    /** @brief Maximum size of the sequence array */
    constexpr int MAX_SEQUENCE_SIZE = 32;
    
    /** @brief Number of LEDs/buttons in the game */
    constexpr int NUM_LEDS = 8;
    
    /** @brief Sequence length for levels 3-4 */
    constexpr int LEVEL_2_SEQUENCE_LENGTH = 3;
    
    /** @brief Sequence length for levels 5-6 */
    constexpr int LEVEL_4_SEQUENCE_LENGTH = 4;
    
    /** @brief Sequence length for levels 7-8 */
    constexpr int LEVEL_6_SEQUENCE_LENGTH = 5;
    
    /** @brief Threshold for level 2 sequence length */
    constexpr int LEVEL_2_THRESHOLD = 2;
    
    /** @brief Threshold for level 4 sequence length */
    constexpr int LEVEL_4_THRESHOLD = 4;
    
    /** @brief Threshold for level 6 sequence length */
    constexpr int LEVEL_6_THRESHOLD = 6;
    
    /** @brief Amount to shift LED bits for proper positioning */
    constexpr int LED_SHIFT_AMOUNT = 8;
    
    /** @brief Mask for all LEDs (0xFF) */
    constexpr int ALL_LEDS_MASK = 0xFF;
    
    /** @brief Value indicating no button is pressed */
    constexpr int NO_BUTTON_PRESSED = -1;
    
    /** @brief Number of blinks required in the start animation */
    constexpr int REQUIRED_BLINKS = 2;

    // Timing constants (ms)
    /** @brief Duration for which LEDs stay on during sequence display */
    constexpr unsigned long LED_ON_TIME = 300;
    
    /** @brief Duration for which LEDs stay off between sequence elements */
    constexpr unsigned long LED_OFF_TIME = 200;
    
    /** @brief Duration for which feedback is shown after user input */
    constexpr unsigned long INPUT_FEEDBACK_TIME = 200;
    
    /** @brief Duration for which error state is displayed */
    constexpr unsigned long ERROR_DISPLAY_TIME = 1000;
    
    /** @brief Duration for which finish state is displayed */
    constexpr unsigned long FINISH_DISPLAY_TIME = 1000;
    
    /** @brief Delay for button debouncing */
    constexpr unsigned long BUTTON_DEBOUNCE_DELAY = 200;
    
    /** @brief Base delay between rounds */
    constexpr unsigned long ROUND_CONFIG_BASE_DELAY = 1000;
    
    /** @brief Delay before starting the game */
    constexpr unsigned long STARTING_PAUSE_DELAY = 250;

    // Tone parameters
    /** @brief Duration of tone during sequence display */
    constexpr unsigned long TONE_DURATION_SEQUENCE = 200;
    
    /** @brief Duration of tone during user input */
    constexpr unsigned long TONE_DURATION_INPUT = 150;
    
    /** @brief Duration of error tone */
    constexpr unsigned long ERROR_TONE_DURATION = 700;
    
    /** @brief Frequency of error tone */
    constexpr int ERROR_TONE_FREQUENCY = 300;
    
    /** @brief Frequency of start tone */
    constexpr int START_TONE_FREQUENCY = 1200;
    
    /** @brief Duration of start tone */
    constexpr int START_TONE_DURATION = 300;

    // Start animation blink interval
    /** @brief Interval between blinks in the start animation */
    constexpr unsigned long START_ANIM_INTERVAL = 200;
    
    // Display messages
    /** @brief Title displayed at the start of the game */
    constexpr const char* GAME_TITLE = "Memory Mole!";
    
    /** @brief Message displayed at the start of the game */
    constexpr const char* GOOD_LUCK_MESSAGE = "Good luck";
    
    /** @brief Message displayed when user makes a mistake */
    constexpr const char* ERROR_MESSAGE = "ERROR!";
    
    /** @brief Message displayed when user completes the game */
    constexpr const char* SUCCESS_MESSAGE = "GOOD";
    
    /** @brief Message displayed after an error */
    constexpr const char* WATCH_CAREFULLY_MESSAGE = "Watch carefully!";
}

/**
 * @brief Frequencies for the tones used in the game.
 *
 * This namespace contains the frequency values for each LED in the game.
 * These frequencies are used to generate tones corresponding to the LEDs.
 */
namespace Frequencies
{
    /** @brief Array of frequencies for each LED, in ascending order */
    constexpr int ledFrequencies[MemoryGameConfig::NUM_LEDS] = {220, 262, 294, 330, 349, 392, 440, 494};
}

/**
 * @brief Game states for the Memory Game.
 *
 * This enum class defines the various states that the game can be in.
 * Each state represents a specific phase of the game, such as initialization,
 * displaying sequences, waiting for user input, and handling errors.
 */
enum class MemoryGameState
{
    /** @brief Initial state, game not started */
    Idle,
    
    /** @brief Game initialization state */
    Init,
    
    /** @brief Start animation state */
    StartAnimation,
    
    /** @brief Pause before sequence display */
    Pause,
    
    /** @brief Displaying the sequence to the user */
    DisplaySequence,
    
    /** @brief Waiting for user input */
    GetUserInput,
    
    /** @brief Waiting after user input before accepting next input */
    WaitInputDelay,
    
    /** @brief Error state after incorrect input */
    Error,
    
    /** @brief Checking if round is won */
    RoundWinCheck,
    
    /** @brief Waiting before starting next level */
    WaitNextLevel,
    
    /** @brief Game completed state */
    Finish
};

/**
 * @brief Sequence phases for the Memory Game.
 *
 * This enum class defines the phases of the sequence display.
 * Each phase represents a specific action, such as turning on or off the LEDs.
 * These phases are used to control the timing and behavior of the LED sequence.
 */
enum class SeqPhase
{
    /** @brief LED is on during sequence display */
    LedOn,
    
    /** @brief LED is off between sequence elements */
    LedOff
};

/**
 * @brief Start animation phases for the Memory Game.
 *
 * This enum class defines the phases of the start animation.
 * Each phase represents a specific action, such as blinking LEDs on or off.
 * These phases are used to control the timing and behavior of the start animation.
 */
enum class StartAnimPhase
{
    /** @brief Initial state of start animation */
    Idle,
    
    /** @brief LEDs are on during blink */
    BlinkOn,
    
    /** @brief LEDs are off between blinks */
    BlinkOff,
    
    /** @brief Animation is complete */
    Done
};

/**
 * @brief MemoryGame class for the memory mole game challenge.
 *
 * This class implements the game logic, including state management,
 * sequence generation, user input handling, and display updates.
 * It inherits from the BaseGame class to provide a common interface for all games.
 * 
 * The game works as follows:
 * 1. A sequence of LEDs is displayed to the user
 * 2. The user must repeat the sequence by pressing the corresponding buttons
 * 3. If correct, the sequence length increases for the next round
 * 4. If incorrect, the user must try again
 * 5. The game continues until the user reaches the maximum level or fails
 */
class MemoryGame : public BaseGame
{
public:
    /**
     * @brief Default constructor for MemoryGame.
     * 
     * Initializes all member variables to their default values.
     */
    MemoryGame();
    
    /**
     * @brief Initializes the game.
     * 
     * Sets up the game state, displays the initial message, and prepares for the game to start.
     * This method is called once at the beginning of the game.
     */
    void init();
    
    /**
     * @brief Main game loop method.
     * 
     * This method is called repeatedly by the game engine.
     * It handles the game state machine, user input, and visual feedback.
     * 
     * @return true when the game is complete, false otherwise
     */
    bool run() override;

private:
    // State variables
    /** @brief Current state of the game */
    MemoryGameState currentState;
    
    /** @brief Current phase of sequence display */
    SeqPhase seqPhase;
    
    /** @brief Current phase of start animation */
    StartAnimPhase startAnimPhase;

    /** @brief Whether the game has been initialized */
    bool challengeInitialized;
    
    /** @brief Whether the game is complete */
    bool challengeComplete;
    
    /** @brief Whether the sequence display has started */
    bool displayStarted;

    /** @brief Current level of the game */
    int level;
    
    /** @brief Length of the current sequence */
    int seqLength;
    
    /** @brief Array storing the current sequence */
    int sequence[MemoryGameConfig::MAX_SEQUENCE_SIZE];
    
    /** @brief Current index in the user's input sequence */
    int userIndex;
    
    /** @brief Last button pressed by the user */
    int lastButtonPressed;
    
    /** @brief Count of blinks in the start animation */
    int blinkCount;
    
    /** @brief Mask for the LEDs in the start animation */
    uint16_t blinkMask;

    // Timing variables
    /** @brief Time of the last state change */
    unsigned long lastStateChangeTime;
    
    /** @brief Time of the last action */
    unsigned long lastActionTime;
    
    /** @brief Time of the last button press */
    unsigned long lastPressTime;
    
    /** @brief Start time of input delay */
    unsigned long inputDelayStart;
    
    /** @brief Start time of error display */
    unsigned long errorDelayStart;
    
    /** @brief Start time of finish display */
    unsigned long finishDelayStart;
    
    /** @brief Start time of level delay */
    unsigned long levelDelayStart;

    /** @brief Current index in the sequence display */
    int seqDisplayIndex;

    // Private helper functions
    /**
     * @brief Sets the game state and updates the state change time.
     * 
     * @param newState The new state to set
     */
    void setState(MemoryGameState newState);
    
    /**
     * @brief Resets the sequence display variables.
     */
    void resetSequenceDisplay();
    
    /**
     * @brief Gets the sequence length for a given level.
     * 
     * @param lvl The level to get the sequence length for
     * @return The sequence length for the given level
     */
    int getSequenceLengthForLevel(int lvl) const;
    
    /**
     * @brief Updates the 7-segment display to show the current level.
     */
    void update7SegmentDisplay() const;
    
    /**
     * @brief Generates a random sequence of the given length.
     * 
     * @param length The length of the sequence to generate
     */
    void generateSequence(int length);
    
    /**
     * @brief Updates the start animation.
     * 
     * @return true when the animation is complete, false otherwise
     */
    bool updateStartAnimation();
    
    /**
     * @brief Starts the game by initializing variables and generating the first sequence.
     */
    void startGame();
    
    /**
     * @brief Updates the sequence display.
     */
    void updateSequenceDisplay();
    
    /**
     * @brief Checks for user input and processes it.
     */
    void checkUserInput();
    
    /**
     * @brief Handles the round win condition.
     */
    void handleRoundWin();
    
    // Helper methods for visual feedback
    /**
     * @brief Displays error feedback to the user.
     */
    void displayErrorFeedback();
    
    /**
     * @brief Displays success feedback to the user.
     */
    void displaySuccessFeedback();
    
    /**
     * @brief Displays the current level progress.
     */
    void displayLevelProgress();
    
    /**
     * @brief Clears all visual feedback.
     */
    void clearVisualFeedback();
};

#endif
