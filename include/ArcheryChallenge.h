#ifndef ARCHERY_GAME3_H
#define ARCHERY_GAME3_H

#include "BaseGame.h"
#include "Pins.h"

/**
 * @brief Configuration constants for the Archery Challenge game.
 */
namespace ArcheryConfig
{
    constexpr int TOTAL_ROUNDS = 3;
    constexpr int ARROWS_PER_ROUND = 3;

    // Tolerance for hitting target (target "size") per round
    constexpr int TOLERANCE_ROUND1 = 110;
    constexpr int TOLERANCE_ROUND2 = 80;
    constexpr int TOLERANCE_ROUND3 = 40;

    // Timing constants (in milliseconds)
    constexpr unsigned long INTRO_DURATION = 1500;           // Duration of intro message display
    constexpr unsigned long FEEDBACK_DURATION = 1000;        // Duration to show feedback for a missed shot
    constexpr unsigned long SUCCESS_DISPLAY_DURATION = 1000; // Duration to display "Hit!" feedback
    constexpr unsigned long RESTART_EFFECT_DURATION = 1500;  // Duration of "Out of arrows" effect
    constexpr unsigned long RESTART_BLINK_INTERVAL = 200;    // Blink interval for restart effect LEDs
    constexpr unsigned long RETRY_DURATION = 1000;           // Pause before restarting challenge

    // Magical effect timing
    constexpr unsigned long SHIELD_UP_MS = 500;        // Shield active duration (when target invulnerable)
    constexpr unsigned long SHIELD_DOWN_MS = 1500;     // Shield inactive duration (target vulnerable)
    constexpr unsigned long TARGET_VISIBLE_MS = 1000;  // Duration target stays visible (for disappearing effect)
    constexpr unsigned long TARGET_INVISIBLE_MS = 500; // Duration target disappears (for disappearing effect)
    
    // Hardware constants
    constexpr int POT_MIN_RAW = 300;      // Minimum raw potentiometer value
    constexpr int POT_MAX_RAW = 1023;     // Maximum raw potentiometer value
    constexpr int POT_MIN_MAPPED = 0;     // Minimum mapped potentiometer value
    constexpr int POT_MAX_MAPPED = 1023;  // Maximum mapped potentiometer value
    constexpr int TARGET_MIN_SAFE = 100;  // Minimum safe target value
    constexpr int TARGET_MAX_SAFE = 923;  // Maximum safe target value
    
    // LED constants
    constexpr int TARGET_INDICATOR_LED = 7;  // LED index for target visibility indicator
    constexpr int MAX_LEDS = 8;              // Total number of LEDs on the display
    
    // Sound constants
    constexpr int ARROW_FIRE_FREQ = 400;     // Frequency for arrow firing sound
    constexpr int ARROW_FIRE_DURATION = 50;  // Duration for arrow firing sound
    constexpr int HIT_FREQ_1 = 1000;         // First frequency for hit sound
    constexpr int HIT_FREQ_2 = 1200;         // Second frequency for hit sound
    constexpr int HIT_DURATION = 150;        // Duration for hit sound
    constexpr int MISS_FREQ = 300;           // Frequency for miss sound
    constexpr int MISS_DURATION = 150;       // Duration for miss sound
    constexpr int SHIELD_BLOCK_FREQ = 1000;  // Frequency for shield block sound
    constexpr int SHIELD_BLOCK_DURATION = 100; // Duration for shield block sound
    constexpr int FAIL_FREQ = 200;           // Frequency for failure sound
    constexpr int FAIL_DURATION = 500;       // Duration for failure sound
}

/**
 * @brief Possible magical effects that can occur in a round.
 */
enum class ArcheryEffect
{
    Winds,     // Shifting Winds: target aim shifts after each arrow
    Disappear, // Disappearing Target: target blinks in and out
    Shield     // Magic Shield: target periodically invulnerable
};

/**
 * @brief Internal states for managing a single round attempt.
 */
enum class RoundAttemptState
{
    Init,    // Initialize round (setup target, effect, etc.)
    Playing, // Waiting for player to aim and shoot
    Feedback // Showing feedback after a shot (miss) before continuing
};

/**
 * @brief Main state machine states for the Archery Challenge game.
 */
enum class ArcheryState
{
    Init,
    Intro,
    WaitIntro,
    GameLoop,
    ProcessRound,
    RoundSuccess,
    RestartEffect,
    Retry,
    Finished
};

/**
 * @brief Archery Challenge game class (Challenge 3).
 *
 * The player is a legendary archer in a magical tournament with 3 rounds.
 * Each round they must hit a target with up to 3 arrows. Incorporates magical twists.
 */
class ArcheryChallenge : public BaseGame
{
public:
    /**
     * @brief Constructor for the Archery Challenge game.
     * 
     * Initializes all game state variables to their default values.
     */
    ArcheryChallenge();
    
    /**
     * @brief Initializes the game state and hardware.
     * 
     * Sets up the game for a fresh start, including resetting all state variables,
     * clearing displays, and playing the start melody.
     */
    void init();
    
    /**
     * @brief Runs one iteration of the game loop.
     * 
     * This is the main game loop that handles state transitions and game logic.
     * 
     * @return true when the challenge is complete, false otherwise
     */
    bool run() override;

private:
    // Game state tracking
    ArcheryState state;
    int currentRound;
    bool roundResult; // Result of the last round (hit or fail)

    unsigned long stateStart; // Timestamp of state start (for timing)

