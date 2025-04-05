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

// Example levels (gravity, initialAlt, padWidth, padStartX, padMoves, padMoveInterval, fuelAmount)
static const LevelConfig levelTable[GravityGameConfig::TOTAL_LEVELS] = {
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
        // Landing feedback: green blink and tone
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

void GravityLander::setupLevel()
{
    // Load level configuration
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
    shipRow = 0;         // Start ship on top LCD row at beginning of level
    isThrusting = false; // Thruster is initially off

    padX = cfg.padStartX;
    padWidth = cfg.padWidth;
    padDir = (cfg.padMoves ? 1 : 0);
    if (cfg.padMoves)
    {
        lastPadMoveTime = millis();
    }

    // Set initial LED color to blue (safe starting state)
    rgbLed.setColor(0, 0, 255);

    // Prepare LCD: clear screen and draw ground and landing pad on bottom row
    lcd.clear();
    lcd.setCursor(0, 1);
    for (uint8_t x = 0; x < 16; ++x)
    {
        if (x >= padX && x < padX + padWidth)
        {
            lcd.print('_'); // landing pad (underscores)
        }
        else
        {
            lcd.print('-'); // flat ground (dashes)
        }
    }

    // Define custom characters for the rocket ship at various vertical positions.
    // We use 4 frames for passive descent (no thruster) and 4 frames for active thruster.
    // Each frame is a 5x8 pixel bitmap (stored in a byte[8]) representing the ship in one LCD character cell.
    // The frames gradually shift the rocket downward within the cell to simulate smooth vertical movement.
    // Passive (no flame) frames:
    byte passiveHigh[8] = {
        0b00100, // Row 0: rocket tip (appears at top of cell)
        0b01110, // Row 1: rocket body (upper section)
        0b01110, // Row 2: rocket body (middle section)
        0b01110, // Row 3: rocket body (lower section)
        0b00100, // Row 4: rocket nozzle
        0b00000, // Row 5: (empty - no rocket here)
        0b00000, // Row 6: (empty)
        0b00000  // Row 7: (empty - bottom of cell empty)
    };
    byte passiveMidHigh[8] = {
        0b00000, // Row 0: (empty)
        0b00100, // Row 1: rocket tip (one row down from top)
        0b01110, // Row 2: rocket body (upper)
        0b01110, // Row 3: rocket body (middle)
        0b01110, // Row 4: rocket body (lower)
        0b00100, // Row 5: rocket nozzle
        0b00000, // Row 6: (empty)
        0b00000  // Row 7: (empty)
    };
    byte passiveMidLow[8] = {
        0b00000, // Row 0: (empty)
        0b00000, // Row 1: (empty)
        0b00100, // Row 2: rocket tip (shifted further down)
        0b01110, // Row 3: rocket body (upper)
        0b01110, // Row 4: rocket body (middle)
        0b01110, // Row 5: rocket body (lower)
        0b00100, // Row 6: rocket nozzle
        0b00000  // Row 7: (empty)
    };
    byte passiveLow[8] = {
        0b00000, // Row 0: (empty)
        0b00000, // Row 1: (empty)
        0b00000, // Row 2: (empty)
        0b00100, // Row 3: rocket tip (now at lower part of cell)
        0b01110, // Row 4: rocket body (upper)
        0b01110, // Row 5: rocket body (middle)
        0b01110, // Row 6: rocket body (lower)
        0b00100  // Row 7: rocket nozzle (bottom of cell)
    };
    // Thruster (flame) frames:
    byte thrusterHigh[8] = {
        0b00100, // Row 0: rocket tip
        0b01110, // Row 1: rocket body (upper)
        0b01110, // Row 2: rocket body (middle)
        0b01110, // Row 3: rocket body (lower)
        0b00100, // Row 4: rocket nozzle
        0b01110, // Row 5: thruster flame (appears just below nozzle)
        0b00000, // Row 6: (empty)
        0b00000  // Row 7: (empty)
    };
    byte thrusterMidHigh[8] = {
        0b00000, // Row 0: (empty)
        0b00100, // Row 1: rocket tip
        0b01110, // Row 2: rocket body (upper)
        0b01110, // Row 3: rocket body (middle)
        0b01110, // Row 4: rocket body (lower)
        0b00100, // Row 5: rocket nozzle
        0b01110, // Row 6: thruster flame (flame moves down as rocket shifts)
        0b00000  // Row 7: (empty)
    };
    byte thrusterMidLow[8] = {
        0b00000, // Row 0: (empty)
        0b00000, // Row 1: (empty)
        0b00100, // Row 2: rocket tip
        0b01110, // Row 3: rocket body (upper)
        0b01110, // Row 4: rocket body (middle)
        0b01110, // Row 5: rocket body (lower)
        0b00100, // Row 6: rocket nozzle
        0b01110  // Row 7: thruster flame (at bottom of cell)
    };
    byte thrusterLow[8] = {
        0b00000, // Row 0: (empty)
        0b00000, // Row 1: (empty)
        0b00000, // Row 2: (empty)
        0b00100, // Row 3: rocket tip
        0b01110, // Row 4: rocket body (upper)
        0b01110, // Row 5: rocket body (middle)
        0b01110, // Row 6: rocket body (lower)
        0b00100  // Row 7: rocket nozzle (no flame, at bottom of cell - flame would be off-screen)
    };

    // Load the custom characters into the LCD's CGRAM (slots 0-7).
    // Indices 0-3 correspond to passiveHigh..passiveLow, 4-7 to thrusterHigh..thrusterLow.
    lcd.createChar(0, passiveHigh);
    lcd.createChar(1, passiveMidHigh);
    lcd.createChar(2, passiveMidLow);
    lcd.createChar(3, passiveLow);
    lcd.createChar(4, thrusterHigh);
    lcd.createChar(5, thrusterMidHigh);
    lcd.createChar(6, thrusterMidLow);
    lcd.createChar(7, thrusterLow);

    // Initialize horizontal ship position from potentiometer input
    shipX = map(analogRead(POT_PIN), 0, 1023, 0, 15);

    // Draw the initial ship character on the LCD at the starting position.
    lcd.setCursor(shipX, shipRow);
    uint8_t initialCharIndex = getShipCharacterIndex(altitude, isThrusting);
    lcd.write(initialCharIndex);
    // (At start, altitude is high, so this will typically draw the top-most rocket frame on the top row)

    // Initialize fuel LED gauge to full (all 8 segments on)
    uint8_t ledCount = 8;
    uint16_t ledMask = ((1 << ledCount) - 1) << 8;
    whadda.setLEDs(ledMask);
}

void GravityLander::handleThruster()
{
    bool thrusterActive = false;

    // Check if thruster button is pressed (with debounce)
    if (button.readWithDebounce())
    {
        // If thrusters are not overheated and we have fuel, activate thrust
        if (!overheated && fuel > 0)
        {
            thrusterActive = true;
            // Apply upward acceleration (decrease velocity) due to thruster
            float dt = GravityGameConfig::TICK_INTERVAL_MS / 1000.0f;
            velocity -= GravityGameConfig::THRUSTER_POWER * dt;
            if (velocity < 0)
                velocity = 0;
            // Consume one unit of fuel and increase heat
            fuel--;
            heat += GravityGameConfig::HEAT_PER_TICK;

            // Check for overheat condition and handle LED/Buzzer feedback
            if (heat >= GravityGameConfig::OVERHEAT_THRESHOLD)
            {
                overheated = true;
                heat = GravityGameConfig::OVERHEAT_THRESHOLD;
                rgbLed.setColor(255, 0, 0);
                buzzer.playTone(GravityGameConfig::TONE_OVERHEAT_FREQ, GravityGameConfig::TONE_OVERHEAT_DUR);
            }
            else if (heat >= GravityGameConfig::CAUTION_THRESHOLD)
            {
                // Caution: thruster getting hot (set LED to yellow)
                rgbLed.setColor(255, 255, 0);
            }
        }
    }

    // Update isThrusting state for use elsewhere (e.g., display)
    isThrusting = thrusterActive;

    // Cool down thrusters when not firing
    if (!thrusterActive)
    {
        if (heat > 0)
        {
            heat -= GravityGameConfig::COOL_PER_TICK;
            if (heat < 0)
                heat = 0;
        }
    }

    // If overheated, recover once cooled below caution threshold
    if (overheated)
    {
        if (heat <= GravityGameConfig::CAUTION_THRESHOLD)
        {
            overheated = false;
            // Restore LED color: yellow if still hot, blue if cooled
            if (heat >= GravityGameConfig::CAUTION_THRESHOLD)
                rgbLed.setColor(255, 255, 0);
            else
                rgbLed.setColor(0, 0, 255);
        }
    }
}

void GravityLander::updatePhysics()
{
    // Advance physics: integrate gravity to update velocity and altitude
    float dt = GravityGameConfig::TICK_INTERVAL_MS / 1000.0f;
    velocity += levelTable[level - 1].gravity * dt;
    altitude -= velocity * dt;

    // Check for landing or crash (when ship reaches the ground)
    if (altitude <= 0.0f)
    {
        altitude = 0.0f; // clamp altitude at ground level
        bool onPad = (shipX >= padX && shipX < padX + padWidth);
        bool softSpeed = (velocity <= GravityGameConfig::SAFE_LANDING_SPEED);
        if (onPad && softSpeed)
        {
            // Successful landing
            setState(LanderState::Landed);
        }
        else
        {
            // Crash landing
            setState(LanderState::Crash);
        }
    }

    // Read horizontal control (potentiometer) for ship's X position, range 0-15
    uint8_t newShipX = map(analogRead(POT_PIN), 0, 1023, 0, 15);

    // Determine which LCD row the ship should be on based on altitude.
    // If altitude is above the threshold, ship remains on the top row (row 0).
    // Once altitude falls below the threshold, the ship is drawn on the bottom row (row 1).
    uint8_t desiredRow = (altitude >= GravityGameConfig::ALTITUDE_DISPLAY_THRESHOLD) ? 0 : 1;

    // If the ship's position changed (horizontally or vertically), erase the old ship character.
    if (newShipX != shipX || desiredRow != shipRow)
    {
        if (shipRow == 1)
        {
            // The ship was on the bottom row: we must restore whatever ground/pad was beneath it.
            char groundChar = (shipX >= padX && shipX < padX + padWidth) ? '_' : '-';
            lcd.setCursor(shipX, 1);
            lcd.print(groundChar); // restore pad/ground char now that ship moved away
        }
        else
        {
            // The ship was on the top row: just clear the character (replace with blank space, as sky).
            lcd.setCursor(shipX, shipRow);
            lcd.print(' ');
        }
    }

    // Update the ship's current position state to the new values
    shipX = newShipX;
    shipRow = desiredRow;

    // Move the landing pad if this level has a moving platform (not directly related to ship animation)
    if (padDir != 0)
    {
        unsigned long now = millis();
        const LevelConfig &cfg = levelTable[level - 1];
        if (now - lastPadMoveTime >= cfg.padMoveInterval)
        {
            // Erase old pad (draw ground over its old position)
            lcd.setCursor(padX, 1);
            for (uint8_t i = 0; i < padWidth; ++i)
                lcd.print('-');
            // Update pad position and bounce at edges
            padX += padDir;
            if ((padDir == 1 && padX + padWidth > 15) || (padDir == -1 && padX == 0))
            {
                padDir = -padDir;
                padX += padDir;
            }
            // Draw new pad at the new position
            lcd.setCursor(padX, 1);
            for (uint8_t i = 0; i < padWidth; ++i)
                lcd.print('_');
            lastPadMoveTime = now;
        }
    }
}

void GravityLander::updateDisplay()
{
    // Only update the ship's LCD character if the ship is airborne (altitude > 0).
    // If altitude == 0, the ship has landed or crashed and we will display the respective message instead.
    if (altitude > 0)
    {
        lcd.setCursor(shipX, shipRow);
        // Determine which custom character to display for the ship based on current altitude and thruster state.
        uint8_t charIndex = getShipCharacterIndex(altitude, isThrusting);
        lcd.write(charIndex); // Draw the ship (rocket) at the new position/frame
    }

    // Update the fuel LED gauge (8 LEDs) and handle blinking when overheated
    if (overheated)
    {
        unsigned long now = millis();
        if (now - lastBlinkTime >= GravityGameConfig::OVERHEAT_BLINK_INTERVAL)
        {
            lastBlinkTime = now;
            blinkState = !blinkState;
            if (blinkState)
            {
                // Blink off: temporarily turn off all fuel LEDs to indicate overheat
                whadda.clearLEDs();
            }
            else
            {
                // Blink on: restore LED indication of remaining fuel
                uint8_t fuelLeds = (initialFuel > 0) ? (fuel * 8 + initialFuel - 1) / initialFuel : 0;
                uint16_t mask = ((fuelLeds > 0 ? (1 << fuelLeds) - 1 : 0) << 8);
                whadda.setLEDs(mask);
            }
        }
    }
    else
    {
        // If not overheated, simply show proportional fuel level on LEDs
        uint8_t fuelLeds = (initialFuel > 0) ? (fuel * 8 + initialFuel - 1) / initialFuel : 0;
        uint16_t mask = ((fuelLeds > 0 ? (1 << fuelLeds) - 1 : 0) << 8);
        whadda.setLEDs(mask);
    }
}

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
        // Transition immediately to the level intro state
        setState(LanderState::LevelIntro);
        break;

    case LanderState::LevelIntro:
        // Briefly display "Level X" intro, then set up the level and start playing
        if (now - stateStartTime >= GravityGameConfig::LEVEL_INTRO_TIME)
        {
            setupLevel();
            setState(LanderState::Playing);
        }
        break;

    case LanderState::Playing:
        // Main gameplay loop (runs in a non-blocking manner at fixed time intervals)
        if (now - lastTickTime >= GravityGameConfig::TICK_INTERVAL_MS)
        {
            lastTickTime = now;
            handleThruster(); // process input and update velocity/fuel/heat
            updatePhysics();  // update altitude, position, and handle land/crash
            updateDisplay();  // refresh LCD display (ship icon and LEDs)
        }
        break;

    case LanderState::Crash:
        // After crash, wait for a moment then restart the level
        if (now - stateStartTime >= GravityGameConfig::CRASH_DISPLAY_TIME)
        {
            setState(LanderState::LevelIntro);
        }
        break;

    case LanderState::Landed:
        // After safe landing, advance to next level or finish if last level
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
        // After completing all levels, end the challenge after a final message delay
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
    // Determine which custom character index to use for the ship based on its altitude and thruster state.
    // We have 4 vertical animation frames (indices 0-3 for no thruster, 4-7 for thruster).
    const uint8_t FRAMES = 4;
    float threshold = GravityGameConfig::ALTITUDE_DISPLAY_THRESHOLD;
    uint8_t frameIndex = 0;

    if (altitude >= threshold)
    {
        // Ship is in the upper region (closer to starting altitude, on the top LCD row).
        if (initialAltitude > threshold)
        {
            // Linearly map the altitude in the range [threshold, initialAltitude] to frames [3..0].
            // initialAltitude corresponds to frame 0 (fully top position), threshold corresponds to frame 3 (lower in the top cell).
            float interval = (initialAltitude - threshold) / (FRAMES - 1);
            for (uint8_t i = 1; i < FRAMES; ++i)
            {
                if (altitude <= initialAltitude - i * interval)
                {
                    frameIndex = i;
                }
            }
            // frameIndex will be 0 when altitude is close to initialAltitude, and approach 3 as altitude approaches threshold.
        }
        else
        {
            // If the initial altitude is very low (near threshold), we cannot map many frames; just use the lowest top-row frame.
            frameIndex = FRAMES - 1; // 3
        }
    }
    else
    {
        // Ship is in the lower region (close to ground, drawn on bottom LCD row).
        // Map altitude in [0, threshold] to frames [3..0] (0 = highest position in bottom cell, 3 = lowest).
        float interval = threshold / (FRAMES - 1);
        for (uint8_t i = 1; i < FRAMES; ++i)
        {
            if (altitude <= threshold - i * interval)
            {
                frameIndex = i;
            }
        }
        // frameIndex will be 0 when altitude is just below threshold, increasing to 3 as altitude approaches 0.
    }

    // If thruster is active, use the set of characters with the thruster flame.
    // These are stored in indices 4-7, so we offset the frame index by 4 in that case.
    if (thrusterActive)
    {
        frameIndex += FRAMES; // 0->4, 1->5, 2->6, 3->7 for thruster frames
    }
    return frameIndex;
}