#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Button.h"
#include "Pins.h"
#include "Game4.h"

// External hardware instances (defined in main.cpp)
extern LiquidCrystal_I2C lcd;
extern RGBLed rgbLed;
extern Buzzer buzzer;
extern Whadda whadda;
extern Button button;

// Level configuration table for Gravity Lander Mission
struct LevelConfig
{
    float gravity;
    int initialAlt;
    uint8_t padWidth;
    uint8_t padStartX;
    bool padMoves;
    unsigned long padMoveInterval;
    int fuelAmount;
};

// Example levels (you may have more or different)
static const LevelConfig levelTable[GravityGameConfig::TOTAL_LEVELS] = {
    // gravity, initialAlt, padWidth, padStartX, padMoves, padMoveInterval, fuelAmount
    {1.0f, 10, 5, 5, false, 0, 20},
    {1.5f, 15, 3, 6, false, 0, 15},
    {2.0f, 20, 2, 0, true, 500, 10}};

GravityLander::GravityLander()
    : currentState(LanderState::Idle),
      challengeInitialized(false),
      challengeComplete(false),
      level(0),
      altitude(0.0f),
      initialAltitude(0.0f),
      velocity(0.0f),
      fuel(0),
      initialFuel(0),
      heat(0),
      overheated(false),
      lastTickTime(0),
      lastPadMoveTime(0),
      blinkState(false),
      lastBlinkTime(0),
      shipX(0),
      shipRow(0), // Start at top row by default
      padX(0),
      padWidth(0),
      padDir(0),
      stateStartTime(0),
      isThrusting(false)
{
}

// ---------------------------------------------------
//  Initialization and State Transitions
// ---------------------------------------------------

void GravityLander::init()
{
    if (!challengeInitialized)
    {
        buzzer.playRoundStartMelody();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Gravity Lander!");
        lcd.setCursor(0, 1);
        lcd.print("Good luck");

        whadda.clearDisplay();
        whadda.clearLEDs();

        challengeInitialized = true;
        challengeComplete = false;

        // Start on level 1
        level = 1;
        setState(LanderState::Init);
    }
}

void GravityLander::setState(LanderState newState)
{
    currentState = newState;
    stateStartTime = millis();

    switch (newState)
    {
    case LanderState::LevelIntro:
        lcd.clear();
        lcd.setCursor(0, 0);
        {
            char msg[16];
            snprintf(msg, sizeof(msg), "Level %d", level);
            lcd.print(msg);
        }
        lcd.setCursor(0, 1);
        lcd.print("Get ready!");
        // (Optional: a timer or countdown could be shown here)
        break;

    case LanderState::Crash:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CRASH!");
        lcd.setCursor(0, 1);
        lcd.print("Retrying...");
        buzzer.playTone(GravityGameConfig::TONE_CRASH_FREQ, GravityGameConfig::TONE_CRASH_DUR);
        break;

    case LanderState::Landed:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Landed Safely!");
        lcd.setCursor(0, 1);
        lcd.print("Good job");
        // Landing feedback
        buzzer.playTone(GravityGameConfig::TONE_LAND_FREQ, GravityGameConfig::TONE_LAND_DUR);
        rgbLed.blinkColor(0, 255, 0, 2);
        whadda.clearLEDs();
        break;

    case LanderState::Completed:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("All levels done!");
        lcd.setCursor(0, 1);
        lcd.print("Mission Success");
        break;

    default:
        break;
    }
}

// ---------------------------------------------------
//  Level Setup and Gameplay
// ---------------------------------------------------

