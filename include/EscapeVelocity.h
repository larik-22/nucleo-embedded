#ifndef MEMORY_GAME2_H
#define MEMORY_GAME2_H

#include "BaseGame.h"
#include "Pins.h"

/**
 * @brief Configuration namespace for the Escape Velocity game
 * 
 * Contains all configurable parameters for the game, including timing,
 * difficulty settings, and visual feedback parameters.
 */
namespace EscVelocityConfig
{
    // Game parameters
    constexpr int TOTAL_GATES = 5;                // Number of gates to pass
    constexpr int STARTING_LIVES = 3;             // Initial lives
    constexpr unsigned long GATE_TIME_MS = 10000; // Allowed time per gate
    constexpr unsigned long IN_RANGE_MS = 2500;   // Must stay in range for this duration
    constexpr unsigned long BEEP_INTERVAL = 250;  // Gap between short beeps (ms)
    constexpr float ALPHA = 0.1f;                 // Exponential smoothing factor
    constexpr int TOLERANCE = 10;                 // Threshold to reduce flicker issues

    // Potentiometer configuration
    constexpr int POT_MIN_RAW = 300;              // Minimum raw potentiometer value
    constexpr int POT_MAX_RAW = 1023;             // Maximum raw potentiometer value
    constexpr int POT_MIN_MAPPED = 0;             // Minimum mapped potentiometer value
    constexpr int POT_MAX_MAPPED = 1023;          // Maximum mapped potentiometer value
    constexpr int POT_MIN_POSSIBLE = 100;         // Minimum possible value after scaling

    // Velocity range generation parameters
    constexpr int BASE_MAX = 1023;                // Base maximum value for velocity range
    constexpr int BASE_WINDOW = 30;               // Base window size for velocity range
    constexpr float WINDOW_FACTOR_MULTIPLIER = 2.0f; // Multiplier for window factor
    constexpr int RANDOM_OFFSET_MIN = 50;         // Minimum random offset
    constexpr int RANDOM_OFFSET_PADDING = 50;     // Padding for random offset

    // Visual feedback parameters
    constexpr int IN_RANGE_TONE_FREQ = 800;       // Frequency for in-range tone
    constexpr int IN_RANGE_TONE_DURATION = 50;    // Duration for in-range tone
    constexpr int OUT_OF_RANGE_TONE_FREQ = 300;   // Frequency for out-of-range tone
    constexpr int OUT_OF_RANGE_TONE_DURATION = 200; // Duration for out-of-range tone
    constexpr int SUCCESS_TONE1_FREQ = 1000;      // First success tone frequency
    constexpr int SUCCESS_TONE1_DURATION = 150;   // First success tone duration
    constexpr int SUCCESS_TONE2_FREQ = 1200;      // Second success tone frequency
    constexpr int SUCCESS_TONE2_DURATION = 150;   // Second success tone duration
    constexpr int FAILED_TONE_FREQ = 200;         // Failed tone frequency
    constexpr int FAILED_TONE_DURATION = 500;     // Failed tone duration
    constexpr int LED_COUNT = 8;                  // Total number of LEDs
    constexpr int MAX_LIVES_LED = 3;              // Maximum number of LEDs for lives

    // State durations
    constexpr unsigned long INTRO_DURATION = 1500;
    constexpr unsigned long SUCCESS_BEEP_DURATION = 300;
    constexpr unsigned long FAILED_PAUSE_DURATION = 1000;
    constexpr unsigned long RETRY_DURATION = 1000;
    constexpr unsigned long RESTART_EFFECT_DURATION = 1500;
    constexpr unsigned long RESTART_BLINK_INTERVAL = 200;
}

/**
 * @brief States for the gate attempt process
 * 
 * Represents the different states a gate attempt can be in.
 */
enum class GateAttemptState
{
    Init,   ///< Initial state when starting a gate attempt
    Loop    ///< Main loop state for processing the gate attempt
};

/**
 * @brief States for the overall game state machine
 * 
 * Represents the different states the game can be in.
 */
enum class EscVelocityState
{
    Init,           ///< Initial state when the game starts
    Intro,          ///< Introduction screen
    WaitIntro,      ///< Waiting for intro to finish
    GameLoop,       ///< Main game loop
    ProcessGate,    ///< Processing gate attempt result
    SuccessBeep,    ///< Playing success sound
    FailedPause,    ///< Pause after failing a gate
    RestartEffect,  ///< Visual effect when restarting
    Retry,          ///< Retry state after losing all lives
    Finished        ///< Game completed successfully
};

