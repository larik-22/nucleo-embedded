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

// Llama standing (two parts)
static byte llamaStandingPart1[8] = {
    B00000,
    B00000,
    B00110, // Llama head
    B00110,
    B00111, // Llama neck
    B00111,
    B00011, // Llama body
    B00011};
static byte llamaStandingPart2[8] = {
    B00111,
    B00111,
    B00111,
    B00100,
    B11100,
    B11100,
    B11000,
    B11000};

// Llama with right foot forward
static byte llamaRightFootPart1[8] = {
    B00000,
    B00000,
    B00110, // Llama head
    B00110,
    B00111, // Llama neck
    B00111,
    B00011, // Llama body
    B00011};
static byte llamaRightFootPart2[8] = {
    B00111,
    B00111,
    B00111,
    B00100,
    B11100,
    B11100,
    B11000,
    B00000 // Right foot forward
};

// Llama with left foot forward
static byte llamaLeftFootPart1[8] = {
    B00000,
    B00000,
    B00110, // Llama head
    B00110,
    B00111, // Llama neck
    B00111,
    B00011, // Llama body
    B00000  // Left foot forward
};
static byte llamaLeftFootPart2[8] = {
    B00111,
    B00111,
    B00111,
    B00100,
    B11100,
    B11100,
    B11000,
    B11000};

// Cactus (two parts)
static byte cactusPart1[8] = {
    B00000,
    B00100,
    B00100,
    B10100,
    B10100,
    B11100,
    B00100,
    B00100};
static byte cactusPart2[8] = {
    B00100,
    B00101,
    B00101,
    B10101,
    B11111,
    B00100,
    B00100,
    B00100};
// byte TWO_BRANCHES_PART_1[8] = {B00000, B00100, B00100, B10100, B10100, B11100, B00100, B00100};
// byte TWO_BRANCHES_PART_2[8] = {B00100, B00101, B00101, B10101, B11111, B00100, B00100, B00100};

RunnerGame::RunnerGame()
    : currentState(RunnerGameState::Idle),
      lastUpdateTime(0),
      gameStartTime(0),
      cactusPos(RunnerGameConfig::INITIAL_CACTUS_POS),
      llamaRow(RunnerGameConfig::GROUND_ROW), // Renamed from tRexRow
      isJumping(false),
      jumpStartTime(0),
      jumpButtonReleased(true),
      score(0),
      currentCactusType(0),
      gameInterval(RunnerGameConfig::INITIAL_GAME_INTERVAL),
      lastSpeedIncreaseTime(0),
      gameOverTime(0),
      animationState(0),
      lastAnimationTime(0)
{
}

void RunnerGame::init()
{
    // Initialize the LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // Create custom characters - EXACTLY as in the working example
    lcd.createChar(0, llamaStandingPart1);
    lcd.createChar(1, llamaStandingPart2);
    lcd.createChar(2, llamaRightFootPart1);
    lcd.createChar(3, llamaRightFootPart2);
    lcd.createChar(4, llamaLeftFootPart1);
    lcd.createChar(5, llamaLeftFootPart2);
    lcd.createChar(6, cactusPart1);
    lcd.createChar(7, cactusPart2);

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
    llamaRow = RunnerGameConfig::GROUND_ROW; // Renamed from tRexRow
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

    // Play winning melody with a more elaborate effect
    buzzer.playWinMelody();

    // Add visual feedback for winning
    rgbLed.startBlinkColor(0, 255, 0, 3);
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
    // Winning condition: survive for 30 seconds
    if (currentTime - gameStartTime >= RunnerGameConfig::WIN_TIME)
    {
        showWinScreen();
        return true; // Game is complete - return true to indicate completion
    }

    // Update jump state
    updateJumpState(currentTime, jumpPressed);

    // Update game speed
    updateGameSpeed(currentTime);

    // Update animation state
    updateAnimationState(currentTime);

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
    if (!isJumping && jumpPressed && jumpButtonReleased)
    {
        isJumping = true;
        jumpStartTime = currentTime;
        llamaRow = RunnerGameConfig::JUMP_ROW; // Renamed from tRexRow
        jumpButtonReleased = false;
        playJumpSound();
        showJumpFeedback();
    }

    if (isJumping)
    {
        if (currentTime - jumpStartTime >= RunnerGameConfig::JUMP_DURATION)
        {
            isJumping = false;
            llamaRow = RunnerGameConfig::GROUND_ROW; // Renamed from tRexRow
        }
        else if (!jumpPressed && (currentTime - jumpStartTime >= RunnerGameConfig::MIN_JUMP_DURATION))
        {
            isJumping = false;
            llamaRow = RunnerGameConfig::GROUND_ROW; // Renamed from tRexRow
        }
    }
}

void RunnerGame::updateAnimationState(unsigned long currentTime)
{
    // Update animation state at fixed intervals
    if (currentTime - lastAnimationTime >= RunnerGameConfig::ANIMATION_INTERVAL)
    {
        lastAnimationTime = currentTime;

        // Cycle through animation states (0, 1, 2)
        animationState = (animationState + 1) % 3;
    }
}

bool RunnerGame::updateGameObjects()
{
    cactusPos--;

    if (cactusPos < 0)
    {
        cactusPos = RunnerGameConfig::INITIAL_CACTUS_POS;
        score++;
        updateScoreDisplay();
        playScoreSound();
        showScoreFeedback();
    }

    if (cactusPos == 0 && llamaRow == RunnerGameConfig::GROUND_ROW) // Renamed from tRexRow
    {
        return true;
    }

    return false;
}

void RunnerGame::drawGameGraphics()
{
    lcd.clear();

    lcd.setCursor(cactusPos, RunnerGameConfig::GROUND_ROW);
    lcd.write(byte(6));
    lcd.setCursor(cactusPos + 1, RunnerGameConfig::GROUND_ROW);
    lcd.write(byte(7));

    if (isJumping)
    {
        lcd.setCursor(0, llamaRow); // Renamed from tRexRow
        lcd.write(byte(0));
        lcd.setCursor(1, llamaRow); // Renamed from tRexRow
        lcd.write(byte(1));
    }
    else
    {
        lcd.setCursor(0, llamaRow); // Renamed from tRexRow
        if (animationState == 0)
        {
            lcd.write(byte(0));
            lcd.setCursor(1, llamaRow); // Renamed from tRexRow
            lcd.write(byte(1));
        }
        else if (animationState == 1)
        {
            lcd.write(byte(2));
            lcd.setCursor(1, llamaRow); // Renamed from tRexRow
            lcd.write(byte(3));
        }
        else
        {
            lcd.write(byte(4));
            lcd.setCursor(1, llamaRow); // Renamed from tRexRow
            lcd.write(byte(5));
        }
    }
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