void GravityLander::setupLevel()
{
    const LevelConfig &cfg = levelTable[level - 1];
    altitude = cfg.initialAlt;
    initialAltitude = altitude;
    velocity = 0.0f;
    fuel = cfg.fuelAmount;
    initialFuel = cfg.fuelAmount;
    heat = 0;
    overheated = false;
    blinkState = false;
    lastBlinkTime = 0;
    shipRow = 0;         // Start on top row
    isThrusting = false; // No thrust at start

    padX = cfg.padStartX;
    padWidth = cfg.padWidth;
    padDir = (cfg.padMoves ? 1 : 0);
    if (cfg.padMoves)
    {
        lastPadMoveTime = millis();
    }

    // Set LED color to blue (safe starting state)
    rgbLed.setColor(0, 0, 255);

    // Clear LCD and draw the ground and landing pad on the bottom row
    lcd.clear();
    lcd.setCursor(0, 1);
    for (uint8_t x = 0; x < 16; ++x)
    {
        if (x >= padX && x < padX + padWidth)
            lcd.print('_'); // landing pad
        else
            lcd.print('-'); // flat ground
    }

    // Define custom characters for rocket animation frames (passive descent and with thruster)
    byte passiveHigh[8] = {
        0b00100, // Row0: rocket tip (top of cell)
        0b01110, // Row1: rocket body (upper section)
        0b01110, // Row2: rocket body (middle section)
        0b01110, // Row3: rocket body (lower section)
        0b00100, // Row4: rocket nozzle
        0b00000, // Row5: (empty)
        0b00000, // Row6: (empty)
        0b00000  // Row7: (empty)
    };
    byte passiveMidHigh[8] = {
        0b00000, // Row0: (empty)
        0b00100, // Row1: rocket tip
        0b01110, // Row2: rocket body (upper)
        0b01110, // Row3: rocket body (middle)
        0b01110, // Row4: rocket body (lower)
        0b00100, // Row5: rocket nozzle
        0b00000, // Row6: (empty)
        0b00000  // Row7: (empty)
    };
    byte passiveMidLow[8] = {
        0b00000, // Row0: (empty)
        0b00000, // Row1: (empty)
        0b00100, // Row2: rocket tip
        0b01110, // Row3: rocket body (upper)
        0b01110, // Row4: rocket body (middle)
        0b01110, // Row5: rocket body (lower)
        0b00100, // Row6: rocket nozzle
        0b00000  // Row7: (empty)
    };
    byte passiveLow[8] = {
        0b00000, // Row0: (empty)
        0b00000, // Row1: (empty)
        0b00000, // Row2: (empty)
        0b00100, // Row3: rocket tip
        0b01110, // Row4: rocket body (upper)
        0b01110, // Row5: rocket body (middle)
        0b01110, // Row6: rocket body (lower)
        0b00100  // Row7: rocket nozzle (bottom of cell)
    };
    byte thrusterHigh[8] = {
        0b00100, // Row0: rocket tip
        0b01110, // Row1: rocket body (upper)
        0b01110, // Row2: rocket body (middle)
        0b01110, // Row3: rocket body (lower)
        0b00100, // Row4: rocket nozzle
        0b01110, // Row5: flame (thruster firing)
        0b00000, // Row6: (empty)
        0b00000  // Row7: (empty)
    };
    byte thrusterMidHigh[8] = {
        0b00000, // Row0: (empty)
        0b00100, // Row1: rocket tip
        0b01110, // Row2: rocket body (upper)
        0b01110, // Row3: rocket body (middle)
        0b01110, // Row4: rocket body (lower)
        0b00100, // Row5: rocket nozzle
        0b01110, // Row6: flame
        0b00000  // Row7: (empty)
    };
    byte thrusterMidLow[8] = {
        0b00000, // Row0: (empty)
        0b00000, // Row1: (empty)
        0b00100, // Row2: rocket tip
        0b01110, // Row3: rocket body (upper)
        0b01110, // Row4: rocket body (middle)
        0b01110, // Row5: rocket body (lower)
        0b00100, // Row6: rocket nozzle
        0b01110  // Row7: flame
    };
    byte thrusterLow[8] = {
        0b00000, // Row0: (empty)
        0b00000, // Row1: (empty)
        0b00000, // Row2: (empty)
        0b00100, // Row3: rocket tip
        0b01110, // Row4: rocket body (upper)
        0b01110, // Row5: rocket body (middle)
        0b01110, // Row6: rocket body (lower)
        0b00100  // Row7: rocket nozzle (no room for flame at bottom)
    };

    // Load the custom characters into the LCD (CGRAM slots 0-7)
    lcd.createChar(0, passiveHigh);
    lcd.createChar(1, passiveMidHigh);
    lcd.createChar(2, passiveMidLow);
    lcd.createChar(3, passiveLow);
    lcd.createChar(4, thrusterHigh);
    lcd.createChar(5, thrusterMidHigh);
    lcd.createChar(6, thrusterMidLow);
    lcd.createChar(7, thrusterLow);

    // Initialize horizontal ship position from potentiometer
    shipX = map(analogRead(POT_PIN), 0, 1023, 0, 15);

    // Draw the initial ship character at the starting position
    lcd.setCursor(shipX, shipRow);
    uint8_t initialCharIndex = getShipCharacterIndex(altitude, isThrusting);
    lcd.write(initialCharIndex);

    // Set all Whadda LED segments on to represent full fuel at start
    uint8_t ledCount = 8;
    uint16_t ledMask = ((1 << ledCount) - 1) << 8; // turn on all 8 fuel LEDs
    whadda.setLEDs(ledMask);
}

// ---------------------------------------------------
//  Physics & Thruster Handling
// ---------------------------------------------------

