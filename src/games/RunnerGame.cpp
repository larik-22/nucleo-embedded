#include "RunnerGame.h"
#include "Globals.h"

// -------------------------------------------------------------------
// Custom Icons for Llama and Cactus
// -------------------------------------------------------------------

/**
 * @brief Bitmap for the standing llama character (part 1)
 * 
 * Custom character bitmap for the first part of the standing llama.
 * Used when the llama is not moving or during jumps.
 */
static byte llamaStandingPart1[8] = {B00000, B00000, B00110, B00110, B00111, B00111, B00011, B00011};

/**
 * @brief Bitmap for the standing llama character (part 2)
 * 
 * Custom character bitmap for the second part of the standing llama.
 * Used when the llama is not moving or during jumps.
 */
static byte llamaStandingPart2[8] = {B00111, B00111, B00111, B00100, B11100, B11100, B11000, B11000};

/**
 * @brief Bitmap for the llama with right foot forward (part 1)
 * 
 * Custom character bitmap for the first part of the llama with right foot forward.
 * Used in the running animation sequence.
 */
static byte llamaRightFootPart1[8] = {B00000, B00000, B00110, B00110, B00111, B00111, B00011, B00011};

/**
 * @brief Bitmap for the llama with right foot forward (part 2)
 * 
 * Custom character bitmap for the second part of the llama with right foot forward.
 * Used in the running animation sequence.
 */
static byte llamaRightFootPart2[8] = {B00111, B00111, B00111, B00100, B11100, B11100, B11000, B00000};

/**
 * @brief Bitmap for the llama with left foot forward (part 1)
 * 
 * Custom character bitmap for the first part of the llama with left foot forward.
 * Used in the running animation sequence.
 */
static byte llamaLeftFootPart1[8] = {B00000, B00000, B00110, B00110, B00111, B00111, B00011, B00000};

/**
 * @brief Bitmap for the llama with left foot forward (part 2)
 * 
 * Custom character bitmap for the second part of the llama with left foot forward.
 * Used in the running animation sequence.
 */
static byte llamaLeftFootPart2[8] = {B00111, B00111, B00111, B00100, B11100, B11100, B11000, B11000};

/**
 * @brief Bitmap for the cactus obstacle (part 1)
 * 
 * Custom character bitmap for the first part of the cactus obstacle.
 */
static byte cactusPart1[8] = {B00000, B00100, B00100, B10100, B10100, B11100, B00100, B00100};

/**
 * @brief Bitmap for the cactus obstacle (part 2)
 * 
 * Custom character bitmap for the second part of the cactus obstacle.
 */
static byte cactusPart2[8] = {B00100, B00101, B00101, B10101, B11111, B00100, B00100, B00100};

/**
 * @brief Constructor for RunnerGame
 * 
 * Initializes all member variables to their default values.
 * The game starts in the Idle state with the llama at ground level.
 */
RunnerGame::RunnerGame()
    : currentState(RunnerGameState::Idle),
      lastUpdateTime(0),
      gameStartTime(0),
      cactusPos(RunnerGameConfig::INITIAL_CACTUS_POS),
      llamaRow(RunnerGameConfig::GROUND_ROW),
      isJumping(false),
      jumpStartTime(0),
      jumpButtonReleased(true),
      score(0),
      currentObstacleType(ObstacleType::CactusType1),
      gameInterval(RunnerGameConfig::INITIAL_GAME_INTERVAL),
      lastSpeedIncreaseTime(0),
      gameOverTime(0),
      animationState(0),
      lastAnimationTime(0),
      winStateStartTime(0)
{
}

/**
 * @brief Initializes the game
 * 
 * Sets up the LCD display, creates custom characters, and displays the welcome screen.
 * This method is called once at the beginning of the game.
 */
void RunnerGame::init()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // Create custom characters
    lcd.createChar(RunnerGameConfig::LLAMA_STANDING_PART1_ID, llamaStandingPart1);
    lcd.createChar(RunnerGameConfig::LLAMA_STANDING_PART2_ID, llamaStandingPart2);
    lcd.createChar(RunnerGameConfig::LLAMA_RIGHT_FOOT_PART1_ID, llamaRightFootPart1);
    lcd.createChar(RunnerGameConfig::LLAMA_RIGHT_FOOT_PART2_ID, llamaRightFootPart2);
    lcd.createChar(RunnerGameConfig::LLAMA_LEFT_FOOT_PART1_ID, llamaLeftFootPart1);
    lcd.createChar(RunnerGameConfig::LLAMA_LEFT_FOOT_PART2_ID, llamaLeftFootPart2);
    lcd.createChar(RunnerGameConfig::CACTUS_PART1_ID, cactusPart1);
    lcd.createChar(RunnerGameConfig::CACTUS_PART2_ID, cactusPart2);

    // Display the welcome/idle screen on the LCD
    lcd.setCursor(0, 0);
    lcd.print(RunnerGameConfig::WELCOME_MSG_LINE1);
    lcd.setCursor(0, 1);
    lcd.print(RunnerGameConfig::WELCOME_MSG_LINE2);

    // Clear the score display on Whadda
    whadda.clearDisplay();

    currentState = RunnerGameState::Idle;
}

