#include "EscapeVelocity.h"
#include "Globals.h"

/**
 * @brief Constructor for the Escape Velocity game
 * 
 * Initializes all game variables to their default values.
 */
EscapeVelocity::EscapeVelocity() : state(EscVelocityState::Init),
                                   currentGate(1),
                                   lives(EscVelocityConfig::STARTING_LIVES),
                                   stateStart(0),
                                   gateResult(false),
                                   showTimerFlag(true),
                                   gateState(GateAttemptState::Init),
                                   minVel(0),
                                   maxVel(0),
                                   gateStart(0),
                                   inRangeStart(0),
                                   lastBeep(0),
                                   wasOutOfRange(true),
                                   blinkState(false),
                                   potFilter(0.0f)
{
}

/**
 * @brief Get the smoothed potentiometer value
 * 
 * Reads the potentiometer, applies mapping and smoothing filter.
 * 
 * @param gateLevel Current gate level for scaling
 * @return Smoothed and scaled potentiometer value
 */
int EscapeVelocity::getSmoothedPotValue(int gateLevel)
{
    int raw = analogRead(POT_PIN);
    int mapped = constrain(map(raw, 
                              EscVelocityConfig::POT_MIN_RAW, 
                              EscVelocityConfig::POT_MAX_RAW, 
                              EscVelocityConfig::POT_MIN_MAPPED, 
                              EscVelocityConfig::POT_MAX_MAPPED), 
                          EscVelocityConfig::POT_MIN_MAPPED, 
                          EscVelocityConfig::POT_MAX_MAPPED);
    potFilter = (1.0f - EscVelocityConfig::ALPHA) * potFilter + EscVelocityConfig::ALPHA * (float)mapped;
    return (int)(potFilter * gateLevel);
}

/**
 * @brief Initialize the potentiometer filter
 * 
 * Sets the initial filter value to the current potentiometer reading.
 */
void EscapeVelocity::initPotFilter()
{
    potFilter = (float)analogRead(POT_PIN);
}

/**
 * @brief Generate a velocity range for a gate
 * 
 * Creates a range of valid velocities that scales with gate level.
 * The range becomes more difficult to maintain as the gate level increases.
 * 
 * @param gateLevel Current gate level
 * @param minVel Output parameter for minimum velocity
 * @param maxVel Output parameter for maximum velocity
 */
void EscapeVelocity::generateVelocityRange(int gateLevel, int &minVel, int &maxVel)
{
    // Compute a non-linear maximum possible value that scales with gate level.
    long baseMax = EscVelocityConfig::BASE_MAX;
    // Adding an extra term to make the maximum grow non-linearly.
    long maxPossible = baseMax * gateLevel + (long)(50 * pow(gateLevel, 1.5));
    if (maxPossible < EscVelocityConfig::POT_MIN_POSSIBLE)
        maxPossible = EscVelocityConfig::POT_MIN_POSSIBLE;

    // Compute a normalized difficulty factor (0 at level 1, 1 at final level)
    float difficultyFactor = (float)gateLevel / (float)EscVelocityConfig::TOTAL_GATES;

    // Generate a random offset, but later blend it with the target mid–point.
    long randomOffset = random(EscVelocityConfig::RANDOM_OFFSET_MIN, 
                              maxPossible - EscVelocityConfig::RANDOM_OFFSET_PADDING);
    long targetMid = maxPossible / 2;
    // Blend the random offset and the mid point based on difficulty: higher levels lean more to the center.
    long center = (long)((1.0f - difficultyFactor) * randomOffset + difficultyFactor * targetMid);

    // Compute a dynamic window width:
    // Base window increases with level difficulty using a combination of linear and logarithmic factors.
    int baseWindow = EscVelocityConfig::BASE_WINDOW;
    float windowFactor = 1.0f + difficultyFactor * EscVelocityConfig::WINDOW_FACTOR_MULTIPLIER; // More difficult levels have a larger multiplier.
    int dynamicWindow = (int)(baseWindow * windowFactor + 10 * log(gateLevel + 1));
    const int halfRange = dynamicWindow;

    // Set minimum and maximum, ensuring they are within bounds.
    minVel = center - halfRange;
    maxVel = center + halfRange;
    if (minVel < 0)
        minVel = 0;
    if (maxVel > maxPossible)
        maxVel = maxPossible;

    // Debug output
    Serial.print("[Gate ");
    Serial.print(gateLevel);
    Serial.print("] Range: ");
    Serial.print(minVel);
    Serial.print(" - ");
    Serial.println(maxVel);
}

/**
 * @brief Update the gate displays
 * 
 * Updates the LCD and Whadda displays with current gate information.
 * 
 * @param gateLevel Current gate level
 * @param potValue Current potentiometer value
 */
