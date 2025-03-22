#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <RGBLed.h>
#include <Buzzer.h>
#include <Whadda.h>
#include <Pins.h>
#include <Game2.h>

// External hardware interfaces
extern LiquidCrystal_I2C lcd;
extern RGBLed rgbLed;
extern Buzzer buzzer;
extern Whadda whadda;

// Global flag for LCD timer display (used by original code)
extern bool showTimer;

namespace Game2Config
{
    constexpr int TOTAL_GATES = 5;                // Number of gates to pass
    constexpr int STARTING_LIVES = 3;             // Initial lives
    constexpr unsigned long GATE_TIME_MS = 10000; // Allowed time per gate
    constexpr unsigned long IN_RANGE_MS = 2500;   // Must stay in range for this duration
    constexpr unsigned long BEEP_INTERVAL = 250;  // Gap between short beeps (ms)
    constexpr float ALPHA = 0.1f;                 // Exponential smoothing factor
    constexpr int TOLERANCE = 5;                  // Threshold to reduce flicker issues

    constexpr unsigned long INTRO_DURATION = 1500;
    constexpr unsigned long SUCCESS_BEEP_DURATION = 300;
    constexpr unsigned long FAILED_PAUSE_DURATION = 1000;
    constexpr unsigned long RETRY_DURATION = 1000;
    constexpr unsigned long RESTART_EFFECT_DURATION = 1500;
    constexpr unsigned long RESTART_BLINK_INTERVAL = 200;
}

// Internal state for gate attempt
enum class GateAttemptState
{
    Init,
    Loop
};

enum class Game2State
{
    Init,
    Intro,
    WaitIntro,
    GameLoop,
    ProcessGate,
    SuccessBeep,
    FailedPause,
    RestartEffect,
    Retry,
    Finished
};

class MemoryGame2
{
public:
    MemoryGame2();
    void init();
    bool run(); // Call repeatedly until the game finishes (returns true)

private:
    // Overall game state machine
    Game2State state;
    int currentGate;
    int lives;
    unsigned long stateStart;
    bool gateResult;
    bool showTimerFlag;

    // Gate attempt state
    GateAttemptState gateState;
    int minVel;
    int maxVel;
    unsigned long gateStart;
    unsigned long inRangeStart;
    unsigned long lastBeep;
    bool wasOutOfRange;
    bool blinkState;

    // Filter for potentiometer smoothing
    float potFilter;

    // Utility functions
    bool hasElapsed(unsigned long start, unsigned long delay) const;
    int getSmoothedPotValue(int gateLevel);
    void initPotFilter();
    void generateVelocityRange(int gateLevel, int &minVel, int &maxVel);
    void updateGateDisplays(int gateLevel, int potValue);
    bool isPotInRange(int potValue, int minVel, int maxVel);
    void setWhaddaLives(int lives);

    // Gate attempt update (non-blocking)
    bool updateGateAttempt(int gateLevel);

    // Restart effect helper
    void runRestartEffect();
};

MemoryGame2::MemoryGame2() : state(Game2State::Init),
                             currentGate(1),
                             lives(Game2Config::STARTING_LIVES),
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

bool MemoryGame2::hasElapsed(unsigned long start, unsigned long delay) const
{
    return (millis() - start) >= delay;
}

int MemoryGame2::getSmoothedPotValue(int gateLevel)
{
    int raw = analogRead(POT_PIN);
    potFilter = (1.0f - Game2Config::ALPHA) * potFilter + Game2Config::ALPHA * (float)raw;
    return (int)(potFilter * gateLevel);
}

void MemoryGame2::initPotFilter()
{
    potFilter = (float)analogRead(POT_PIN);
}

void MemoryGame2::generateVelocityRange(int gateLevel, int &minVel, int &maxVel)
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

void MemoryGame2::updateGateDisplays(int gateLevel, int potValue)
{
    lcd.setCursor(0, 0);
    lcd.print("Gate ");
    lcd.print(gateLevel);
    char buff[16];
    sprintf(buff, "Spd %4d", potValue);
    whadda.displayText(buff);
}

bool MemoryGame2::isPotInRange(int potValue, int minVel, int maxVel)
{
    int minCheck = minVel - Game2Config::TOLERANCE;
    int maxCheck = maxVel + Game2Config::TOLERANCE;
    return (potValue >= minCheck && potValue <= maxCheck);
}

void MemoryGame2::setWhaddaLives(int lives)
{
    // Clear all 8 LEDs first
    for (int i = 0; i < 8; i++)
    {
        whadda.setLED(i, false);
    }
    // Light up the first 3 LEDs based on lives
    for (int i = 0; i < lives && i < 3; i++)
    {
        whadda.setLED(i, true);
    }
}

