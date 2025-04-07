#include "Globals.h"
#include "ArcheryChallenge.h"


ArcheryChallenge::ArcheryChallenge()
    : state(ArcheryState::Init),
      currentRound(1),
      roundResult(false),
      stateStart(0),
      roundState(RoundAttemptState::Init),
      arrowCount(0),
      currentEffect(ArcheryEffect::Winds),
      targetValue(0),
      tolerance(0),
      shieldActive(false),
      targetVisible(true),
      lastShieldToggle(0),
      lastEffectToggle(0),
      feedbackStart(0),
      prevButtonState(false)
{
}

void ArcheryChallenge::init()
{
    // Play a short melody to signal the challenge start
    buzzer.playRoundStartMelody();
    // Initialize game variables
    resetGameState();
    // Reset displays
    whadda.clearDisplay();
    rgbLed.off();
    // Ensure timer is shown by default during gameplay (will be turned off during special messages)
    showTimer = true;
}

bool ArcheryChallenge::run()
{
    unsigned long now = millis();

    switch (state)
    {
    case ArcheryState::Init:
        // Initialize game and move to intro state
        init();
        state = ArcheryState::Intro;
        break;

    case ArcheryState::Intro:
        // Display introductory message for the Archery Challenge
        displayLcdMessage("Archery Challenge", "Ready your bow!", true);
        stateStart = now;
        state = ArcheryState::WaitIntro;
        break;

    case ArcheryState::WaitIntro:
        // Wait for the intro message to display for a fixed duration
        if (millis() - stateStart >= ArcheryConfig::INTRO_DURATION)
        {
            // After intro, clear screen and enable timer display
            lcd.clear();
            showTimer = true;
            state = ArcheryState::GameLoop;
        }
        break;

    case ArcheryState::GameLoop:
        // Handle rounds sequentially
        if (currentRound <= ArcheryConfig::TOTAL_ROUNDS)
        {
            // Run the current round attempt; if it returns true, the round ended (hit or out of arrows)
            if (updateRoundAttempt(currentRound))
            {
                state = ArcheryState::ProcessRound;
            }
        }
        else
        {
            // All rounds completed successfully
            state = ArcheryState::Finished;
        }
        break;

    case ArcheryState::ProcessRound:
        if (roundResult)
        {
            // The player hit the target and won the round
            displayHitFeedback(currentRound);
            stateStart = now;
            state = ArcheryState::RoundSuccess;
        }
        else
        {
            // The player failed to hit the target in this round (out of arrows)
            Serial.println("Round " + String(currentRound) + " failed. Restarting challenge.");
            // Sound a failure tone
            playFailSound();
            stateStart = now;
            state = ArcheryState::RestartEffect;
        }
        break;

    case ArcheryState::RoundSuccess:
        // Wait for a short duration after a successful hit
        if (millis() - stateStart >= ArcheryConfig::SUCCESS_DISPLAY_DURATION)
        {
            rgbLed.off();
            lcd.clear();
            currentRound++;
            showTimer = true;
            roundState = RoundAttemptState::Init;

            // If we just finished the final round, mark as finished; otherwise continue game loop
            if (currentRound > ArcheryConfig::TOTAL_ROUNDS)
            {
                state = ArcheryState::Finished;
            }
            else
            {
                state = ArcheryState::GameLoop;
            }
        }
        break;

    case ArcheryState::RestartEffect:
        // Show restart (failure) effect: blink LEDs and display "Out of arrows..."
        runRestartEffect();
        if (millis() - stateStart >= ArcheryConfig::RESTART_EFFECT_DURATION)
        {
            displayRetryMessage();
            stateStart = now;
            state = ArcheryState::Retry;
        }
        break;

    case ArcheryState::Retry:
        // Short pause before restarting the challenge from Round 1
        if (millis() - stateStart >= ArcheryConfig::RETRY_DURATION)
        {
            // Reset game variables for a fresh start (but skip the intro this time)
            lcd.clear();
            resetGameState();
            // Reset displays
            whadda.clearDisplay();
            rgbLed.off();
            showTimer = true;
            state = ArcheryState::GameLoop;
        }
        break;

    case ArcheryState::Finished:
        // All rounds complete – challenge success
        displayFinishedMessage();
        return true; // Signal to main that this challenge is finished
    }

    return false; // Challenge not yet complete
}