/**
 * @brief Starts a new game
 * 
 * Resets all game variables, positions the llama and cactus, and transitions to the Playing state.
 */
void RunnerGame::startGame()
{
    // Clear the entire screen at the start of a new game
    lcd.clear();

    cactusPos = RunnerGameConfig::INITIAL_CACTUS_POS;
    llamaRow = RunnerGameConfig::GROUND_ROW;
    isJumping = false;
    score = 0;
    jumpButtonReleased = false;
    currentState = RunnerGameState::Playing;
    lastUpdateTime = millis();
    gameStartTime = millis();
    lastSpeedIncreaseTime = millis();
    gameInterval = RunnerGameConfig::INITIAL_GAME_INTERVAL;
    animationState = 0;
    lastAnimationTime = millis();
    updateScoreDisplay();
}

/**
 * @brief Displays the game over screen
 * 
 * Shows the game over message, plays the collision sound, and transitions to the GameOver state.
 */
void RunnerGame::showGameOver()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(RunnerGameConfig::GAME_OVER_MSG);
    currentState = RunnerGameState::GameOver;
    gameOverTime = millis(); // Record when game over occurred
    playCollisionSound();
    showCollisionFeedback();
}

/**
 * @brief Displays the win screen
 * 
 * Shows the winning message, plays the win melody, and transitions to the Winning state.
 */
void RunnerGame::showWinScreen()
{
    // Display the winning message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(RunnerGameConfig::WIN_MSG_LINE1);
    lcd.setCursor(0, 1);
    lcd.print(RunnerGameConfig::WIN_MSG_LINE2);
    updateScoreDisplay(); // Final score display

    // Start blinking the LED with the win color
    rgbLed.startBlinkColor(RunnerGameConfig::WIN_LED_RED, RunnerGameConfig::WIN_LED_GREEN, RunnerGameConfig::WIN_LED_BLUE, RunnerGameConfig::WIN_BLINK_COUNT);

    // Play winning melody
    buzzer.playWinMelody();

    // Set state to winning and record the start time
    currentState = RunnerGameState::Winning;
    winStateStartTime = millis();
}

/**
 * @brief Handles the idle state logic
 * 
 * Waits for the player to press the jump button to start the game.
 * 
 * @param jumpPressed Whether the jump button is currently pressed
 * @return false (game is not complete)
 */
bool RunnerGame::handleIdleState(bool jumpPressed)
{
    // Wait for jump button press to start the game
    if (jumpPressed && jumpButtonReleased)
    {
        startGame();
    }
    return false; // Game is not complete yet, waiting for player to start
}

/**
 * @brief Handles the playing state logic
 * 
 * Updates the game state, checks for collisions, and draws the game graphics.
 * 
 * @param currentTime Current time in milliseconds
 * @param jumpPressed Whether the jump button is currently pressed
 * @return false (game is not complete)
 */
bool RunnerGame::handlePlayingState(unsigned long currentTime, bool jumpPressed)
{
    // Winning condition: survive for the specified time
    if (currentTime - gameStartTime >= RunnerGameConfig::WIN_TIME)
    {
        showWinScreen();
        return false; // Game is not complete yet, we're in the winning state
    }

    // Update jump state
    updateJumpState(currentTime, jumpPressed);

    // Update game speed
    updateGameSpeed(currentTime);

    // Update game objects at fixed intervals
    if (currentTime - lastUpdateTime >= gameInterval)
    {
        lastUpdateTime = currentTime;

        // Update game objects and check for collision
        if (updateGameObjects())
        {
            showGameOver();
            return false; // Game is not complete yet, player can restart
        }

        // Draw game graphics
        drawGameGraphics();
    }

    return false; // Game is not complete yet
}

/**
 * @brief Handles the game over state logic
 * 
 * Waits for a specified delay before automatically restarting the game.
 * 
 * @param jumpPressed Whether the jump button is currently pressed (unused in this state)
 * @return false (game is not complete)
 */
bool RunnerGame::handleGameOverState(bool jumpPressed)
{
    unsigned long currentTime = millis();

    // Auto-restart after the specified delay
    if (currentTime - gameOverTime >= RunnerGameConfig::RESTART_DELAY)
    {
        startGame();
    }

    return false; // Game is not complete yet, player can restart
}

