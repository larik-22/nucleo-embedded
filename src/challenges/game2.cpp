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
static const int STARTING_LIVES = 3;             // Initial lives
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

// -----------------------------------------------------------------------------
// Gate Attempt Logic
// -----------------------------------------------------------------------------
/**
 * @brief Updates LCD (top row) with the current pot speed and updates Whadda display.
 *        (You may wish to use fixed‑width strings to avoid leftover characters.)
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
 */
static bool isPotInRange(int potValue, int minVel, int maxVel)
{
    int minCheck = minVel - TOLERANCE;
    int maxCheck = maxVel + TOLERANCE;
    return (potValue >= minCheck && potValue <= maxCheck);
}

/**
 * @brief Non-blocking gate attempt update.
 *        Call repeatedly until it returns true. When finished, 'gatePassed' holds the result.
 */
bool doGateAttemptUpdate(int gateLevel, bool &gatePassed)
{
    enum GateState
    {
        GA_INIT,
        GA_LOOP,
    };
    static GateState gaState = GA_INIT;
    static int minVel, maxVel;
    static unsigned long gateStart, inRangeStart, lastBeep;
    static bool wasOutOfRange;
    // Added blink state for LED toggling:
    static bool blinkState = false;

    unsigned long now = millis();

    switch (gaState)
    {
    case GA_INIT:
        // Initialize a new gate attempt.
        generateVelocityRange(gateLevel, minVel, maxVel);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Gate ");
        lcd.print(gateLevel);
        lcd.setCursor(0, 1);
        lcd.print("Range:");
        lcd.print(minVel);
        lcd.print("-");
        lcd.print(maxVel);
        whadda.clearDisplay();
        initPotFilter();
        gateStart = now;
        inRangeStart = now;
        lastBeep = now;
        wasOutOfRange = true;
        blinkState = false;

        // DEBUG: print range info
        Serial.print("[Gate ");
        Serial.print(gateLevel);
        Serial.print("] Range: ");
        Serial.print(minVel);
        Serial.print(" - ");
        Serial.println(maxVel);
        gaState = GA_LOOP;
        return false; // Not finished yet

    case GA_LOOP:
    {
        // Check for time-out.
        if (now - gateStart > GATE_TIME_MS)
        {
            gatePassed = false;
            gaState = GA_INIT;
            return true;
        }
        // Read the potentiometer and update displays.
        int potValue = getSmoothedPotValue(gateLevel);
        updateGateDisplays(gateLevel, potValue);
        if (isPotInRange(potValue, minVel, maxVel))
        {
            // Toggle the LED to blink.
            if (now - lastBeep >= BEEP_INTERVAL)
            {
                blinkState = !blinkState;
                if (blinkState)
                {
                    // LED ON in green and play a short beep.
                    buzzer.playTone(800, 50);
                    rgbLed.setColor(0, 255, 0);
                }
                else
                {
                    // LED OFF.
                    rgbLed.off();
                }
                lastBeep = now;
            }
            if (wasOutOfRange)
            {
                wasOutOfRange = false;
                inRangeStart = now;
            }
            if (now - inRangeStart >= IN_RANGE_MS)
            {
                // Successfully maintained in-range for the required time.
                gatePassed = true;
                gaState = GA_INIT;
                return true;
            }
        }
        else
        {
            // Out-of-range condition: show a steady red color.
            rgbLed.setColor(255, 0, 0);
            if (!wasOutOfRange)
            {
                wasOutOfRange = true;
                buzzer.playTone(300, 200);
            }
        }
        return false;
    }
    }
    return false;
}

// -----------------------------------------------------------------------------
// Non-Blocking Main Challenge Function
// -----------------------------------------------------------------------------

/**
 * @brief Non-blocking game2 state machine.
 *        Call runGame2() repeatedly until it returns true.
 *
 * Changes:
 *  - Uses millis() for waiting (pausing) to show messages.
 *  - Uses global flag showTimer to hide the timer when printing full-row messages.
 *  - Implements a dedicated restart effect state (R2_RESTART_EFFECT) to ensure it plays once.
 */
