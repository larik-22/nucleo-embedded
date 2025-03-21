#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <RGBLed.h>
#include <Buzzer.h>
#include <Whadda.h>
#include <pins.h>
#include <game2.h>

extern LiquidCrystal_I2C lcd;
extern RGBLed rgbLed;
extern Buzzer buzzer;
extern Whadda whadda;

// /**
//  * @brief Function to run the second game.
//  * @return true if the game is completed.
//  */
// bool runGame2()
// {
//     // 1. Initialize the game
//     // 2. Start rounds (5 rounds)
//     // 3. Code to check current potentiometer value
//     // 4. Generate gate with threshold for each level
//     // 5. Show required value on LCD
//     // 6. Show current value on Whadda
//     // 7. Show time left to complete round
//     // 8. 3 leds on whadda – 3 lives
//     // 9. If not completed in time, decrease a live with a cool effect and give 5sec to find range until decreasing next live
//     // 10. If completed, show something on lcd and return true
// }

// Gameplay parameters
static const int TOTAL_GATES = 3;
static const int STARTING_LIVES = 3;
static const unsigned long GATE_TIME_LIMIT_MS = 10000;  // 10 seconds
static const unsigned long REQUIRED_IN_RANGE_MS = 3000; // 3 seconds
static const unsigned long BEEP_INTERVAL_MS = 500;      // beep gap
static const float ALPHA = 0.1f;                        // smoothing factor
static const int tolerance = 5;                         // hysteresis around range

static float potFilter = 0.0f;
static void initializePotFilter()
{
    // Initialize with a direct read to avoid large transitional jump
    potFilter = (float)analogRead(POT_PIN);
}

/**
 * Returns a smoothed pot value scaled by the current gate level.
 * By multiplying by gateLevel, the maximum reading (and thus difficulty)
 * increases with each successive gate.
 */
static int getSmoothedPotValue(int gateLevel)
{
    // Read raw pot
    int raw = analogRead(POT_PIN);

    // Exponential smoothing
    potFilter = (1.0f - ALPHA) * potFilter + ALPHA * (float)raw;

    // Scale up with gateLevel
    int scaledValue = (int)(potFilter * gateLevel);

    return scaledValue;
}

// -----------------------------------------------------------------------------
// Range generation, lives handling, animations, etc.
// -----------------------------------------------------------------------------
static void generateVelocityRange(int gateLevel, int &minVel, int &maxVel)
{
    long maxPossible = 1023L * gateLevel;
    if (maxPossible < 100)
        maxPossible = 100; // safety net

    // random() in [50, maxPossible - 50] to pick a center
    long center = random(50, maxPossible - 50);
    int halfRange = 30; // narrower or widen to tweak difficulty

    minVel = center - halfRange;
    maxVel = center + halfRange;

    if (minVel < 0)
        minVel = 0;
    // It's okay if maxVel goes beyond the real pot reading range for extra challenge
}

static void updateLivesOnWhadda(int lives)
{
    for (int i = 0; i < 8; i++)
    {
        whadda.setLED(i, false);
    }
    for (int i = 0; i < lives && i < 3; i++)
    {
        whadda.setLED(i, true);
    }
}

static void doRestartEffect()
{
    // Display "Out of lives" on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Out of lives...");

    // A short buzzer effect
    for (int i = 0; i < 3; i++)
    {
        buzzer.playTone(200, 300);
        delay(300);
    }
    // Maybe flash all Whadda LEDs
    for (int i = 0; i < 8; i++)
    {
        whadda.setLED(i, true);
    }
    delay(800);
    for (int i = 0; i < 8; i++)
    {
        whadda.setLED(i, false);
    }

    // Clear LCD for next run
    lcd.clear();
    delay(500);
}

/**
 * Attempt to pass a single gate. Return true if passed, false if time runs out.
 */