/**
 * @brief Escape Velocity game implementation
 * 
 * A game where the player must control a potentiometer to keep a value
 * within a specific range for a set duration to pass through gates.
 * The game gets progressively more difficult as the player advances.
 */
class EscapeVelocity : public BaseGame
{
public:
    /**
     * @brief Constructor for the Escape Velocity game
     * 
     * Initializes all game variables to their default values.
     */
    EscapeVelocity();
    
    /**
     * @brief Initialize the game
     * 
     * Sets up the game state, displays, and initializes all necessary variables.
     */
    void init();
    
    /**
     * @brief Run the game
     * 
     * Main game loop that handles state transitions and game logic.
     * 
     * @return true if the game has completed, false otherwise
     */
    bool run() override;

private:
    // Overall game state
    EscVelocityState state;      ///< Current game state
    int currentGate;             ///< Current gate number (1-based)
    int lives;                   ///< Remaining lives
    unsigned long stateStart;    ///< Timestamp when current state started
    bool gateResult;             ///< Result of the last gate attempt
    bool showTimerFlag;          ///< Whether to show the timer

    // Gate attempt state variables
    GateAttemptState gateState;  ///< Current gate attempt state
    int minVel;                  ///< Minimum velocity for current gate
    int maxVel;                  ///< Maximum velocity for current gate
    unsigned long gateStart;     ///< Timestamp when current gate started
    unsigned long inRangeStart;  ///< Timestamp when player entered valid range
    unsigned long lastBeep;      ///< Timestamp of last beep
    bool wasOutOfRange;          ///< Whether player was out of range
    bool blinkState;             ///< Current blink state for visual feedback

    // Potentiometer smoothing filter
    float potFilter;             ///< Filtered potentiometer value

    // Potentiometer handling
    /**
     * @brief Get the smoothed potentiometer value
     * 
     * Reads the potentiometer, applies mapping and smoothing filter.
     * 
     * @param gateLevel Current gate level for scaling
     * @return Smoothed and scaled potentiometer value
     */
    int getSmoothedPotValue(int gateLevel);
    
    /**
     * @brief Initialize the potentiometer filter
     * 
     * Sets the initial filter value to the current potentiometer reading.
     */
    void initPotFilter();
    
    /**
     * @brief Check if a potentiometer value is within the valid range
     * 
     * @param potValue Current potentiometer value
     * @param minVel Minimum velocity threshold
     * @param maxVel Maximum velocity threshold
     * @return true if the value is within range, false otherwise
     */
    bool isPotInRange(int potValue, int minVel, int maxVel);
    
    // Velocity range generation
    /**
     * @brief Generate a velocity range for a gate
     * 
     * Creates a range of valid velocities that scales with gate level.
     * 
     * @param gateLevel Current gate level
     * @param minVel Output parameter for minimum velocity
     * @param maxVel Output parameter for maximum velocity
     */
    void generateVelocityRange(int gateLevel, int &minVel, int &maxVel);
    
    // Display updates
    /**
     * @brief Update the gate displays
     * 
     * Updates the LCD and Whadda displays with current gate information.
     * 
     * @param gateLevel Current gate level
     * @param potValue Current potentiometer value
     */
    void updateGateDisplays(int gateLevel, int potValue);
    
    /**
     * @brief Set the Whadda lives display
     * 
     * Updates the Whadda LED display to show remaining lives.
     * 
     * @param lives Number of lives to display
     */
    void setWhaddaLives(int lives);
    
    // Gate attempt update
    /**
     * @brief Update the current gate attempt
     * 
     * Processes the current gate attempt and updates state accordingly.
     * 
     * @param gateLevel Current gate level
     * @return true if the gate attempt is complete, false otherwise
     */
    bool updateGateAttempt(int gateLevel);
    
    // Game state transitions
    /**
     * @brief Handle gate success
     * 
     * Processes a successful gate attempt and transitions to the next state.
     */
    void handleGateSuccess();
    
    /**
     * @brief Handle gate failure
     * 
     * Processes a failed gate attempt, decrements lives, and transitions to the next state.
     */
    void handleGateFailure();
    
    /**
     * @brief Reset the game
     * 
     * Resets the game state after losing all lives.
     */
    void resetGame();
    
    // Restart effect helper
    /**
     * @brief Run the restart effect
     * 
     * Displays visual effects when restarting the game after losing all lives.
     */
    void runRestartEffect();
};

#endif
