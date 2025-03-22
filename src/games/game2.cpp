#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Game2.h"

// TODO - Clean up includes and external references
// TODO - documentation

// External hardware interfaces
extern LiquidCrystal_I2C lcd;
extern RGBLed rgbLed;
extern Buzzer buzzer;
extern Whadda whadda;
extern bool showTimer;

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

int EscapeVelocity::getSmoothedPotValue(int gateLevel)
{
    int raw = analogRead(POT_PIN);
    int mapped = constrain(map(raw, 300, 1023, 0, 1023), 0, 1023);
    potFilter = (1.0f - EscVelocityConfig::ALPHA) * potFilter + EscVelocityConfig::ALPHA * (float)mapped;
    return (int)(potFilter * gateLevel);
}

void EscapeVelocity::initPotFilter()
{
    potFilter = (float)analogRead(POT_PIN);
}

void EscapeVelocity::generateVelocityRange(int gateLevel, int &minVel, int &maxVel)
{
    long maxPossible = 1023L * gateLevel;
    if (maxPossible < 100)
        maxPossible = 100;
    long center = random(50, maxPossible - 50);
    const int halfRange = 30;
    minVel = center - halfRange;
    maxVel = center + halfRange;
    if (minVel < 0)
        minVel = 0;
}

void EscapeVelocity::updateGateDisplays(int gateLevel, int potValue)
{
    lcd.setCursor(0, 0);
    lcd.print("Gate ");
    lcd.print(gateLevel);
    char buff[16];
    sprintf(buff, "Spd %4d", potValue);
    whadda.displayText(buff);
}

bool EscapeVelocity::isPotInRange(int potValue, int minVel, int maxVel)
{
    int minCheck = minVel - EscVelocityConfig::TOLERANCE;
    int maxCheck = maxVel + EscVelocityConfig::TOLERANCE;
    return (potValue >= minCheck && potValue <= maxCheck);
}

void EscapeVelocity::setWhaddaLives(int lives)
{
    // Clear all 8 LEDs
    for (int i = 0; i < 8; i++)
    {
        whadda.setLED(i, false);
    }
    // Light up the first 3 LEDs according to lives
    for (int i = 0; i < lives && i < 3; i++)
    {
        whadda.setLED(i, true);
    }
}

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
        // Debug output
        Serial.print("[Gate ");
        Serial.print(gateLevel);
        Serial.print("] Range: ");
        Serial.print(minVel);
        Serial.print(" - ");
        Serial.println(maxVel);
        gateState = GateAttemptState::Loop;
        return false;

    case GateAttemptState::Loop:
    {
        if (now - gateStart > EscVelocityConfig::GATE_TIME_MS)
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
            if (now - lastBeep >= EscVelocityConfig::BEEP_INTERVAL)
            {
                blinkState = !blinkState;
                if (blinkState)
                {
                    buzzer.playTone(800, 50);
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
            if (now - inRangeStart >= EscVelocityConfig::IN_RANGE_MS)
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
                buzzer.playTone(300, 200);
            }
        }
        return false;
    }
    }
    return false; // Fallback (should not occur)
}

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
            // Gate passed – play a short beep sequence.
            buzzer.playTone(1000, 150);
            buzzer.playTone(1200, 150);
            stateStart = now;
            state = EscVelocityState::SuccessBeep;
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
                stateStart = now;
                state = EscVelocityState::RestartEffect;
            }
            else
            {
                stateStart = now;
                state = EscVelocityState::FailedPause;
            }
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
            lcd.clear();
            currentGate = 1;
            lives = EscVelocityConfig::STARTING_LIVES;
            state = EscVelocityState::GameLoop;
            Serial.println("SHOULD SET LIVES TO: " + String(lives));
            setWhaddaLives(lives);
            showTimer = true;
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

// Global instance and runner function
EscapeVelocity escVelocity;

bool runGame2()
{
    return escVelocity.run();
}