/**
 * @brief Runs one update of the current round attempt state machine.
 *
 * @param roundLevel The current round number (1-indexed).
 * @return true if the round ended (target hit or out of arrows), false if still in progress.
 */
bool ArcheryChallenge::updateRoundAttempt(int roundLevel)
{
    unsigned long now = millis();
    bool currentState = isButtonPressed();

    switch (roundState)
    {
    case RoundAttemptState::Init:
    {
        // ** Initialize a new round **
        resetRoundState();
        
        // Set tolerance based on round level
        tolerance = getToleranceForRound(roundLevel);
        
        // Generate random target value
        targetValue = generateRandomTarget();

        // Print target value for debugging
        Serial.println("Round " + String(roundLevel) + " target value: " + String(targetValue));

        // Select random magical effect
        currentEffect = selectRandomEffect();

        // Initialize effect-related variables
        shieldActive = false;
        targetVisible = true;
        lastShieldToggle = now;
        lastEffectToggle = now;
        feedbackStart = 0;

        // Display round info
        displayRoundInfo(roundLevel);
        displayEffectInfo(currentEffect);

        // Setup display based on effect
        whadda.clearDisplay();
        setWhaddaArrows(ArcheryConfig::ARROWS_PER_ROUND);

        // If the effect is "Disappearing Target", use an LED to represent the target visibility
        if (currentEffect == ArcheryEffect::Disappear)
        {
            whadda.setLED(ArcheryConfig::TARGET_INDICATOR_LED, true);
        }
        
        // If the effect is "Shield", we'll indicate shield status with the RGB LED
        rgbLed.off();

        roundState = RoundAttemptState::Playing;
        prevButtonState = currentState;
        return false;
    }

    case RoundAttemptState::Playing:
    {
        // ** Main loop for aiming and shooting in the round **

        // Apply magical effects dynamics
        if (currentEffect == ArcheryEffect::Shield)
        {
            handleShieldEffect(now);
        }

        if (currentEffect == ArcheryEffect::Disappear)
        {
            handleDisappearEffect(now);
        }

        // Check for a button press to fire an arrow (rising edge detection)
        if (!prevButtonState && currentState)
        {
            // ** Fire the arrow **
            fireArrow();
            
            // Read potentiometer and check for hit
            int potValue = readPotentiometer();
            bool hit = checkHit(potValue);

            Serial.println("Arrow fired! Pot value: " + String(potValue) + ", Hit: " + String(hit));

            // Process hit with shield effect
            bool shieldBlocked = false;
            if (currentEffect == ArcheryEffect::Shield && shieldActive)
            {
                // If shield is active, a would-be hit is blocked
                if (hit)
                {
                    shieldBlocked = true;
                    hit = false;
                }
            }

            // Update the arrows-left LED indicator
            updateArrowsDisplay();

            if (hit)
            {
                // *** Target HIT ***
                roundResult = true;
                setSuccessLed();
                roundState = RoundAttemptState::Init;
                return true;
            }
            else
            {
                // *** Missed the target ***
                displayMissFeedback(shieldBlocked, targetVisible, potValue);

                if (arrowCount >= ArcheryConfig::ARROWS_PER_ROUND)
                {
                    // No arrows remaining – round failed
                    roundResult = false;
                    roundState = RoundAttemptState::Init;
                    return true;
                }
                else
                {
                    // Arrows remain – go into feedback pause before next shot
                    showTimer = false;
                    feedbackStart = now;
                    roundState = RoundAttemptState::Feedback;
                }
            }
        }

        // Update previous button state for next iteration
        prevButtonState = currentState;
        return false;
    }

    case RoundAttemptState::Feedback:
        // ** Pausing to show feedback after a miss **
        if (millis() - feedbackStart >= ArcheryConfig::FEEDBACK_DURATION)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Round ");
            lcd.print(roundLevel);
            lcd.setCursor(0, 1);
            lcd.print("Aim and Fire!");
            rgbLed.off();
            showTimer = true;
            roundState = RoundAttemptState::Playing;
        }
        prevButtonState = currentState;
        return false;
    }

    // Fallback
    prevButtonState = currentState;
    return false;
}

/**
 * @brief Sets the Whadda LED display to indicate a given number of arrows remaining.
 *
 * Lights the first N LEDs (out of 8) to represent arrows left.
 * Turns off the rest.
 *
 * @param arrowsLeft Number of arrows remaining to display.
 */
