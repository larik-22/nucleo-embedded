#include <Pins.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Game1.h"
#include "Game2.h"
#include "Game3.h"
#include "Game4.h"

// -----------------------------------------------------------------------------
// Component Instances
// -----------------------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
RGBLed rgbLed(RGB_RED, RGB_GREEN, RGB_BLUE);
Buzzer buzzer(BUZZER_PIN);
Whadda whadda(STB_PIN, CLK_PIN, DIO_PIN);

// -----------------------------------------------------------------------------
// Global Variables for Game State
// -----------------------------------------------------------------------------
bool showTimer = true;
bool gameStarted = false;
bool allChallengesComplete = false;
unsigned long gameStartTime = 0;
const unsigned long GAME_DURATION = 600000UL; // e.g., 10 minutes

// -----------------------------------------------------------------------------
// Function Declarations
// -----------------------------------------------------------------------------
void updateTimerOnLCD();
bool timeRemaining();
void checkGameStart();
void runChallenges();
void handleGameOver();
void handleGameWin();

/**
 * @brief Setup function
 *
 * This function initializes the hardware components, including the LCD,
 * RGB LED, Buzzer, and Whadda display. It also sets up the initial state
 * of the game, including the start button and the display message.
 */
void setup()
{
  Serial.begin(9600);
  Serial.println("Initializing...");

  // Initialize Whadda display
  whadda.displayBegin();
  whadda.clearDisplay();
  whadda.displayText("Escape Room!");

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Escape Room!");

  // Initialize start button
  pinMode(BTN_PIN, INPUT_PULLUP);

  // Initialize Buzzer and RGB LED
  buzzer.begin();
  rgbLed.begin();

  // Prompt user to press the start button
  lcd.setCursor(0, 1);
  lcd.print("Press start btn");
}

/**
 * @brief Main loop function
 *
 * Main loop function that handles the game logic, including:
 * - Checking for button presses
 * - Updating the timer
 * - Running challenges
 * - Checking win/lose conditions
 */
void loop()
{
  // Update hardware interfaces
  rgbLed.update();
  whadda.update();

  if (!gameStarted)
  {
    // Check for a valid button press to start the game
    checkGameStart();
  }
  else
  {
    // Update the countdown timer on the LCD
    updateTimerOnLCD();

    // Check if time has expired; if so, handle game over
    if (!timeRemaining())
    {
      handleGameOver();
    }

    // Run the current challenge
    runChallenges();

    // If all challenges are complete, handle the win condition
    if (allChallengesComplete)
    {
      handleGameWin();
    }
  }
}

/**
 * @brief Checks for a valid start button press.
 *
 * Uses a static variable to track the duration of the button press.
 */
void checkGameStart()
{
  static unsigned long btnPressStart = 0;

  if (digitalRead(BTN_PIN) == LOW)
  { // Button is active LOW
    if (btnPressStart == 0)
    {
      btnPressStart = millis();
    }
    else if (millis() - btnPressStart > 100)
    { // Valid press threshold
      gameStarted = true;
      gameStartTime = millis(); // Record start time
      lcd.clear();
      showTimer = false;
      lcd.print("Game Started!");
      btnPressStart = 0; // Reset for future use
    }
  }
  else
  {
    btnPressStart = 0;
  }
}

/**
 * @brief Updates the countdown timer on the LCD.
 *
 * Formats the remaining time (MM:SS) and displays it at a fixed position.
 */
void updateTimerOnLCD()
{
  if (!showTimer)
    return;

  unsigned long elapsed = millis() - gameStartTime;
  unsigned long remaining = (elapsed < GAME_DURATION) ? (GAME_DURATION - elapsed) : 0;

  // Convert remaining time to minutes and seconds
  unsigned int seconds = (remaining / 1000UL) % 60;
  unsigned int minutes = (remaining / 1000UL) / 60;

  lcd.setCursor(11, 0);
  char timerStr[6]; // Format: MM:SS
  snprintf(timerStr, sizeof(timerStr), "%02u:%02u", minutes, seconds);
  lcd.print(timerStr);
}

/**
 * @brief Returns true if game time is still remaining.
 */
bool timeRemaining()
{
  unsigned long elapsed = millis() - gameStartTime;
  return (elapsed < GAME_DURATION);
}

/**
 * @brief Runs challenges sequentially.
 *
 * Uses a static challenge counter to decide which challenge to run.
 * Each challenge is non-blocking; when a challenge is finished, the counter is incremented.
 */
void runChallenges()
{
  static int currentChallenge = 1;
  bool challengeFinished = false;

  switch (currentChallenge)
  {
  case 1:
  {
    static EscapeVelocity escapeVelocity;
    challengeFinished = escapeVelocity.run();
    if (challengeFinished)
    {
      currentChallenge++;
      lcd.clear();
      lcd.print("Game 2 start");
    }
  }
  break;

  case 2:
  {
    static MemoryGame memoryGame;
    challengeFinished = memoryGame.run();
    if (challengeFinished)
    {
      currentChallenge++;
      lcd.clear();
      lcd.print("Game 3 start");
    }
  }
  break;

  case 3:
  {
    if (runGame3())
    {
      currentChallenge++;
      lcd.clear();
      lcd.print("Game 4 start");
    }
  }
  break;

  case 4:
  {
    if (runGame4())
    {
      allChallengesComplete = true;
    }
  }
  break;

  default:
    // No further challenges
    break;
  }
}

/**
 * @brief Handles the win condition.
 *
 * Plays a win melody, updates the display, and locks the game in a win state.
 */
void handleGameWin()
{
  buzzer.playWinMelody();
  lcd.clear();
  lcd.print("You Escaped!");
  while (1)
  {
    // Example win effect: set LEDs to green and blink them
    rgbLed.setColor(0, 255, 0);
    rgbLed.blinkCurrentColor(3);
  }
}

/**
 * @brief Handles the game-over condition.
 *
 * Plays the lose melody, updates the display, and locks the game.
 */
void handleGameOver()
{
  buzzer.playLoseMelody();
  lcd.clear();
  showTimer = false;
  lcd.print("Game Over!");
  while (1)
  {
    // Remain in the game-over state (or add reset logic here)
  }
}
