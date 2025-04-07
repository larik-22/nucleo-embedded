#ifndef RUNNER_GAME_H
#define RUNNER_GAME_H

#include "BaseGame.h"
#include <Arduino.h>

/**
 * @brief Configuration constants for the Runner Game.
 *
 * This namespace contains all the configuration parameters for the Runner Game,
 * including display settings, timing constants, and game mechanics.
 */
namespace RunnerGameConfig
{
    // Display settings
    constexpr int LCD_COLS = 16;
    constexpr int LCD_ROWS = 2;

    // Custom character IDs
    constexpr int LLAMA_STANDING_PART1_ID = 0;
    constexpr int LLAMA_STANDING_PART2_ID = 1;
    constexpr int LLAMA_RIGHT_FOOT_PART1_ID = 2;
    constexpr int LLAMA_RIGHT_FOOT_PART2_ID = 3;
    constexpr int LLAMA_LEFT_FOOT_PART1_ID = 4;
    constexpr int LLAMA_LEFT_FOOT_PART2_ID = 5;
    constexpr int CACTUS_PART1_ID = 6;
    constexpr int CACTUS_PART2_ID = 7;

    // Game mechanics
    constexpr int INITIAL_CACTUS_POS = 15;
    constexpr int GROUND_ROW = 1;
    constexpr int JUMP_ROW = 0;
    constexpr int MIN_JUMP_DURATION = 200;  // ms
    constexpr int NUM_OBSTACLE_TYPES = 3;   // 0 = bird, 1 = cactus type 1, 2 = cactus type 2
    constexpr int ANIMATION_INTERVAL = 150; // ms
    constexpr int ANIMATION_STATES = 3;     // Number of animation states (0, 1, 2)

    // Speed settings
    constexpr unsigned long INITIAL_GAME_INTERVAL = 200;    // ms
    constexpr unsigned long MIN_GAME_INTERVAL = 100;        // ms
    constexpr unsigned long SPEED_INCREASE_INTERVAL = 5000; // ms - increase speed every 5 seconds
    constexpr float SPEED_INCREASE_FACTOR = 0.9;            // 10% faster each time

    // Timing constants (ms)
    constexpr unsigned long JUMP_DURATION = 600;
    constexpr unsigned long WIN_TIME = 4000;           // 4 seconds to win
    constexpr unsigned long WIN_STATE_DURATION = 2000; // 2 seconds for winning state
    constexpr unsigned long RESTART_DELAY = 2000;      // 2 seconds delay before auto-restart

    // Game messages
    constexpr const char *WELCOME_MSG_LINE1 = "Runner Game";
    constexpr const char *WELCOME_MSG_LINE2 = "Press jump btn";
    constexpr const char *GAME_OVER_MSG = "GAME OVER!";
    constexpr const char *WIN_MSG_LINE1 = "YOU WIN!";
    constexpr const char *WIN_MSG_LINE2 = "Survived 1 min";

    // Sound effects
    constexpr int JUMP_SOUND_FREQ = 800;
    constexpr int JUMP_SOUND_DURATION = 100;
    constexpr int COLLISION_SOUND_FREQ = 200;
    constexpr int COLLISION_SOUND_DURATION = 500;
    constexpr int SCORE_SOUND_FREQ = 1000;
    constexpr int SCORE_SOUND_DURATION = 150;

    // LED feedback
    constexpr int JUMP_LED_RED = 0;
    constexpr int JUMP_LED_GREEN = 255;
    constexpr int JUMP_LED_BLUE = 0;

    constexpr int COLLISION_LED_RED = 255;
    constexpr int COLLISION_LED_GREEN = 0;
    constexpr int COLLISION_LED_BLUE = 0;

    constexpr int SCORE_LED_RED = 0;
    constexpr int SCORE_LED_GREEN = 0;
    constexpr int SCORE_LED_BLUE = 255;

    constexpr int WIN_LED_RED = 255;
    constexpr int WIN_LED_GREEN = 215;
    constexpr int WIN_LED_BLUE = 0;

    constexpr int LED_DURATION = 200;  // ms
    constexpr int WIN_BLINK_COUNT = 3; // Number of times to blink for win effect
}

/**
 * @brief Game states for the Runner Game.
 *
 * This enum class defines the various states that the game can be in.
 */
enum class RunnerGameState
{
    Idle,
    Playing,
    GameOver,
    Winning
};

