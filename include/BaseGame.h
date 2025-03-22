#ifndef BASE_GAME_H
#define BASE_GAME_H

#include <Arduino.h>

// BaseGame acts as an abstract class with common helpers.
class BaseGame
{
public:
    virtual ~BaseGame() {}
    // The main game loop. Returns true when the game is complete.
    virtual bool run() = 0;

protected:
    // Helper: Check if a specified delay has elapsed since a start time.
    bool hasElapsed(unsigned long start, unsigned long delayTime) const
    {
        return (millis() - start) >= delayTime;
    }

    // Helper: Reset a timer variable.
    void resetTimer(unsigned long &timerRef)
    {
        timerRef = millis();
    }
};

#endif
