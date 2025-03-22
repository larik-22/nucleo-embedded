#ifndef MEMORY_GAME2_H
#define MEMORY_GAME2_H

#include "BaseGame.h"
#include "Pins.h"

namespace EscVelocityConfig
{
    constexpr int TOTAL_GATES = 1;                // Number of gates to pass
    constexpr int STARTING_LIVES = 3;             // Initial lives
    constexpr unsigned long GATE_TIME_MS = 10000; // Allowed time per gate
    constexpr unsigned long IN_RANGE_MS = 2500;   // Must stay in range for this duration
    constexpr unsigned long BEEP_INTERVAL = 250;  // Gap between short beeps (ms)
    constexpr float ALPHA = 0.1f;                 // Exponential smoothing factor
    constexpr int TOLERANCE = 5;                  // Threshold to reduce flicker issues

    // State durations
    constexpr unsigned long INTRO_DURATION = 1500;
    constexpr unsigned long SUCCESS_BEEP_DURATION = 300;
    constexpr unsigned long FAILED_PAUSE_DURATION = 1000;
    constexpr unsigned long RETRY_DURATION = 1000;
    constexpr unsigned long RESTART_EFFECT_DURATION = 1500;
    constexpr unsigned long RESTART_BLINK_INTERVAL = 200;
}

enum class GateAttemptState
{
    Init,
    Loop
};

enum class EscVelocityState
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

class EscapeVelocity : public BaseGame
{
public:
    EscapeVelocity();
    void init();
    bool run() override;

private:
    // Overall game state
    EscVelocityState state;
    int currentGate;
    int lives;
    unsigned long stateStart;
    bool gateResult;
    bool showTimerFlag;

    // Gate attempt state variables
    GateAttemptState gateState;
    int minVel;
    int maxVel;
    unsigned long gateStart;
    unsigned long inRangeStart;
    unsigned long lastBeep;
    bool wasOutOfRange;
    bool blinkState;

    // Potentiometer smoothing filter
    float potFilter;

    int getSmoothedPotValue(int gateLevel);
    void initPotFilter();
    void generateVelocityRange(int gateLevel, int &minVel, int &maxVel);
    void updateGateDisplays(int gateLevel, int potValue);
    bool isPotInRange(int potValue, int minVel, int maxVel);
    void setWhaddaLives(int lives);

    // Gate attempt update
    bool updateGateAttempt(int gateLevel);

    // Restart effect helper
    void runRestartEffect();
};

#endif