void EscapeVelocity::updateGateDisplays(int gateLevel, int potValue)
{
    lcd.setCursor(0, 0);
    lcd.print("Gate ");
    lcd.print(gateLevel);
    char buff[16];
    sprintf(buff, "Spd %4d", potValue);
    whadda.displayText(buff);
}

/**
 * @brief Check if a potentiometer value is within the valid range
 * 
 * @param potValue Current potentiometer value
 * @param minVel Minimum velocity threshold
 * @param maxVel Maximum velocity threshold
 * @return true if the value is within range, false otherwise
 */
bool EscapeVelocity::isPotInRange(int potValue, int minVel, int maxVel)
{
    int minCheck = minVel - EscVelocityConfig::TOLERANCE;
    int maxCheck = maxVel + EscVelocityConfig::TOLERANCE;
    return (potValue >= minCheck && potValue <= maxCheck);
}

/**
 * @brief Set the Whadda lives display
 * 
 * Updates the Whadda LED display to show remaining lives.
 * 
 * @param lives Number of lives to display
 */
void EscapeVelocity::setWhaddaLives(int lives)
{
    // Clear all LEDs
    for (int i = 0; i < EscVelocityConfig::LED_COUNT; i++)
    {
        whadda.setLED(i, false);
    }
    // Light up the first MAX_LIVES_LED LEDs according to lives
    for (int i = 0; i < lives && i < EscVelocityConfig::MAX_LIVES_LED; i++)
    {
        whadda.setLED(i, true);
    }
}

/**
 * @brief Handle gate success
 * 
 * Processes a successful gate attempt and transitions to the next state.
 * Plays a success sound sequence and updates the game state.
 */
void EscapeVelocity::handleGateSuccess()
{
    // Gate passed – play a short beep sequence.
    buzzer.playTone(EscVelocityConfig::SUCCESS_TONE1_FREQ, EscVelocityConfig::SUCCESS_TONE1_DURATION);
    buzzer.playTone(EscVelocityConfig::SUCCESS_TONE2_FREQ, EscVelocityConfig::SUCCESS_TONE2_DURATION);
    stateStart = millis();
    state = EscVelocityState::SuccessBeep;
}

/**
 * @brief Handle gate failure
 * 
 * Processes a failed gate attempt, decrements lives, and transitions to the next state.
 * Plays a failure sound and updates the game state based on remaining lives.
 */
void EscapeVelocity::handleGateFailure()
{
    // Gate failed – lose a life.
    Serial.println("Gate failed. Current lives: " + String(lives));
    lives--;
    setWhaddaLives(lives);
    buzzer.playTone(EscVelocityConfig::FAILED_TONE_FREQ, EscVelocityConfig::FAILED_TONE_DURATION);
    if (lives <= 0)
    {
        stateStart = millis();
        state = EscVelocityState::RestartEffect;
    }
    else
    {
        stateStart = millis();
        state = EscVelocityState::FailedPause;
    }
}

/**
 * @brief Reset the game
 * 
 * Resets the game state after losing all lives.
 * Resets all game variables to their initial values.
 */
void EscapeVelocity::resetGame()
{
    lcd.clear();
    currentGate = 1;
    lives = EscVelocityConfig::STARTING_LIVES;
    // Reset the gate attempt state
    gateState = GateAttemptState::Init;
    Serial.println("SHOULD SET LIVES TO: " + String(lives));
    setWhaddaLives(lives);
    showTimer = true;
    state = EscVelocityState::GameLoop;
}

/**
 * @brief Update the current gate attempt
 * 
 * Processes the current gate attempt and updates state accordingly.
 * Handles the gate attempt state machine and visual feedback.
 * 
 * @param gateLevel Current gate level
 * @return true if the gate attempt is complete, false otherwise
 */