static bool attemptGate(int gateNumber)
{
    // Generate the target range
    int minVel, maxVel;
    generateVelocityRange(gateNumber, minVel, maxVel);

    whadda.clearDisplay();

    // Setup the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gate ");
    lcd.print(gateNumber);

    // Second line: Show the range
    lcd.setCursor(0, 1);
    lcd.print("Range:");
    lcd.print(minVel);
    lcd.print("-");
    lcd.print(maxVel);

    // Keep track of time
    unsigned long gateStartTime = millis();
    unsigned long inRangeStartTime = 0;
    bool wasOutOfRange = true;
    unsigned long lastBeepTime = 0;

    // For each new gate, re-initialize pot filter
    initializePotFilter();

    Serial.print("Gate ");
    Serial.print(gateNumber);
    Serial.print(" - Range: ");
    Serial.print(minVel);
    Serial.print(" - ");
    Serial.println(maxVel);

    // Gameplay loop for this gate
    while (true)
    {
        // Check if gate time is over
        if (millis() - gateStartTime > GATE_TIME_LIMIT_MS)
        {
            return false; // gate failed
        }

        // Get the smoothed pot reading (scaled)
        int potValue = getSmoothedPotValue(gateNumber);

        // Update LCD line 0 with the current speed (right side)
        lcd.setCursor(8, 0);
        // e.g. "Spd:####"
        char buff[8];
        sprintf(buff, "Spd:%4d", potValue);
        whadda.displayText(buff);

        // For stability, we add a small "tolerance" on either side of the range
        int minCheck = minVel - tolerance;
        int maxCheck = maxVel + tolerance;

        if (potValue >= minCheck && potValue <= maxCheck)
        {
            // Possibly beep if enough time elapsed
            if (millis() - lastBeepTime >= BEEP_INTERVAL_MS)
            {
                buzzer.playTone(800, 50);
                lastBeepTime = millis();
            }

            if (wasOutOfRange)
            {
                // Just entered range
                inRangeStartTime = millis();
                wasOutOfRange = false;
            }

            // Check how long we have been continuously in range
            if (millis() - inRangeStartTime >= REQUIRED_IN_RANGE_MS)
            {
                // Gate passed
                return true;
            }
        }
        else
        {
            // Out of range
            if (!wasOutOfRange)
            {
                // Optional: beep to note the moment we go out of range
                buzzer.playTone(300, 100);
            }
            wasOutOfRange = true;
        }

        delay(50);
    }
}

// -----------------------------------------------------------------------------
// Main challenge function
// -----------------------------------------------------------------------------
bool runGame2()
{
    // Setup local state
    int currentGate = 1;
    int lives = STARTING_LIVES;

    // Greet the user
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Escape Velocity!");
    lcd.setCursor(0, 1);
    lcd.print("Good luck!");
    delay(1500);

    // Initialize Whadda lives
    updateLivesOnWhadda(lives);

    // Attempt to complete all gates
    while (currentGate <= TOTAL_GATES)
    {
        bool success = attemptGate(currentGate);
        if (success)
        {
            // Short beep to celebrate
            buzzer.playTone(1000, 150);
            delay(150);
            buzzer.playTone(1200, 150);

            // Move on
            currentGate++;
        }
        else
        {
            // Fail => lose a life
            lives--;
            updateLivesOnWhadda(lives);

            buzzer.playTone(200, 500); // Long beep

            // If out of lives, do a restart effect
            if (lives <= 0)
            {
                doRestartEffect();

                // Restart the challenge
                currentGate = 1;
                lives = STARTING_LIVES;
                updateLivesOnWhadda(lives);

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Retrying...");
                delay(1000);
            }
        }
    }

    // All gates cleared => success
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Challenge Done!");
    // Victory beep pattern
    buzzer.playTone(1500, 300);
    delay(200);
    buzzer.playTone(1800, 300);
    delay(200);

    // Return true so main game knows the challenge is finished
    Serial.println("Game 2 completed!");
    return true;
}