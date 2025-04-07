#include "RunnerGame.h"
#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Button.h"

// External hardware instances
extern LiquidCrystal_I2C lcd;
extern RGBLed rgbLed;
extern Buzzer buzzer;
extern Whadda whadda;
extern Button jumpButton;
extern bool showTimer;

// -------------------------------------------------------------------
// Custom Icons for Llama and Cactus
// -------------------------------------------------------------------

// Llama Icon:
// Designed with a small head, body, and legs
static byte llamaChar[8] = {
    B00110, // Row 0: head
    B00111, // Row 1: head outline
    B00111, // Row 2: neck
    B00110, // Row 3: neck outline
    B01110, // Row 4: body
    B01110, // Row 5: body
    B01110, // Row 6: body
    B01010  // Row 7: legs
};

// Cactus Icon:
// A symmetrical cactus with branches
static byte cactusChar[8] = {
    B01110, // Row 0: top of cactus
    B01110, // Row 1: upper body
    B11111, // Row 2: main body
    B01110, // Row 3: middle body
    B11111, // Row 4: main body
    B01110, // Row 5: lower body
    B11111, // Row 6: base
    B01110  // Row 7: bottom
};

RunnerGame::RunnerGame()
    : currentState(RunnerGameState::Idle),
      lastUpdateTime(0),
      gameStartTime(0),
      cactusPos(RunnerGameConfig::INITIAL_CACTUS_POS),
      tRexRow(RunnerGameConfig::GROUND_ROW),
      isJumping(false),
      jumpStartTime(0),
      jumpButtonReleased(true), // Ready to accept a jump on first press
      score(0),
      currentCactusType(0),
      gameInterval(RunnerGameConfig::INITIAL_GAME_INTERVAL),
      lastSpeedIncreaseTime(0),
      gameOverTime(0)
{
}

void RunnerGame::init()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // Load custom characters into the LCD's memory
    lcd.createChar(RunnerGameConfig::T_REX_CHAR_ID, llamaChar);
    lcd.createChar(RunnerGameConfig::CACTUS1_CHAR_ID, cactusChar);
    lcd.createChar(RunnerGameConfig::CACTUS2_CHAR_ID, cactusChar); // Using same cactus for both

    // Display the welcome/idle screen on the LCD
    lcd.setCursor(0, 0);
    lcd.print(RunnerGameConfig::WELCOME_MSG_LINE1);
    lcd.setCursor(0, 1);
    lcd.print(RunnerGameConfig::WELCOME_MSG_LINE2);

    // Clear the score display on Whadda
    whadda.clearDisplay();

    currentState = RunnerGameState::Idle;
}

void RunnerGame::startGame()
{
    lcd.clear();
    cactusPos = RunnerGameConfig::INITIAL_CACTUS_POS;
    tRexRow = RunnerGameConfig::GROUND_ROW; // Llama starts on the bottom row
    isJumping = false;
    score = 0;
    jumpButtonReleased = false; // Require button release before new jump
    currentState = RunnerGameState::Playing;
    lastUpdateTime = millis();
    gameStartTime = millis();                               // Start the win timer
    lastSpeedIncreaseTime = millis();                       // Initialize speed increase timer
    gameInterval = RunnerGameConfig::INITIAL_GAME_INTERVAL; // Reset game speed
    // Initialize score on Whadda display
    updateScoreDisplay();
}

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

void RunnerGame::showWinScreen()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(RunnerGameConfig::WIN_MSG_LINE1);
    lcd.setCursor(0, 1);
    lcd.print(RunnerGameConfig::WIN_MSG_LINE2);
    updateScoreDisplay(); // Final score display

    // Play winning melody
    buzzer.playWinMelody();
}

bool RunnerGame::handleIdleState(bool jumpPressed)
{
    // Wait for jump button press to start the game
    if (jumpPressed && jumpButtonReleased)
    {
        startGame();
    }
    return false; // Game is not complete yet, waiting for player to start
}

