#ifndef GRAVITY_LANDER_H
#define GRAVITY_LANDER_H

#include "BaseGame.h"
#include <Arduino.h>

/**
 * @brief Configuration constants for the Gravity Lander Mission game.
 *
 * This namespace contains tuning parameters for game mechanics, including
 * timing intervals, physical constants (gravity, thruster power, safe landing speed),
 * and tone frequencies/durations for events.
 */
namespace GravityGameConfig
{
    constexpr unsigned int TOTAL_LEVELS = 3;              ///< Total number of levels in this game
    constexpr unsigned long TICK_INTERVAL_MS = 100;       ///< Physics update interval in milliseconds (non-blocking tick)
    constexpr unsigned long LEVEL_INTRO_TIME = 1000;      ///< Duration to show the "Level X" intro (ms)
    constexpr unsigned long CRASH_DISPLAY_TIME = 2000;    ///< Duration to display "CRASH!" message (ms)
    constexpr unsigned long LAND_DISPLAY_TIME = 1500;     ///< Duration to display landing success message (ms)
    constexpr unsigned long COMPLETE_DISPLAY_TIME = 1500; ///< Duration to display final completion message (ms)

    // Physical parameters
    constexpr float SAFE_LANDING_SPEED = 2.0; ///< Maximum safe landing velocity (units per second)
    constexpr float THRUSTER_POWER = 3.0;     ///< Thruster acceleration power (units per second^2)
    constexpr int OVERHEAT_THRESHOLD = 100;   ///< Thruster heat level at which it overheats
    constexpr int CAUTION_THRESHOLD = 70;     ///< Heat level at which caution (yellow) is indicated
    constexpr int HEAT_PER_TICK = 10;         ///< Heat added per tick when thruster is active
    constexpr int COOL_PER_TICK = 5;          ///< Heat dissipated per tick when thruster not in use

    // Buzzer tone parameters (frequencies in Hz, durations in ms)
    constexpr int TONE_LAND_FREQ = 1000;             ///< Tone frequency for safe landing
    constexpr unsigned long TONE_LAND_DUR = 300;     ///< Tone duration for safe landing (ms)
    constexpr int TONE_CRASH_FREQ = 300;             ///< Tone frequency for crash (low buzz)
    constexpr unsigned long TONE_CRASH_DUR = 500;    ///< Tone duration for crash (ms)
    constexpr int TONE_OVERHEAT_FREQ = 800;          ///< Tone frequency for thruster overheat warning
    constexpr unsigned long TONE_OVERHEAT_DUR = 200; ///< Tone duration for overheat warning (ms)

    // LED blink interval for overheat warning
    constexpr unsigned long OVERHEAT_BLINK_INTERVAL = 250; ///< Interval for blinking fuel LEDs when overheated (ms)
    constexpr float ALTITUDE_DISPLAY_THRESHOLD = 3.0f;     ///< Altitude threshold (game units) for switching ship from top to bottom LCD row
}

/**
 * @brief Game states for the Gravity Lander Mission.
 *
 * This enum defines the primary states of the game state-machine:
 * - Idle: Inactive (not used actively in this game).
 * - Init: Initial setup state after game start.
 * - LevelIntro: Showing the "Level X" message and prepping the level.
 * - Playing: The main gameplay loop where the ship is falling and user controls it.
 * - Crash: Showing crash message and resetting current level.
 * - Landed: Showing success message for a level and preparing the next level.
 * - Completed: All levels finished, showing final message before ending the challenge.
 */
enum class LanderState
{
    Idle,
    Init,
    LevelIntro,
    Playing,
    Crash,
    Landed,
    Completed
};

/**
 * @brief GravityLander class for the Gravity Lander Mission game.
 *
 * This class encapsulates the game logic, state management, and hardware interactions
 * for the Gravity Lander Mission. It inherits from BaseGame to integrate with the
 * overall challenge framework. The class uses a non-blocking loop in `run()` to update
 * game physics and handle user input without delaying the main program.
 */
class GravityLander : public BaseGame
{
public:
    GravityLander();
    void init();
    bool run() override; // Called repeatedly; returns true when the challenge is complete

private:
    // State variables
    LanderState currentState;
    bool challengeInitialized;
    bool challengeComplete;
    int level; ///< Current level number (1-indexed)

    // Physical simulation variables
    float altitude;                ///< Current altitude of the ship (0 = ground level)
    float initialAltitude;         ///< Starting altitude at current level (for display mapping)
    float velocity;                ///< Current vertical velocity (positive downward)
    int fuel;                      ///< Remaining fuel units for thruster
    int initialFuel;               ///< Fuel at start of current level (for LED gauge scaling)
    int heat;                      ///< Current thruster heat level (0-100 scale)
    bool overheated;               ///< Whether thrusters are currently overheated/locked
    unsigned long lastTickTime;    ///< Last time the physics/logic tick was updated (ms)
    unsigned long lastPadMoveTime; ///< Last time the landing pad moved (for moving pad levels)
    bool blinkState;               ///< State for blinking fuel LEDs during overheat (true/false toggle)
    unsigned long lastBlinkTime;   ///< Last time the LED blink state toggled (ms)

    // Position variables
    uint8_t shipX;    ///< Current horizontal position of the ship (column 0-15 on LCD)
    uint8_t shipRow;  ///< Current vertical position of the ship (row 0-1 on LCD)
    uint8_t padX;     ///< Current starting column of the landing pad on LCD bottom row
    uint8_t padWidth; ///< Width of the landing pad (number of '_' characters)
    int8_t padDir;    ///< Current horizontal direction of pad movement (1 or -1, 0 if static)

    // Timing markers for state transitions
    unsigned long stateStartTime; ///< Timestamp when the current state was entered (ms)

    bool isThrusting; ///< True when thrusters are active, for visual cue

    // Private helper functions
    void setState(LanderState newState);
    void setupLevel();                                                  // Initialize variables for the current level
    void updatePhysics();                                               // Update ship physics (gravity & thruster) and check landing
    void updateDisplay();                                               // Update LCD (ship & pad) and LED displays (fuel, thruster status)
    void handleThruster();                                              // Process thruster input (button press) affecting velocity, fuel, heat
    uint8_t getShipCharacterIndex(float altitude, bool thrusterActive); ///< Determine ship character index for current altitude/thruster
};

#endif
