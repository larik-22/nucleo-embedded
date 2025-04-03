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
    constexpr unsigned long TARGET_VISIBLE_MS = 1500;  // Duration target stays visible (for disappearing effect)
    constexpr unsigned long TARGET_INVISIBLE_MS = 300; // Duration target disappears (for disappearing effect)
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
    ArcheryChallenge();
    void init();
    bool run() override; // Runs one iteration of the game loop; returns true when challenge is complete

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

    // Helper functions
    bool updateRoundAttempt(int roundLevel);
    void setWhaddaArrows(int arrowsLeft);
    void runRestartEffect();
};

#endif // ARCHERY_GAME3_H
