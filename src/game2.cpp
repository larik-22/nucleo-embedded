#include <Arduino.h>
#include "game2.h"
#include "Buzzer.h"

extern Buzzer buzzer;

bool runGame2()
{
    static bool challengeComplete = false;
    if (challengeComplete)
        return true;

    // Simple stub logic: wait 2 seconds, then done
    static unsigned long startTime = 0;
    static bool started = false;

    if (!started)
    {
        buzzer.playRoundStartMelody();
        startTime = millis();
        started = true;
        // Possibly show a short message or read some sensors
    }

    if (millis() - startTime >= 4000)
    {
        challengeComplete = true;
    }

    return challengeComplete;
}