/**
 * @brief Handles the winning state logic
 * 
 * Displays the winning animation and transitions back to the idle state after a delay.
 * 
 * @param currentTime Current time in milliseconds
 * @return true when the winning state is complete, false otherwise
 */
bool RunnerGame::handleWinningState(unsigned long currentTime)
{
    // Update RGB LED if it's blinking
    rgbLed.update();

    // Check if the winning state duration has elapsed
    if (currentTime - winStateStartTime >= RunnerGameConfig::WIN_STATE_DURATION)
    {
        // Turn off the LED
        rgbLed.off();

        // Return to idle state
        currentState = RunnerGameState::Idle;

        // Return true to indicate the game is complete
        return true;
    }

    // Still in winning state, continue blinking
    return false;
}

/**
 * @brief Updates the jump state based on button input
 * 
 * Handles jump initiation and termination based on button presses and timing.
 * 
 * @param currentTime Current time in milliseconds
 * @param jumpPressed Whether the jump button is currently pressed
 */
void RunnerGame::updateJumpState(unsigned long currentTime, bool jumpPressed)
{
    // Initiate jump only if not already jumping and the button has been released
    if (!isJumping && jumpPressed && jumpButtonReleased)
    {
        isJumping = true;
        jumpStartTime = currentTime;
        llamaRow = RunnerGameConfig::JUMP_ROW;
        jumpButtonReleased = false;
        playJumpSound();
        showJumpFeedback();
    }

    // Handle jump termination:
    // - Force end jump after maximum duration (JUMP_DURATION)
    // - Or if the button is released early (after a minimum jump time)
    if (isJumping)
    {
        if (currentTime - jumpStartTime >= RunnerGameConfig::JUMP_DURATION)
        {
            // Maximum jump duration reached; force Llama to fall
            isJumping = false;
            llamaRow = RunnerGameConfig::GROUND_ROW;
        }
        else if (!jumpPressed && (currentTime - jumpStartTime >= RunnerGameConfig::MIN_JUMP_DURATION))
        {
            // Button released after a minimal jump duration; end jump early
            isJumping = false;
            llamaRow = RunnerGameConfig::GROUND_ROW;
        }
    }
}

/**
 * @brief Updates the game objects and checks for collisions
 * 
 * Moves the cactus, updates the animation state, and checks for collisions.
 * 
 * @return true if a collision is detected, false otherwise
 */
bool RunnerGame::updateGameObjects()
{
    unsigned long currentTime = millis();

    // Update cactus position
    cactusPos--;

    // Update animation state for running animation
    if (!isJumping && currentTime - lastAnimationTime >= RunnerGameConfig::ANIMATION_INTERVAL)
    {
        lastAnimationTime = currentTime;
        animationState = (animationState + 1) % RunnerGameConfig::ANIMATION_STATES;
    }

    if (cactusPos < 0)
    {
        cactusPos = RunnerGameConfig::INITIAL_CACTUS_POS;
        score++;
        // Update score on Whadda display
        updateScoreDisplay();
        playScoreSound();
        showScoreFeedback();
    }

    if (cactusPos == 0 && llamaRow == RunnerGameConfig::GROUND_ROW)
    {
        return true;
    }

    return false; // No collision
}

/**
 * @brief Draws the game graphics on the LCD
 * 
 * Renders the llama character and cactus obstacle based on their current positions and states.
 */
void RunnerGame::drawGameGraphics()
{
    lcd.clear();

    lcd.setCursor(cactusPos, RunnerGameConfig::GROUND_ROW);
    lcd.write(byte(RunnerGameConfig::CACTUS_PART1_ID));
    lcd.setCursor(cactusPos + 1, RunnerGameConfig::GROUND_ROW);
    lcd.write(byte(RunnerGameConfig::CACTUS_PART2_ID));

    if (isJumping)
    {
        lcd.setCursor(0, llamaRow);
        lcd.write(byte(RunnerGameConfig::LLAMA_STANDING_PART1_ID));
        lcd.setCursor(1, llamaRow);
        lcd.write(byte(RunnerGameConfig::LLAMA_STANDING_PART2_ID));
    }
    else
    {
        lcd.setCursor(0, llamaRow);
        if (animationState == 0)
        {
            lcd.write(byte(RunnerGameConfig::LLAMA_STANDING_PART1_ID));
            lcd.setCursor(1, llamaRow);
            lcd.write(byte(RunnerGameConfig::LLAMA_STANDING_PART2_ID));
        }
        else if (animationState == 1)
        {
            lcd.write(byte(RunnerGameConfig::LLAMA_RIGHT_FOOT_PART1_ID));
            lcd.setCursor(1, llamaRow);
            lcd.write(byte(RunnerGameConfig::LLAMA_RIGHT_FOOT_PART2_ID));
        }
        else
        {
            lcd.write(byte(RunnerGameConfig::LLAMA_LEFT_FOOT_PART1_ID));
            lcd.setCursor(1, llamaRow);
            lcd.write(byte(RunnerGameConfig::LLAMA_LEFT_FOOT_PART2_ID));
        }
    }
}

