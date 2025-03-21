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

// Game parameters:
static const int TOTAL_GATES = 5;                // Number of gates to pass
static const int STARTING_LIVES = 1;             // Initial lives
static const unsigned long GATE_TIME_MS = 10000; // Allowed time per gate
static const unsigned long IN_RANGE_MS = 2500;   // Must stay in range
static const unsigned long BEEP_INTERVAL = 250;  // Min gap (ms) between short beeps
static const float ALPHA = 0.1f;                 // Exponential smoothing factor
static const int TOLERANCE = 5;                  // Threshold to reduce flicker issues

// Internal filter state for smoothing pot readings:
// Credits: https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogReadSerial
// Credits: [exponential smoothing] https://en.wikipedia.org/wiki/Exponential_smoothing
// And of course huge credits to gpt...
static float g_potFilter = 0.0f;

// -----------------------------------------------------------------------------
// Potentiometer & Range Utilities
// -----------------------------------------------------------------------------

/**
 * @brief Initialize the exponential filter with the current raw pot value.
 */
static void initPotFilter()
{
    g_potFilter = (float)analogRead(POT_PIN);
}

/**
 * @brief Reads the pot, applies exponential smoothing, and scales by gateLevel.
 * @param gateLevel The current gate/level (multiplier).
 * @return The scaled, smoothed pot value.
 */
static int getSmoothedPotValue(int gateLevel)
{
    int raw = analogRead(POT_PIN);
    // Exponential smoothing
    g_potFilter = (1.0f - ALPHA) * g_potFilter + ALPHA * (float)raw;
    // Scale by gateLevel
    return (int)(g_potFilter * gateLevel);
}

/**
 * @brief Generates a random velocity range for a given gateLevel.
 *        The center is in [50, maxPossible - 50] with a ~30 half-range.
 */
static void generateVelocityRange(int gateLevel, int &minVel, int &maxVel)
{
    long maxPossible = 1023L * gateLevel;
    if (maxPossible < 100)
    {
        maxPossible = 100;
    }

    long center = random(50, maxPossible - 50);
    const int halfRange = 30;

    minVel = center - halfRange;
    maxVel = center + halfRange;
    if (minVel < 0)
    {
        minVel = 0;
    }
}

// -----------------------------------------------------------------------------
// Lives & Restart Effects
// -----------------------------------------------------------------------------

/**
 * @brief Displays current lives on Whadda's first 3 LEDs.
 */
static void setWhaddaLives(int lives)
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

/**
 * @brief Shows a short "restart" animation/effect when lives drop to zero.
 */
static void doRestartEffect()
{
    whadda.clearDisplay();
    whadda.displayText("Restarting...");
    buzzer.playLoseMelody(1);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Out of lives...");

    // flash all leds
    whadda.blinkLEDs(0xFF, 5, 100);
    lcd.clear();
}

// -----------------------------------------------------------------------------
// Gate Attempt Logic
// -----------------------------------------------------------------------------

/**
 * @brief Updates LCD (top row) with the current pot speed and Whadda display as well.
 * @param gateLevel   Current gate number.
 * @param potValue    Smoothed pot reading.
 */
static void updateGateDisplays(int gateLevel, int potValue)
{
    lcd.setCursor(0, 0);
    lcd.print("Gate ");
    lcd.print(gateLevel);
    char buff[8];
    sprintf(buff, "Spd %4d", potValue);
    whadda.displayText(buff);
}

/**
 * @brief Checks if potValue is within [minVel, maxVel], with TOLERANCE.
 * @param potValue Current pot reading.
 * @param minVel   Lower bound of the velocity gate.
 * @param maxVel   Upper bound of the velocity gate.
 * @return True if within the tolerated range, false otherwise.
 */
static bool isPotInRange(int potValue, int minVel, int maxVel)
{
    int minCheck = minVel - TOLERANCE;
    int maxCheck = maxVel + TOLERANCE;
    return (potValue >= minCheck && potValue <= maxCheck);
}

/**
 * @brief Main loop for a single gate attempt. Returns true if user passes
 *        the gate, false if the time runs out.
 */
static bool doGateAttempt(int gateLevel)
{
    // Randomly generate velocity range
    int minVel = 0, maxVel = 0;
    generateVelocityRange(gateLevel, minVel, maxVel);

    // Show range info on LCD second line
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gate ");
    lcd.print(gateLevel);

    lcd.setCursor(0, 1);
    lcd.print("Range:");
    lcd.print(minVel);
    lcd.print("-");
    lcd.print(maxVel);

    // Clear Whadda text as well
    whadda.clearDisplay();

    // Re-init pot filter
    initPotFilter();

    // Track times
    unsigned long gateStart = millis();
    unsigned long inRangeStart = 0;
    unsigned long lastBeep = 0;

    bool wasOutOfRange = true;

    // Debug info
    Serial.print("[Gate ");
    Serial.print(gateLevel);
    Serial.print("] Range: ");
    Serial.print(minVel);
    Serial.print(" - ");
    Serial.println(maxVel);

    // Loop until success or time-out
    while (true)
    {
        // Check gate time limit
        if (millis() - gateStart > GATE_TIME_MS)
        {
            return false; // time up -> fail
        }

        // Read the pot
        int potValue = getSmoothedPotValue(gateLevel);

        // Update LCD/Whadda with speed
        updateGateDisplays(gateLevel, potValue);

        if (isPotInRange(potValue, minVel, maxVel))
        {
            // Blink LED green in sync with the short beep
            unsigned long now = millis();
            if (now - lastBeep > BEEP_INTERVAL)
            {
                buzzer.playTone(800, 50);
                rgbLed.setColor(0, 255, 0);
                lastBeep = now;
            }

            // If we were out of range previously, set the inRangeStart
            if (wasOutOfRange)
            {
                wasOutOfRange = false;
                inRangeStart = millis();
            }

            // Check if we've been in range for the required period
            if (millis() - inRangeStart >= IN_RANGE_MS)
            {
                // Gate passed
                return true;
            }
        }
        else
        {
            // Out of range
            rgbLed.setColor(255, 0, 0);

            // If we just exited range, do a short beep
            if (!wasOutOfRange)
            {
                wasOutOfRange = true;
                buzzer.playTone(300, 200);
            }
        }

        delay(50);
    }
}

// -----------------------------------------------------------------------------
// Main Challenge Function
// -----------------------------------------------------------------------------

bool runGame2()
{
    // Intro
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Escape Velocity!");
    lcd.setCursor(0, 1);
    lcd.print("Good luck!");
    delay(1500);

    // Local state
    int currentGate = 1;
    int lives = STARTING_LIVES;
    setWhaddaLives(lives);

    // Attempt each gate
    while (currentGate <= TOTAL_GATES)
    {
        bool passed = doGateAttempt(currentGate);

        if (passed)
        {
            // Short success beep sequence
            buzzer.playTone(1000, 150);
            delay(150);
            buzzer.playTone(1200, 150);
            currentGate++;
        }
        else
        {
            // Gate failed -> lose a life
            lives--;
            setWhaddaLives(lives);
            // Long beep to indicate fail
            buzzer.playTone(200, 500);

            // If no lives left, do restart effect
            if (lives <= 0)
            {
                doRestartEffect();
                currentGate = 1;
                lives = STARTING_LIVES;
                setWhaddaLives(lives);

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

    // Final "win" melody
    buzzer.playWinMelody();
    rgbLed.off();

    Serial.println("Game 2 completed!");

    return true;
}