bool RunnerGame::handlePlayingState(unsigned long currentTime, bool jumpPressed)
{
    // Winning condition: survive for 1 minute
    if (currentTime - gameStartTime >= RunnerGameConfig::WIN_TIME)
    {
        showWinScreen();
        return true; // Game is complete - return true to indicate completion
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

void RunnerGame::updateJumpState(unsigned long currentTime, bool jumpPressed)
{
    // Initiate jump only if not already jumping and the button has been released
    if (!isJumping && jumpPressed && jumpButtonReleased)
    {
        isJumping = true;
        jumpStartTime = currentTime;
        tRexRow = RunnerGameConfig::JUMP_ROW; // Llama moves to the top row when starting a jump
        jumpButtonReleased = false;           // Prevent immediate re-triggering
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
            tRexRow = RunnerGameConfig::GROUND_ROW;
        }
        else if (!jumpPressed && (currentTime - jumpStartTime >= RunnerGameConfig::MIN_JUMP_DURATION))
        {
            // Button released after a minimal jump duration; end jump early
            isJumping = false;
            tRexRow = RunnerGameConfig::GROUND_ROW;
        }
    }
}

bool RunnerGame::updateGameObjects()
{
    // Move the cactus leftward
    cactusPos--;

    // If the cactus goes off-screen, reset its position and increment the score
    if (cactusPos < 0)
    {
        cactusPos = RunnerGameConfig::INITIAL_CACTUS_POS;
        score++;
        // Update score on Whadda display
        updateScoreDisplay();
        playScoreSound();
        showScoreFeedback();
    }

    // Collision detection: if the cactus reaches column 0 and Llama is on the ground
    if (cactusPos == 0 && tRexRow == RunnerGameConfig::GROUND_ROW)
    {
        return true; // Collision detected
    }

    return false; // No collision
}

void RunnerGame::drawGameGraphics()
{
    // Redraw game graphics on the LCD:
    lcd.clear();

    // Draw Llama using its custom icon
    lcd.setCursor(0, tRexRow);
    lcd.write(RunnerGameConfig::T_REX_CHAR_ID); // Use the character ID directly

    // Draw cactus on the bottom row using its custom icon
    lcd.setCursor(cactusPos, RunnerGameConfig::GROUND_ROW);
    lcd.write(RunnerGameConfig::CACTUS1_CHAR_ID); // Use the character ID directly
}

void RunnerGame::updateScoreDisplay()
{
    // Format the score with leading zeros for better display
    char scoreStr[10];
    snprintf(scoreStr, sizeof(scoreStr), "Score: %lu", score);
    whadda.displayText(scoreStr);
}

void RunnerGame::playJumpSound()
{
    buzzer.playTone(RunnerGameConfig::JUMP_SOUND_FREQ, RunnerGameConfig::JUMP_SOUND_DURATION);
}

void RunnerGame::playCollisionSound()
{
    buzzer.playTone(RunnerGameConfig::COLLISION_SOUND_FREQ, RunnerGameConfig::COLLISION_SOUND_DURATION);
}

void RunnerGame::playScoreSound()
{
    buzzer.playTone(RunnerGameConfig::SCORE_SOUND_FREQ, RunnerGameConfig::SCORE_SOUND_DURATION);
}

void RunnerGame::showJumpFeedback()
{
    rgbLed.setColor(RunnerGameConfig::JUMP_LED_RED, RunnerGameConfig::JUMP_LED_GREEN, RunnerGameConfig::JUMP_LED_BLUE);
    delay(RunnerGameConfig::LED_DURATION);
    rgbLed.off();
}

void RunnerGame::showCollisionFeedback()
{
    rgbLed.setColor(RunnerGameConfig::COLLISION_LED_RED, RunnerGameConfig::COLLISION_LED_GREEN, RunnerGameConfig::COLLISION_LED_BLUE);
    delay(RunnerGameConfig::LED_DURATION);
    rgbLed.off();
}

void RunnerGame::showScoreFeedback()
{
    rgbLed.setColor(RunnerGameConfig::SCORE_LED_RED, RunnerGameConfig::SCORE_LED_GREEN, RunnerGameConfig::SCORE_LED_BLUE);
    delay(RunnerGameConfig::LED_DURATION);
    rgbLed.off();
}

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

bool RunnerGame::run()
{
    unsigned long currentTime = millis();

    // Update button state
    bool jumpPressed = jumpButton.read();

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

    default:
        return false; // Game is not complete yet
    }
}
