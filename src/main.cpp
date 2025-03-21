/**
 * main.cpp
 *
 * Main file orchestrating:
 *  - Hardware initialization
 *  - Countdown timer (10 minutes)
 *  - Sequential challenges
 *  - Win/Lose condition
 *
 * Assumes you have the following additional files:
 *  game1.cpp, game2.cpp, game3.cpp, game4.cpp
 * with corresponding headers that expose runGameX() functions.
 */

#include <pins.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"

#include "game1.h"
#include "game2.h"
#include "game3.h"
#include "game4.h"

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
bool gameStarted = false;
bool allChallengesComplete = false;
unsigned long gameStartTime = 0;
const unsigned long GAME_DURATION = 600000UL; // 10 minutes in milliseconds

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------
void updateTimerOnLCD();
bool timeRemaining();

// -----------------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------------
void setup()
{
  // Initialize Serial for debugging if desired
  Serial.begin(9600);
  Serial.println("Initializing...");

  // Initialize Whadda Display (if used in your puzzle, or for extra readouts)
  whadda.displayBegin();
  whadda.clearDisplay();
  whadda.displayText("Escape Room!");

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Escape Room!");

  // Initialize the Start Button
  pinMode(BTN_PIN, INPUT_PULLUP);

  // Initialize the Buzzer and RGB LED
  buzzer.begin();
  rgbLed.begin();

  // Prompt user to press button to start
  lcd.setCursor(0, 1);
  lcd.print("Press start btn");
  // buzzer.playImperialMarch();
}

// -----------------------------------------------------------------------------
// loop()
// -----------------------------------------------------------------------------
void loop()
{
  if (!gameStarted)
  {
    // Wait until user presses button (active LOW)
    if (digitalRead(BTN_PIN) == LOW)
    {
      // Debounce or short delay if desired
      delay(100);
      if (digitalRead(BTN_PIN) == LOW)
      {
        // Start the game
        gameStarted = true;
        gameStartTime = millis(); // Timestamp game start
        lcd.clear();
        lcd.print("Game Started!");
        delay(1000);
        lcd.clear();
      }
    }
  }
  else
  {
    // Update the countdown timer
    updateTimerOnLCD();

    // Check if time is still remaining
    if (!timeRemaining())
    {
      // Time up => losing condition
      buzzer.playLoseMelody();
      lcd.clear();
      lcd.print("Game Over!");
      while (1)
      {
        // Lock up in "game over" state
        // or reset the board, etc.
      }
    }

    // Sequentially run challenges if not yet complete
    static int currentChallenge = 1;

    switch (currentChallenge)
    {
    case 1:
      if (runGame2())
      {
        currentChallenge++;
        // Provide short transition
        lcd.clear();
        lcd.print("Game 2 start");
      }
      break;
    case 2:
      if (runGame1())
      {
        // todo: function to start next challenge
        currentChallenge++;
        lcd.clear();
        lcd.print("Game 3 start");
      }
      break;
    case 3:
      if (runGame3())
      {
        currentChallenge++;
        lcd.clear();
        lcd.print("Game 4 start");
      }
      break;
    case 4:
      if (runGame4())
      {
        // If we complete the 4th challenge, user has won
        allChallengesComplete = true;
      }
      break;
    default:
      // Do nothing
      break;
    }

    // Check for overall completion
    if (allChallengesComplete)
    {
      buzzer.playWinMelody();
      lcd.clear();
      lcd.print("You Escaped!");
      while (1)
      {
        // Lock up in "win" state
        // or do fancy blinking, etc.
        rgbLed.setColor(0, 255, 0); // Set to green for win
        rgbLed.blink(3);
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

/**
 *  updateTimerOnLCD()
 *  - Calculates remaining time and prints it on the second row of the LCD.
 *  - Restores the LCD cursor after challenge messages if needed.
 */
void updateTimerOnLCD()
{
  unsigned long elapsed = millis() - gameStartTime;
  unsigned long remaining = 0;

  if (elapsed < GAME_DURATION)
  {
    remaining = GAME_DURATION - elapsed;
  }
  else
  {
    remaining = 0;
  }

  // Convert to MM:SS
  unsigned int seconds = (remaining / 1000UL) % 60;
  unsigned int minutes = (remaining / 1000UL) / 60;

  // Position the timer on the LCD. Adjust cursor as you prefer.
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  if (minutes < 10)
    lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10)
    lcd.print("0");
  lcd.print(seconds);
}

/**
 *  timeRemaining()
 *  - Returns true if there is still time left on the clock, false if time is up.
 */
bool timeRemaining()
{
  unsigned long elapsed = millis() - gameStartTime;
  return (elapsed < GAME_DURATION);
}