/**
 * @brief Obstacle types for the Runner Game.
 *
 * This enum class defines the different types of obstacles that can appear in the game.
 */
enum class ObstacleType
{
    Bird,
    CactusType1,
    CactusType2
};

/**
 * @brief Runner Game class.
 *
 * This class implements a simple running game for Arduino using an LCD display for graphics
 * and the Whadda module for displaying the score. It manages game states:
 * - Idle: Waiting for the player to start the game.
 * - Playing: The game is running.
 * - Game Over: The game has ended; waiting for restart.
 * - Winning: The game has ended and is in the winning state.
 *
 * Winning condition: Survive for 1 minute.
 */
class RunnerGame : public BaseGame
{
public:
    RunnerGame();

    /**
     * @brief Initializes the game hardware and displays the welcome screen.
     */
    void init();

    /**
     * @brief Runs the game logic. Should be called repeatedly.
     * @return true if the challenge is complete (player wins), false otherwise.
     */
    bool run() override;

private:
    RunnerGameState currentState;
    unsigned long lastUpdateTime;
    unsigned long gameStartTime;
    int cactusPos;
    int llamaRow;
    bool isJumping;
    unsigned long jumpStartTime;
    bool jumpButtonReleased;
    unsigned long score;
    ObstacleType currentObstacleType;    // Track which obstacle type is currently displayed
    unsigned long gameInterval;          // Current game speed interval
    unsigned long lastSpeedIncreaseTime; // Time of last speed increase
    unsigned long gameOverTime;          // Time when game over occurred
    int animationState;                  // Current animation state (0=standing, 1=right foot, 2=left foot)
    unsigned long lastAnimationTime;     // Time of last animation update
    unsigned long winStateStartTime;     // Time when winning state started

    /**
     * @brief Resets game variables and starts a new game.
     */
    void startGame();

    /**
     * @brief Displays the game over screen.
     */
    void showGameOver();

    /**
     * @brief Displays the win screen.
     */
    void showWinScreen();

    /**
     * @brief Handles the idle state logic.
     * @param jumpPressed Whether the jump button is pressed.
     * @return true if the game should continue, false otherwise.
     */
    bool handleIdleState(bool jumpPressed);

    /**
     * @brief Handles the playing state logic.
     * @param currentTime Current time in milliseconds.
     * @param jumpPressed Whether the jump button is pressed.
     * @return true if the game should continue, false otherwise.
     */
    bool handlePlayingState(unsigned long currentTime, bool jumpPressed);

    /**
     * @brief Handles the game over state logic.
     * @param jumpPressed Whether the jump button is pressed.
     * @return true if the game should continue, false otherwise.
     */
    bool handleGameOverState(bool jumpPressed);

    /**
     * @brief Handles the winning state logic.
     * @param currentTime Current time in milliseconds.
     * @return true if the game should continue, false otherwise.
     */
    bool handleWinningState(unsigned long currentTime);

    /**
     * @brief Updates the jump state based on button input.
     * @param currentTime Current time in milliseconds.
     * @param jumpPressed Whether the jump button is pressed.
     */
    void updateJumpState(unsigned long currentTime, bool jumpPressed);

    /**
     * @brief Updates the game objects (obstacle position, collision detection).
     * @return true if collision detected, false otherwise.
     */
    bool updateGameObjects();

    /**
     * @brief Draws the game graphics on the LCD.
     */
    void drawGameGraphics();

    /**
     * @brief Updates the score display.
     */
    void updateScoreDisplay();

    /**
     * @brief Provides audio feedback for jumping.
     */
    void playJumpSound();

    /**
     * @brief Provides audio feedback for collision.
     */
    void playCollisionSound();

    /**
     * @brief Provides audio feedback for scoring.
     */
    void playScoreSound();

    /**
     * @brief Provides visual feedback for jumping.
     */
    void showJumpFeedback();

    /**
     * @brief Provides visual feedback for collision.
     */
    void showCollisionFeedback();

    /**
     * @brief Provides visual feedback for scoring.
     */
    void showScoreFeedback();

    /**
     * @brief Provides visual feedback for winning the game.
     */
    void showWinFeedback();

    /**
     * @brief Updates the game speed based on elapsed time.
     * @param currentTime Current time in milliseconds.
     */
    void updateGameSpeed(unsigned long currentTime);
};

#endif