void ArcheryChallenge::setWhaddaArrows(int arrowsLeft)
{
    for (int i = 0; i < ArcheryConfig::MAX_LEDS; ++i)
    {
        whadda.setLED(i, false);
    }

    for (int i = 0; i < arrowsLeft && i < ArcheryConfig::ARROWS_PER_ROUND; ++i)
    {
        whadda.setLED(i, true);
    }
}

/**
 * @brief Runs the "out of arrows" restart effect.
 *
 * Blinks all LEDs on the Whadda display and shows a message indicating the challenge will restart.
 */
void ArcheryChallenge::runRestartEffect()
{
    showTimer = false;
    whadda.blinkLEDs(0xFF, 3, ArcheryConfig::RESTART_BLINK_INTERVAL);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Out of arrows...");
    lcd.setCursor(0, 1);
    lcd.print("Try again!");
    whadda.displayText("RESTART");
}

// ===== Helper Methods =====

void ArcheryChallenge::displayLcdMessage(const char* line1, const char* line2, bool hideTimer)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    showTimer = !hideTimer;
    lcd.print(line1);
    
    if (line2 != nullptr)
    {
        lcd.setCursor(0, 1);
        lcd.print(line2);
    }
}

void ArcheryChallenge::displayRoundInfo(int roundLevel)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Round ");
    lcd.print(roundLevel);
}

void ArcheryChallenge::displayEffectInfo(ArcheryEffect effect)
{
    lcd.setCursor(0, 1);
    
    if (effect == ArcheryEffect::Winds)
    {
        lcd.print("Shifting Winds!");
    }
    else if (effect == ArcheryEffect::Disappear)
    {
        lcd.print("Target flickers!");
    }
    else if (effect == ArcheryEffect::Shield)
    {
        lcd.print("Magic Shield!");
    }
}

void ArcheryChallenge::displayHitFeedback(int roundLevel)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    showTimer = false;
    lcd.print("Hit! Round ");
    lcd.print(roundLevel);
    lcd.setCursor(0, 1);
    lcd.print("Target ");
    lcd.print(" clear!");
    
    // Play a celebratory tone for the hit
    playHitSound();
    setSuccessLed();
}

void ArcheryChallenge::displayMissFeedback(bool shieldBlocked, bool targetVisible, int potValue)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    
    if (shieldBlocked)
    {
        lcd.print("Blocked by Shield!");
        playShieldBlockSound();
    }
    else if (targetVisible)
    {
        // Indicate if the shot was too high or too low
        if (potValue > targetValue)
        {
            lcd.print("Too high!");
        }
        else
        {
            lcd.print("Too low!");
        }
        playMissSound();
    }
    else
    {
        lcd.print("Target invisible!");
        playMissSound();
    }

    // Set LED feedback for miss (red for normal miss, blue already indicated for shield)
    if (!shieldBlocked)
    {
        setMissLed();
    }
}

void ArcheryChallenge::displayRetryMessage()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    showTimer = false;
    lcd.print("Out of arrows...");
    lcd.setCursor(0, 1);
    lcd.print("Restarting");
}

void ArcheryChallenge::displayFinishedMessage()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Challenge Done!");
    buzzer.playWinMelody();
    rgbLed.off();
    Serial.println("Game 3 completed!");
}

void ArcheryChallenge::resetGameState()
{
    state = ArcheryState::Init;
    currentRound = 1;
    roundResult = false;
    roundState = RoundAttemptState::Init;
    arrowCount = 0;
    prevButtonState = false;
}

void ArcheryChallenge::resetRoundState()
{
    arrowCount = 0;
    roundResult = false;
}

int ArcheryChallenge::getToleranceForRound(int roundLevel)
{
    switch (roundLevel)
    {
    case 1:
        return ArcheryConfig::TOLERANCE_ROUND1;
    case 2:
        return ArcheryConfig::TOLERANCE_ROUND2;
    case 3:
        return ArcheryConfig::TOLERANCE_ROUND3;
    default:
        return ArcheryConfig::TOLERANCE_ROUND1;
    }
}

int ArcheryChallenge::generateRandomTarget()
{
    // Set a random target value (simulating target distance/angle)
    int rawTarget = random(0, 1023);
    
    // Avoid extremes for safety: ensure target is within safe range
    if (rawTarget < ArcheryConfig::TARGET_MIN_SAFE)
        rawTarget = ArcheryConfig::TARGET_MIN_SAFE;
    if (rawTarget > ArcheryConfig::TARGET_MAX_SAFE)
        rawTarget = ArcheryConfig::TARGET_MAX_SAFE;
        
    return rawTarget;
}