void GravityLander::handleThruster()
{
    bool thrusterActive = false;

    // Check thrust button with debounce
    if (button.readWithDebounce())
    {
        // If not overheated and we have fuel, activate thrust
        if (!overheated && fuel > 0)
        {
            thrusterActive = true;

            float dt = GravityGameConfig::TICK_INTERVAL_MS / 1000.0f;
            velocity -= GravityGameConfig::THRUSTER_POWER * dt;
            if (velocity < 0)
                velocity = 0;

            fuel--;
            heat += GravityGameConfig::HEAT_PER_TICK;

            // Check for overheat condition
            if (heat >= GravityGameConfig::OVERHEAT_THRESHOLD)
            {
                overheated = true;
                heat = GravityGameConfig::OVERHEAT_THRESHOLD;
                rgbLed.setColor(255, 0, 0);
                buzzer.playTone(GravityGameConfig::TONE_OVERHEAT_FREQ, GravityGameConfig::TONE_OVERHEAT_DUR);
            }
            else if (heat >= GravityGameConfig::CAUTION_THRESHOLD)
            {
                // Change LED to yellow caution if approaching overheat
                rgbLed.setColor(255, 255, 0);
            }
        }
    }

    isThrusting = thrusterActive;

    // If not thrusting, cool down the thruster
    if (!thrusterActive)
    {
        if (heat > 0)
        {
            heat -= GravityGameConfig::COOL_PER_TICK;
            if (heat < 0)
                heat = 0;
        }
    }

    // Recover from overheat once cooled below caution threshold
    if (overheated)
    {
        if (heat <= GravityGameConfig::CAUTION_THRESHOLD)
        {
            overheated = false;
            // If still above caution threshold use yellow, otherwise back to blue
            if (heat >= GravityGameConfig::CAUTION_THRESHOLD)
                rgbLed.setColor(255, 255, 0);
            else
                rgbLed.setColor(0, 0, 255);
        }
    }
}

void GravityLander::updatePhysics()
{
    // Gravity integration
    float dt = GravityGameConfig::TICK_INTERVAL_MS / 1000.0f;
    velocity += levelTable[level - 1].gravity * dt;
    altitude -= velocity * dt;
    if (altitude < 0.0f)
        altitude = 0.0f;

    // Check landing/crash
    if (altitude <= 0.0f)
    {
        bool onPad = (shipX >= padX && shipX < padX + padWidth);
        bool softSpeed = (velocity <= GravityGameConfig::SAFE_LANDING_SPEED);
        if (onPad && softSpeed)
            setState(LanderState::Landed);
        else
            setState(LanderState::Crash);
    }

    // Horizontal position from potentiometer
    uint8_t newShipX = map(analogRead(POT_PIN), 0, 1023, 0, 15);

    // *** Keep the rocket in row 0 during descent: ***
    // Only move to row 1 if altitude is 0 and we want to show it on the ground
    uint8_t desiredRow = (altitude > 0.0f) ? 0 : 1;

    // If position or row changed, erase old
    if (newShipX != shipX || desiredRow != shipRow)
    {
        // Clear old position. If row was 1, put back ground/pad, else put blank sky
        if (shipRow == 1)
        {
            // Bottom row -> restore ground/pad
            char groundChar = (shipX >= padX && shipX < padX + padWidth) ? '_' : '-';
            lcd.setCursor(shipX, 1);
            lcd.print(groundChar);
        }
        else
        {
            // Top row -> just overwrite with space
            lcd.setCursor(shipX, shipRow);
            lcd.print(' ');
        }
    }

    // Update ship state
    shipX = newShipX;
    shipRow = desiredRow;

    // Move landing pad if applicable (unchanged)
    if (padDir != 0)
    {
        unsigned long now = millis();
        const LevelConfig &cfg = levelTable[level - 1];
        if (now - lastPadMoveTime >= cfg.padMoveInterval)
        {
            // Erase old pad
            lcd.setCursor(padX, 1);
            for (uint8_t i = 0; i < padWidth; ++i)
                lcd.print('-');

            // Update padX, bounce at edges
            padX += padDir;
            if ((padDir == 1 && (padX + padWidth) > 15) ||
                (padDir == -1 && padX == 0))
            {
                padDir = -padDir;
                padX += padDir;
            }

            // Draw new pad
            lcd.setCursor(padX, 1);
            for (uint8_t i = 0; i < padWidth; ++i)
                lcd.print('_');

            lastPadMoveTime = now;
        }
    }
}

// ---------------------------------------------------
//  Display Updates
// ---------------------------------------------------