bool MemoryGame2::updateGateAttempt(int gateLevel)
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
        // Debug info
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
        if (now - gateStart > Game2Config::GATE_TIME_MS)
        {
            // Time out – gate failed.
            gateResult = false;
            gateState = GateAttemptState::Init;
            return true;
        }
        int potValue = getSmoothedPotValue(gateLevel);
        updateGateDisplays(gateLevel, potValue);
        if (isPotInRange(potValue, minVel, maxVel))
        {
            if (now - lastBeep >= Game2Config::BEEP_INTERVAL)
            {
                blinkState = !blinkState;
                if (blinkState)
                {
                    buzzer.playTone(800, 50);
                    rgbLed.setColor(0, 255, 0); // Green when in range
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
            if (now - inRangeStart >= Game2Config::IN_RANGE_MS)
            {
                // Maintained in-range long enough.
                gateResult = true;
                gateState = GateAttemptState::Init;
                return true;
            }
        }
        else
        {
            rgbLed.setColor(255, 0, 0); // Red when out-of-range
            if (!wasOutOfRange)
            {
                wasOutOfRange = true;
                buzzer.playTone(300, 200);
            }
        }
        return false;
    }
    }
    return false; // Fallback (should not reach here)
}

void MemoryGame2::runRestartEffect()
{
    const unsigned long EFFECT_DURATION = Game2Config::RESTART_EFFECT_DURATION;
    // Disable timer display during restart effect
    showTimerFlag = false;
    // Blink Whadda's LEDs and update LCD
    whadda.blinkLEDs(0xFF, 3, Game2Config::RESTART_BLINK_INTERVAL);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Out of lives...");
    whadda.displayText("Restarting...");
}

void MemoryGame2::init()
{
    buzzer.playRoundStartMelody();
    state = Game2State::Init;
    currentGate = 1;
    lives = Game2Config::STARTING_LIVES;
    setWhaddaLives(lives);
    stateStart = millis();
    showTimerFlag = true;
    // Initialize gate attempt state
    gateState = GateAttemptState::Init;
}

bool MemoryGame2::run()
{
    unsigned long now = millis();

    switch (state)
    {
    case Game2State::Init:
        init();
        state = Game2State::Intro;
        break;

    case Game2State::Intro:
        lcd.clear();
        lcd.setCursor(0, 0);
        showTimer = false;
        lcd.print("Escape Velocity!");
        lcd.setCursor(0, 1);
        lcd.print("Good luck!");
        stateStart = now;
        state = Game2State::WaitIntro;
        break;

    case Game2State::WaitIntro:
        if (hasElapsed(stateStart, Game2Config::INTRO_DURATION))
        {
            state = Game2State::GameLoop;
            lcd.clear();
            showTimer = true;
        }
        break;

    case Game2State::GameLoop:
        if (currentGate <= Game2Config::TOTAL_GATES)
        {
            if (updateGateAttempt(currentGate))
            {
                state = Game2State::ProcessGate;
            }
        }
        else
        {
            state = Game2State::Finished;
        }
        break;

    case Game2State::ProcessGate:
        if (gateResult)
        {
            // Success: play a short beep sequence.
            buzzer.playTone(1000, 150);
            buzzer.playTone(1200, 150);
            stateStart = now;
            state = Game2State::SuccessBeep;
        }
        else
        {
            // Failure: lose a life.
            Serial.println("Gate failed. Current lives: " + String(lives));
            lives--;
            setWhaddaLives(lives);
            buzzer.playTone(200, 500);
            if (lives <= 0)
            {
                stateStart = now;
                state = Game2State::RestartEffect;
            }
            else
            {
                stateStart = now;
                state = Game2State::FailedPause;
            }
        }
        break;

    case Game2State::SuccessBeep:
        if (hasElapsed(stateStart, Game2Config::SUCCESS_BEEP_DURATION))
        {
            currentGate++;
            state = Game2State::GameLoop;
        }
        break;

    case Game2State::FailedPause:
        if (hasElapsed(stateStart, Game2Config::FAILED_PAUSE_DURATION))
        {
            state = Game2State::GameLoop;
        }
        break;

    case Game2State::RestartEffect:
        runRestartEffect();
        if (hasElapsed(stateStart, Game2Config::RESTART_EFFECT_DURATION))
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            showTimer = false;
            lcd.print("Retrying...");
            stateStart = now;
            state = Game2State::Retry;
        }
        break;

    case Game2State::Retry:
        if (hasElapsed(stateStart, Game2Config::RETRY_DURATION))
        {
            lcd.clear();
            currentGate = 1;
            lives = Game2Config::STARTING_LIVES;
            state = Game2State::GameLoop;
            Serial.println("SHOULD SET LIVES TO: " + String(lives));
            setWhaddaLives(lives);
            showTimer = true;
        }
        break;

    case Game2State::Finished:
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
MemoryGame2 game2;

bool runGame2()
{
    return game2.run();
}
