#ifndef BASE_GAME_H
#define BASE_GAME_H

#include <Arduino.h>

extern bool showTimer;

/**
 * @brief An abstract base class for games.
 *
 * This class acts as a blueprint for creating different game implementations.
 * It defines the main game loop and provides utility functions for managing timers.
 */
class BaseGame
{
public:
    virtual ~BaseGame() {}

    /**
     * @brief Start the game loop.
     *
     * This function initializes the game and enters the main loop.
     * @return true if the game is completed.
     */
    virtual bool run() = 0;

protected:
    /**
     * @brief Check if a specified time interval has elapsed since a given start time.
     *
     * This function compares the current time (in milliseconds) with the start time
     * and the specified delay time. If the elapsed time is greater than or equal to
     */
    bool hasElapsed(unsigned long start, unsigned long delayTime) const
    {
        return (millis() - start) >= delayTime;
    }

    /**
     * @brief Reset a timer reference to the current time.
     */
    void resetTimer(unsigned long &timerRef)
    {
        timerRef = millis();
    }
};

#endif