ArcheryEffect ArcheryChallenge::selectRandomEffect()
{
    // Randomly select a magical effect for this round
    int effectIndex = random(0, 3); // 0: Winds, 1: Disappear, 2: Shield
    return static_cast<ArcheryEffect>(effectIndex);
}

bool ArcheryChallenge::isButtonPressed()
{
    return digitalRead(BTN_PIN) == LOW;
}

int ArcheryChallenge::readPotentiometer()
{
    int raw = analogRead(POT_PIN);
    return constrain(map(raw, 
                         ArcheryConfig::POT_MIN_RAW, 
                         ArcheryConfig::POT_MAX_RAW, 
                         ArcheryConfig::POT_MIN_MAPPED, 
                         ArcheryConfig::POT_MAX_MAPPED), 
                    ArcheryConfig::POT_MIN_MAPPED, 
                    ArcheryConfig::POT_MAX_MAPPED);
}

bool ArcheryChallenge::checkHit(int potValue)
{
    return (abs(potValue - targetValue) <= tolerance);
}

void ArcheryChallenge::handleShieldEffect(unsigned long now)
{
    // Toggle shield on and off based on time intervals
    if (shieldActive)
    {
        if (now - lastShieldToggle >= ArcheryConfig::SHIELD_UP_MS)
        {
            shieldActive = false;
            lastShieldToggle = now;
            // Shield goes down: turn off blue indicator
            rgbLed.off();
        }
    }
    else
    {
        if (now - lastShieldToggle >= ArcheryConfig::SHIELD_DOWN_MS)
        {
            shieldActive = true;
            lastShieldToggle = now;
            // Shield comes up: indicate with blue LED
            setShieldLed();
        }
    }
}

void ArcheryChallenge::handleDisappearEffect(unsigned long now)
{
    // Toggle target visibility (LED 7) at set intervals (mostly visible, brief off)
    if (targetVisible)
    {
        if (now - lastEffectToggle >= ArcheryConfig::TARGET_VISIBLE_MS)
        {
            targetVisible = false;
            lastEffectToggle = now;
            whadda.setLED(ArcheryConfig::TARGET_INDICATOR_LED, false);
        }
    }
    else
    {
        if (now - lastEffectToggle >= ArcheryConfig::TARGET_INVISIBLE_MS)
        {
            targetVisible = true;
            lastEffectToggle = now;
            whadda.setLED(ArcheryConfig::TARGET_INDICATOR_LED, true);
        }
    }
}

void ArcheryChallenge::fireArrow()
{
    arrowCount++;
    buzzer.playTone(ArcheryConfig::ARROW_FIRE_FREQ, ArcheryConfig::ARROW_FIRE_DURATION);
}

void ArcheryChallenge::updateArrowsDisplay()
{
    int arrowsLeft = ArcheryConfig::ARROWS_PER_ROUND - arrowCount;
    setWhaddaArrows(arrowsLeft);
}

void ArcheryChallenge::playHitSound()
{
    buzzer.playTone(ArcheryConfig::HIT_FREQ_1, ArcheryConfig::HIT_DURATION);
    buzzer.playTone(ArcheryConfig::HIT_FREQ_2, ArcheryConfig::HIT_DURATION);
}

void ArcheryChallenge::playMissSound()
{
    buzzer.playTone(ArcheryConfig::MISS_FREQ, ArcheryConfig::MISS_DURATION);
}

void ArcheryChallenge::playShieldBlockSound()
{
    buzzer.playTone(ArcheryConfig::SHIELD_BLOCK_FREQ, ArcheryConfig::SHIELD_BLOCK_DURATION);
}

void ArcheryChallenge::playFailSound()
{
    buzzer.playTone(ArcheryConfig::FAIL_FREQ, ArcheryConfig::FAIL_DURATION);
}

void ArcheryChallenge::setSuccessLed()
{
    rgbLed.setColor(0, 255, 0); // Green LED for success
}

void ArcheryChallenge::setMissLed()
{
    rgbLed.setColor(255, 0, 0); // Red LED for failure
}

void ArcheryChallenge::setShieldLed()
{
    rgbLed.setColor(0, 0, 255); // Blue LED for shield
}