    // Round attempt tracking
    RoundAttemptState roundState;
    int arrowCount; // Arrows used in the current round

    // Current round parameters
    ArcheryEffect currentEffect;
    int targetValue; // The "correct" target aim value for the round
    int tolerance;   // Allowed deviation for a hit

    // Magical effect state variables
    bool shieldActive;
    bool targetVisible;
    unsigned long lastShieldToggle;
    unsigned long lastEffectToggle; // Used for target visibility blink timing (and winds if needed)
    unsigned long feedbackStart;

    bool prevButtonState; // To detect button press events

    /**
     * @brief Updates the current round attempt state machine.
     *
     * Handles the logic for a single round, including aiming, shooting, and feedback.
     *
     * @param roundLevel The current round number (1-indexed).
     * @return true if the round ended (target hit or out of arrows), false if still in progress.
     */
    bool updateRoundAttempt(int roundLevel);
    
    /**
     * @brief Sets the Whadda LED display to indicate a given number of arrows remaining.
     *
     * Lights the first N LEDs (out of 8) to represent arrows left.
     * Turns off the rest.
     *
     * @param arrowsLeft Number of arrows remaining to display.
     */
    void setWhaddaArrows(int arrowsLeft);
    
    /**
     * @brief Runs the "out of arrows" restart effect.
     *
     * Blinks all LEDs on the Whadda display and shows a message indicating the challenge will restart.
     */
    void runRestartEffect();
    
    /**
     * @brief Displays a message on the LCD screen.
     *
     * @param line1 First line of text to display
     * @param line2 Second line of text to display (optional)
     * @param hideTimer Whether to hide the timer during this message
     */
    void displayLcdMessage(const char* line1, const char* line2 = nullptr, bool hideTimer = false);
    
    /**
     * @brief Displays round information on the LCD.
     *
     * @param roundLevel Current round number
     */
    void displayRoundInfo(int roundLevel);
    
    /**
     * @brief Displays information about the current magical effect.
     *
     * @param effect The current magical effect
     */
    void displayEffectInfo(ArcheryEffect effect);
    
    /**
     * @brief Displays feedback for a successful hit.
     *
     * @param roundLevel Current round number
     */
    void displayHitFeedback(int roundLevel);
    
    /**
     * @brief Displays feedback for a missed shot.
     *
     * @param shieldBlocked Whether the shot was blocked by a shield
     * @param targetVisible Whether the target was visible when shot
     * @param potValue The potentiometer value when the shot was fired
     */
    void displayMissFeedback(bool shieldBlocked, bool targetVisible, int potValue);
    
    /**
     * @brief Displays the retry message when out of arrows.
     */
    void displayRetryMessage();
    
    /**
     * @brief Displays the completion message when all rounds are finished.
     */
    void displayFinishedMessage();
    
    /**
     * @brief Resets the game state to initial values.
     */
    void resetGameState();
    
    /**
     * @brief Resets the round state to initial values.
     */
    void resetRoundState();
    
    /**
     * @brief Gets the tolerance value for a specific round.
     *
     * @param roundLevel The round number (1-indexed)
     * @return The tolerance value for the specified round
     */
    int getToleranceForRound(int roundLevel);
    
    /**
     * @brief Generates a random target value within safe limits.
     *
     * @return A random target value between TARGET_MIN_SAFE and TARGET_MAX_SAFE
     */
    int generateRandomTarget();
    
    /**
     * @brief Selects a random magical effect for the round.
     *
     * @return A randomly selected ArcheryEffect
     */
    ArcheryEffect selectRandomEffect();
    
    /**
     * @brief Checks if the button is currently pressed.
     *
     * @return true if the button is pressed, false otherwise
     */
    bool isButtonPressed();
    
    /**
     * @brief Reads and maps the potentiometer value.
     *
     * @return The mapped potentiometer value
     */
    int readPotentiometer();
    
    /**
     * @brief Checks if a shot hit the target.
     *
     * @param potValue The potentiometer value when the shot was fired
     * @return true if the shot hit the target, false otherwise
     */
    bool checkHit(int potValue);
    
    /**
     * @brief Handles the shield effect logic.
     *
     * Toggles the shield on and off based on time intervals.
     *
     * @param now Current time in milliseconds
     */
    void handleShieldEffect(unsigned long now);
    
    /**
     * @brief Handles the disappearing target effect logic.
     *
     * Toggles target visibility at set intervals.
     *
     * @param now Current time in milliseconds
     */
    void handleDisappearEffect(unsigned long now);
    
    /**
     * @brief Processes an arrow being fired.
     *
     * Increments the arrow count and plays the firing sound.
     */
    void fireArrow();
    
    /**
     * @brief Updates the arrows display on the Whadda.
     */
    void updateArrowsDisplay();
    
    /**
     * @brief Plays the sound for a successful hit.
     */
    void playHitSound();
    
    /**
     * @brief Plays the sound for a missed shot.
     */
    void playMissSound();
    
    /**
     * @brief Plays the sound for a shot blocked by a shield.
     */
    void playShieldBlockSound();
    
    /**
     * @brief Plays the sound for a failed round.
     */
    void playFailSound();
    
    /**
     * @brief Sets the RGB LED to indicate success (green).
     */
    void setSuccessLed();
    
    /**
     * @brief Sets the RGB LED to indicate failure (red).
     */
    void setMissLed();
    
    /**
     * @brief Sets the RGB LED to indicate shield active (blue).
     */
    void setShieldLed();

};

#endif // ARCHERY_GAME3_H