/**
 * @brief Updates the score display on the Whadda module
 * 
 * Formats and displays the current score.
 */
void RunnerGame::updateScoreDisplay()
{
    // Format the score with leading zeros for better display
    char scoreStr[10];
    snprintf(scoreStr, sizeof(scoreStr), "Score: %lu", score);
    whadda.displayText(scoreStr);
}

/**
 * @brief Plays the jump sound effect
 * 
 * Plays a tone when the llama jumps.
 */
void RunnerGame::playJumpSound()
{
    buzzer.playTone(RunnerGameConfig::JUMP_SOUND_FREQ, RunnerGameConfig::JUMP_SOUND_DURATION);
}

/**
 * @brief Plays the collision sound effect
 * 
 * Plays a tone when the llama collides with a cactus.
 */
void RunnerGame::playCollisionSound()
{
    buzzer.playTone(RunnerGameConfig::COLLISION_SOUND_FREQ, RunnerGameConfig::COLLISION_SOUND_DURATION);
}

/**
 * @brief Plays the score sound effect
 * 
 * Plays a tone when the player successfully passes an obstacle.
 */
void RunnerGame::playScoreSound()
{
    buzzer.playTone(RunnerGameConfig::SCORE_SOUND_FREQ, RunnerGameConfig::SCORE_SOUND_DURATION);
}

/**
 * @brief Shows visual feedback for jumping
 * 
 * Sets the RGB LED to the jump color for a short duration.
 */
void RunnerGame::showJumpFeedback()
{
    rgbLed.setColor(RunnerGameConfig::JUMP_LED_RED, RunnerGameConfig::JUMP_LED_GREEN, RunnerGameConfig::JUMP_LED_BLUE);
    delay(RunnerGameConfig::LED_DURATION);
    rgbLed.off();
}

/**
 * @brief Shows visual feedback for collision
 * 
 * Sets the RGB LED to the collision color for a short duration.
 */
void RunnerGame::showCollisionFeedback()
{
    rgbLed.setColor(RunnerGameConfig::COLLISION_LED_RED, RunnerGameConfig::COLLISION_LED_GREEN, RunnerGameConfig::COLLISION_LED_BLUE);
    delay(RunnerGameConfig::LED_DURATION);
    rgbLed.off();
}

/**
 * @brief Shows visual feedback for scoring
 * 
 * Sets the RGB LED to the score color for a short duration.
 */
void RunnerGame::showScoreFeedback()
{
    rgbLed.setColor(RunnerGameConfig::SCORE_LED_RED, RunnerGameConfig::SCORE_LED_GREEN, RunnerGameConfig::SCORE_LED_BLUE);
    delay(RunnerGameConfig::LED_DURATION);
    rgbLed.off();
}

/**
 * @brief Updates the game speed based on elapsed time
 * 
 * Gradually increases the game speed to make it more challenging.
 * 
 * @param currentTime Current time in milliseconds
 */
void RunnerGame::updateGameSpeed(unsigned long currentTime)
{
    // Check if it's time to increase the game speed
    if (currentTime - lastSpeedIncreaseTime >= RunnerGameConfig::SPEED_INCREASE_INTERVAL)
    {
        // Calculate new game interval (faster)
        unsigned long newInterval = gameInterval * RunnerGameConfig::SPEED_INCREASE_FACTOR;

        // Ensure we don't go below the minimum interval
        if (newInterval >= RunnerGameConfig::MIN_GAME_INTERVAL)
        {
            gameInterval = newInterval;
        }

        // Update the last speed increase time
        lastSpeedIncreaseTime = currentTime;
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
bool RunnerGame::run()
{
    unsigned long currentTime = millis();

    // Update button state
    bool jumpPressed = button.read();

    // Update jump button release state: only allow a new jump after the button is released.
    if (!jumpPressed)
    {
        jumpButtonReleased = true;
    }

    switch (currentState)
    {
    case RunnerGameState::Idle:
        return handleIdleState(jumpPressed);

    case RunnerGameState::Playing:
        return handlePlayingState(currentTime, jumpPressed);

    case RunnerGameState::GameOver:
        return handleGameOverState(jumpPressed);

    case RunnerGameState::Winning:
        return handleWinningState(currentTime);

    default:
        return false; // Game is not complete yet
    }
}