bool EscapeVelocity::updateGateAttempt(int gateLevel)
{
    unsigned long now = millis();

    switch (gateState)
    {
    case GateAttemptState::Init:
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

        gateState = GateAttemptState::Loop;
        return false;

    case GateAttemptState::Loop:
    {
        if (hasElapsed(gateStart, EscVelocityConfig::GATE_TIME_MS))
        {
            // Timeout: gate failed.
            gateResult = false;
            gateState = GateAttemptState::Init;
            return true;
        }
        int potValue = getSmoothedPotValue(gateLevel);
        updateGateDisplays(gateLevel, potValue);
        if (isPotInRange(potValue, minVel, maxVel))
        {
            if (hasElapsed(lastBeep, EscVelocityConfig::BEEP_INTERVAL))
            {
                blinkState = !blinkState;
                if (blinkState)
                {
                    buzzer.playTone(EscVelocityConfig::IN_RANGE_TONE_FREQ, EscVelocityConfig::IN_RANGE_TONE_DURATION);
                    rgbLed.setColor(0, 255, 0); // Green indicates in-range
                }
                else
                {
                    rgbLed.off();
                }
                lastBeep = now;
            }
            if (wasOutOfRange)
            {
                wasOutOfRange = false;
                inRangeStart = now;
            }
            if (hasElapsed(inRangeStart, EscVelocityConfig::IN_RANGE_MS))
            {
                // Maintained in-range long enough: gate passed.
                gateResult = true;
                gateState = GateAttemptState::Init;
                return true;
            }
        }
        else
        {
            rgbLed.setColor(255, 0, 0); // Red indicates out-of-range
            if (!wasOutOfRange)
            {
                wasOutOfRange = true;
                buzzer.playTone(EscVelocityConfig::OUT_OF_RANGE_TONE_FREQ, EscVelocityConfig::OUT_OF_RANGE_TONE_DURATION);
            }
        }
        return false;
    }
    }
    return false; // Fallback (should not occur)
}

/**
 * @brief Run the restart effect
 * 
 * Displays visual effects when restarting the game after losing all lives.
 * Blinks LEDs and displays a message on the LCD.
 */
void EscapeVelocity::runRestartEffect()
{
    // Disable timer display during restart effect and blink LEDs
    showTimerFlag = false;
    whadda.blinkLEDs(0xFF, 3, EscVelocityConfig::RESTART_BLINK_INTERVAL);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Out of lives...");
    whadda.displayText("Restarting...");
}

/**
 * @brief Initialize the game
 * 
 * Sets up the game state, displays, and initializes all necessary variables.
 * Plays the round start melody and sets up the initial game state.
 */
void EscapeVelocity::init()
{
    buzzer.playRoundStartMelody();
    state = EscVelocityState::Init;
    currentGate = 1;
    lives = EscVelocityConfig::STARTING_LIVES;
    setWhaddaLives(lives);
    stateStart = millis();
    showTimerFlag = true;
    // Initialize gate attempt state
    gateState = GateAttemptState::Init;
}

/**
 * @brief Run the game
 * 
 * Main game loop that handles state transitions and game logic.
 * Implements the game state machine and processes user input.
 * 
 * @return true if the game has completed, false otherwise
 */
bool EscapeVelocity::run()
{
    unsigned long now = millis();

    switch (state)
    {
    case EscVelocityState::Init:
        init();
        state = EscVelocityState::Intro;
        break;

    case EscVelocityState::Intro:
        lcd.clear();
        lcd.setCursor(0, 0);
        showTimer = false;
        lcd.print("Escape Velocity!");
        lcd.setCursor(0, 1);
        lcd.print("Good luck!");
        stateStart = now;
        state = EscVelocityState::WaitIntro;
        break;

    case EscVelocityState::WaitIntro:
        if (hasElapsed(stateStart, EscVelocityConfig::INTRO_DURATION))
        {
            state = EscVelocityState::GameLoop;
            lcd.clear();
            showTimer = true;
        }
        break;

    case EscVelocityState::GameLoop:
        if (currentGate <= EscVelocityConfig::TOTAL_GATES)
        {
            if (updateGateAttempt(currentGate))
            {
                state = EscVelocityState::ProcessGate;
            }
        }
        else
        {
            state = EscVelocityState::Finished;
        }
        break;

    case EscVelocityState::ProcessGate:
        if (gateResult)
        {
            handleGateSuccess();
        }
        else
        {
            handleGateFailure();
        }
        break;

    case EscVelocityState::SuccessBeep:
        if (hasElapsed(stateStart, EscVelocityConfig::SUCCESS_BEEP_DURATION))
        {
            currentGate++;
            state = EscVelocityState::GameLoop;
        }
        break;

    case EscVelocityState::FailedPause:
        if (hasElapsed(stateStart, EscVelocityConfig::FAILED_PAUSE_DURATION))
        {
            gateState = GateAttemptState::Init;
            state = EscVelocityState::GameLoop;
        }
        break;

    case EscVelocityState::RestartEffect:
        runRestartEffect();
        if (hasElapsed(stateStart, EscVelocityConfig::RESTART_EFFECT_DURATION))
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            showTimer = false;
            lcd.print("Retrying...");
            stateStart = now;
            state = EscVelocityState::Retry;
        }
        break;

    case EscVelocityState::Retry:
        if (hasElapsed(stateStart, EscVelocityConfig::RETRY_DURATION))
        {
            resetGame();
        }
        break;

    case EscVelocityState::Finished:
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
