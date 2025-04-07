#include "Globals.h"
#include "ArcheryChallenge.h"

// TODO: Correct button class usage instead of pin
// TODO: Remove disappear target effect (useless) –≥ and add another effect instead
// TODO: Split more into functions

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
    state = ArcheryState::Init;
    currentRound = 1;
    roundResult = false;
    roundState = RoundAttemptState::Init;
    arrowCount = 0;
    prevButtonState = false;
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
        lcd.clear();
        lcd.setCursor(0, 0);
        showTimer = false; // Hide timer during custom intro message
        lcd.print("Archery Challenge");
        lcd.setCursor(0, 1);
        lcd.print("Ready your bow!");
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
            // Provide a success indication (short beep and message) before moving to the next round
            lcd.clear();
            lcd.setCursor(0, 0);
            showTimer = false;
            lcd.print("Hit! Round ");
            lcd.print(currentRound);
            lcd.setCursor(0, 1);
            lcd.print("Target ");
            lcd.print(" clear!");
            // Play a celebratory tone for the hit
            buzzer.playTone(1000, 150);
            buzzer.playTone(1200, 150);
            rgbLed.setColor(0, 255, 0); // Green LED for success
            stateStart = now;
            state = ArcheryState::RoundSuccess;
        }
        else
        {
            // The player failed to hit the target in this round (out of arrows)
            Serial.println("Round " + String(currentRound) + " failed. Restarting challenge.");
            // Sound a failure tone
            buzzer.playTone(200, 500);
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
            lcd.clear();
            lcd.setCursor(0, 0);
            showTimer = false;
            lcd.print("Out of arrows...");
            lcd.setCursor(0, 1);
            lcd.print("Restarting");
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
            currentRound = 1;
            roundState = RoundAttemptState::Init;
            arrowCount = 0;
            prevButtonState = false;
            // Reset displays
            whadda.clearDisplay();
            rgbLed.off();
            showTimer = true;
            state = ArcheryState::GameLoop;
        }
        break;

    case ArcheryState::Finished:
        // All rounds complete – challenge success
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Challenge Done!");
        buzzer.playWinMelody();
        rgbLed.off();
        Serial.println("Game 3 completed!");
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
    bool currentState = digitalRead(BTN_PIN) == LOW ? true : false;

    switch (roundState)
    {
    case RoundAttemptState::Init:
    {
        // ** Initialize a new round **
        arrowCount = 0;
        roundResult = false;
        switch (roundLevel)
        {
        case 1:
            tolerance = ArcheryConfig::TOLERANCE_ROUND1;
            break;
        case 2:
            tolerance = ArcheryConfig::TOLERANCE_ROUND2;
            break;
        case 3:
            tolerance = ArcheryConfig::TOLERANCE_ROUND3;
            break;
        }
        // Set a random target value (simulating target distance/angle)
        int rawTarget = random(0, 1023);
        // Avoid extremes for safety (optional): ensure target is within [100, 923] range
        if (rawTarget < 100)
            rawTarget = 100;
        if (rawTarget > 923)
            rawTarget = 923;
        targetValue = rawTarget;

        // Print target value for debugging
        Serial.println("Round " + String(roundLevel) + " target value: " + String(targetValue));

        // Randomly select a magical effect for this round
        int effectIndex = random(0, 3); // 0: Winds, 1: Disappear, 2: Shield
        currentEffect = static_cast<ArcheryEffect>(effectIndex);

        // Initialize effect-related variables
        shieldActive = false;
        targetVisible = true;
        lastShieldToggle = now;
        lastEffectToggle = now;
        feedbackStart = 0;

        // Display round info on LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Round ");
        lcd.print(roundLevel);
        lcd.setCursor(0, 1);

        if (currentEffect == ArcheryEffect::Winds)
        {
            lcd.print("Shifting Winds!");
        }
        else if (currentEffect == ArcheryEffect::Disappear)
        {
            lcd.print("Target flickers!");
        }
        else if (currentEffect == ArcheryEffect::Shield)
        {
            lcd.print("Magic Shield!");
        }

        whadda.clearDisplay();
        setWhaddaArrows(ArcheryConfig::ARROWS_PER_ROUND);

        // If the effect is "Disappearing Target", use an LED to represent the target visibility
        // We will use the last LED (index 7) as a target indicator (on = visible, off = invisible)
        if (currentEffect == ArcheryEffect::Disappear)
        {
            whadda.setLED(7, true);
        }
        // If the effect is "Shield", we'll indicate shield status with the RGB LED (blue when shield is up)
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
                    rgbLed.setColor(0, 0, 255);
                }
            }
        }

        if (currentEffect == ArcheryEffect::Disappear)
        {
            // Toggle target visibility (LED 7) at set intervals (mostly visible, brief off)
            if (targetVisible)
            {
                if (now - lastEffectToggle >= ArcheryConfig::TARGET_VISIBLE_MS)
                {
                    targetVisible = false;
                    lastEffectToggle = now;
                    whadda.setLED(7, false);
                }
            }
            else
            {
                if (now - lastEffectToggle >= ArcheryConfig::TARGET_INVISIBLE_MS)
                {
                    targetVisible = true;
                    lastEffectToggle = now;
                    whadda.setLED(7, true);
                }
            }
        }

        // (Shifting Winds effect does not require continuous action; it adjusts target after each shot)
        // Check for a button press to fire an arrow (rising edge detection)
        if (!prevButtonState && currentState)
        {
            // ** Fire the arrow **
            arrowCount++;
            buzzer.playTone(400, 50);
            int raw = analogRead(POT_PIN);
            int potValue = constrain(map(raw, 300, 1023, 0, 1023), 0, 1023);
            bool hit = (abs(potValue - targetValue) <= tolerance);

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
            int arrowsLeft = ArcheryConfig::ARROWS_PER_ROUND - arrowCount;
            setWhaddaArrows(arrowsLeft);

            if (hit)
            {
                // *** Target HIT ***
                roundResult = true;
                rgbLed.setColor(0, 255, 0);
                roundState = RoundAttemptState::Init;
                return true;
            }
            else
            {
                // *** Missed the target ***
                lcd.clear();
                lcd.setCursor(0, 0);
                if (shieldBlocked)
                {
                    lcd.print("Blocked by Shield!");
                    buzzer.playTone(1000, 100);
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
                    buzzer.playTone(300, 150);
                }
                else
                {
                    lcd.print("Target invisible!");
                    buzzer.playTone(300, 150);
                }

                // Set LED feedback for miss (red for normal miss, blue already indicated for shield)
                if (!shieldBlocked)
                {
                    rgbLed.setColor(255, 0, 0); // Red LED for a miss
                }

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
    for (int i = 0; i < 8; ++i)
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