bool runGame2()
{
    enum GameState
    {
        R2_INIT,
        R2_INTRO,
        R2_WAIT_INTRO,
        R2_GAME_LOOP,
        R2_PROCESS_GATE,
        R2_SUCCESS_BEEP,
        R2_FAILED_PAUSE,
        R2_RESTART_EFFECT,
        R2_RETRY,
        R2_FINISHED
    };

    static GameState state = R2_INIT;
    static int currentGate = 1;
    static int lives = STARTING_LIVES;
    static unsigned long stateStart = 0;
    static bool gateResult = false;

    switch (state)
    {
    case R2_INIT:
        // Initialize game state.
        currentGate = 1;
        lives = STARTING_LIVES;
        setWhaddaLives(lives);
        state = R2_INTRO;
        break;

    case R2_INTRO:
        lcd.clear();
        lcd.setCursor(0, 0);
        showTimer = false; // hide timer during full message
        lcd.print("Escape Velocity!");
        lcd.setCursor(0, 1);
        lcd.print("Good luck!");
        stateStart = millis();
        state = R2_WAIT_INTRO;
        break;

    case R2_WAIT_INTRO:
        if (millis() - stateStart >= 1500)
        {
            state = R2_GAME_LOOP;
            lcd.clear();
            showTimer = true; // re-enable timer updates
        }
        break;

    case R2_GAME_LOOP:
        if (currentGate <= TOTAL_GATES)
        {
            // Process the current gate attempt non-blockingly.
            if (doGateAttemptUpdate(currentGate, gateResult))
            {
                state = R2_PROCESS_GATE;
            }
        }
        else
        {
            state = R2_FINISHED;
        }
        break;

    case R2_PROCESS_GATE:
        if (gateResult)
        {
            // Gate passed – play a short success beep sequence.
            buzzer.playTone(1000, 150);
            buzzer.playTone(1200, 150);
            stateStart = millis();
            state = R2_SUCCESS_BEEP;
        }
        else
        {
            // Gate failed – lose a life.
            Serial.println("Gate failed. Current lives: " + String(lives));
            lives--;
            setWhaddaLives(lives);
            buzzer.playTone(200, 500);
            if (lives <= 0)
            {
                // Out of lives: move to restart effect state.
                stateStart = millis();
                state = R2_RESTART_EFFECT;
            }
            else
            {
                // NEW: Pause briefly before retrying the same gate.
                stateStart = millis();
                state = R2_FAILED_PAUSE;
            }
        }
        break;

    case R2_SUCCESS_BEEP:
        if (millis() - stateStart >= 300)
        {
            currentGate++;
            state = R2_GAME_LOOP;
        }
        break;

    case R2_FAILED_PAUSE:
        // Wait a moment (1000ms) after a failure before reattempting.
        if (millis() - stateStart >= 1000)
        {
            state = R2_GAME_LOOP;
        }
        break;

    case R2_RESTART_EFFECT:
    {
        // Clean restart effect: blink Whadda visuals without interfering with other states.
        const unsigned long BLINK_INTERVAL = 200;
        const unsigned long EFFECT_DURATION = 1500;
        unsigned long elapsed = millis() - stateStart;

        // Ensure timer updates are disabled during effect.
        showTimer = false;

        // Blink Whadda's LEDs.
        whadda.blinkLEDs(0xFF, 3, BLINK_INTERVAL);

        // Update LCD once per cycle.
        showTimer = false; // hide timer during full message
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Out of lives...");
        whadda.clearDisplay();
        whadda.displayText("Restarting...");

        if (elapsed >= EFFECT_DURATION)
        {

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Retrying...");
            stateStart = millis();
            state = R2_RETRY;
        }
        break;
    }
    case R2_RETRY:
        if (millis() - stateStart >= 1000)
        {
            lcd.clear();
            // show current lives
            currentGate = 1;
            lives = STARTING_LIVES;
            setWhaddaLives(lives);
            state = R2_GAME_LOOP;
            showTimer = true; // re-enable timer updates
        }
        break;

    case R2_FINISHED:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Challenge Done!");
        buzzer.playWinMelody();
        rgbLed.off();
        Serial.println("Game 2 completed!");
        return true;
    }

    return false;
}