void GravityLander::updateDisplay()
{
    // Only draw the ship if it is airborne (not landed or crashed)
    if (altitude > 0.0f)
    {
        lcd.setCursor(shipX, 0);
        uint8_t charIndex = getShipCharacterIndex(altitude, isThrusting);
        lcd.write(charIndex);
    }
    else if (altitude == 0.0f && currentState == LanderState::Playing)
    {
        // If we're still "in the playing state" but altitude is 0,
        // we can optionally draw the rocket in row 1 to show it on the ground
        lcd.setCursor(shipX, 1);
        // If you want the "lowest" frame:
        uint8_t charIndex = (isThrusting ? 7 : 3);
        // 3 = passiveLow, 7 = thrusterLow
        lcd.write(charIndex);
    }

    // Update fuel LED gauge and handle blinking if overheated
    if (overheated)
    {
        unsigned long now = millis();
        if (now - lastBlinkTime >= GravityGameConfig::OVERHEAT_BLINK_INTERVAL)
        {
            lastBlinkTime = now;
            blinkState = !blinkState;
            if (blinkState)
            {
                // Temporarily turn off all fuel LEDs (overheat blink)
                whadda.clearLEDs();
            }
            else
            {
                // Restore fuel level indication
                uint8_t fuelLeds = (initialFuel > 0)
                                       ? (fuel * 8 + initialFuel - 1) / initialFuel
                                       : 0;
                uint16_t mask = ((fuelLeds > 0 ? (1 << fuelLeds) - 1 : 0) << 8);
                whadda.setLEDs(mask);
            }
        }
    }
    else
    {
        // Normal fuel gauge (proportional to remaining fuel)
        uint8_t fuelLeds = (initialFuel > 0)
                               ? (fuel * 8 + initialFuel - 1) / initialFuel
                               : 0;
        uint16_t mask = ((fuelLeds > 0 ? (1 << fuelLeds) - 1 : 0) << 8);
        whadda.setLEDs(mask);
    }
}

// ---------------------------------------------------
//  Main Run Loop (Non-Blocking)
// ---------------------------------------------------

bool GravityLander::run()
{
    if (!challengeInitialized)
    {
        init();
    }
    if (challengeComplete)
    {
        return true;
    }

    unsigned long now = millis();

    switch (currentState)
    {
    case LanderState::Idle:
        // Idle state is not used in this game
        break;

    case LanderState::Init:
        // Immediately transition to LevelIntro
        setState(LanderState::LevelIntro);
        break;

    case LanderState::LevelIntro:
        // Wait for a moment, then set up level and start playing
        if (now - stateStartTime >= GravityGameConfig::LEVEL_INTRO_TIME)
        {
            setupLevel();
            setState(LanderState::Playing);
        }
        break;

    case LanderState::Playing:
        // Main game loop tick (non-blocking)
        if (now - lastTickTime >= GravityGameConfig::TICK_INTERVAL_MS)
        {
            lastTickTime = now;
            handleThruster();
            updatePhysics();
            updateDisplay();
        }
        break;

    case LanderState::Crash:
        // After a crash, display message briefly then restart level
        if (now - stateStartTime >= GravityGameConfig::CRASH_DISPLAY_TIME)
        {
            setState(LanderState::LevelIntro);
        }
        break;

    case LanderState::Landed:
        // After a safe landing, proceed to next level or finish game
        if (now - stateStartTime >= GravityGameConfig::LAND_DISPLAY_TIME)
        {
            if (level < GravityGameConfig::TOTAL_LEVELS)
            {
                level++;
                setState(LanderState::LevelIntro);
            }
            else
            {
                setState(LanderState::Completed);
            }
        }
        break;

    case LanderState::Completed:
        // All levels completed, end challenge after a final message delay
        if (now - stateStartTime >= GravityGameConfig::COMPLETE_DISPLAY_TIME)
        {
            challengeComplete = true;
        }
        break;
    }

    return challengeComplete;
}

uint8_t GravityLander::getShipCharacterIndex(float altitude, bool thrusterActive)
{
    // Make sure altitude stays in [0, initialAltitude]
    if (altitude < 0.0f)
        altitude = 0.0f;
    if (altitude > initialAltitude)
        altitude = initialAltitude;

    // Compute "descent fraction" = how far we are from top (0) to ground (1)
    float descentFrac = 0.0f;
    if (initialAltitude > 0.0f)
    {
        descentFrac = (initialAltitude - altitude) / initialAltitude;
        // 0.0 => top, 1.0 => ground
    }

    // Map that fraction onto 4 frames: 0..3
    // Multiply by 3.0f to get up to ~3, round to nearest int
    float scaled = descentFrac * 3.0f;
    uint8_t frame = (uint8_t)(scaled + 0.5f);
    if (frame > 3)
        frame = 3; // clamp at 3

    // If thruster is active, offset by +4 to use thruster frames [4..7]
    if (thrusterActive)
        frame += 4;

    return frame; // 0..3 for passive, 4..7 for thruster
}
