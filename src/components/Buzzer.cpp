#include "Buzzer.h"
#include <Arduino.h>

/**
 * @brief Constructs a Buzzer object.
 *
 * @param pin The Arduino pin connected to the buzzer.
 */
Buzzer::Buzzer(int pin) : _pin(pin) {}

/**
 * @brief Initializes the buzzer by setting the pin mode.
 */
void Buzzer::begin()
{
    pinMode(_pin, OUTPUT);
}

/**
 * @brief Plays a tone on the buzzer.
 *
 * @param frequency The tone frequency in Hertz.
 * @param duration  Optional tone duration in milliseconds.
 *                  If 0 or not provided, tone will continue until stop() is called.
 */
void Buzzer::playTone(unsigned int frequency, unsigned long duration)
{
    if (duration == 0)
    {
        tone(_pin, frequency);
    }
    else
    {
        tone(_pin, frequency, duration);
    }
}

/**
 * @brief Stops the buzzer tone.
 */
void Buzzer::stop()
{
    noTone(_pin);
}

/**
 * @brief Plays a winning melody.
 *
 * @param repeat Number of times to repeat the melody.
 */
void Buzzer::playWinMelody(unsigned int repeat)
{
    for (unsigned int i = 0; i < repeat; i++)
    {
        playTone(523, 150); // C5
        delay(160);

        playTone(659, 150); // E5
        delay(160);

        playTone(783, 200); // G5
        delay(210);

        playTone(1046, 300); // C6 (High)
        delay(310);

        playTone(880, 250); // A5
        delay(260);

        playTone(987, 400); // B5
        delay(450);

        stop();
        delay(200); // short pause between repetitions
    }
}

/**
 * @brief Plays a losing melody.
 *
 * @param repeat Number of times to repeat the melody.
 */
void Buzzer::playLoseMelody(unsigned int repeat)
{
    for (unsigned int i = 0; i < repeat; i++)
    {
        playTone(440, 250); // A4
        delay(260);

        playTone(415, 200); // G#4
        delay(210);

        playTone(392, 250); // G4
        delay(260);

        playTone(349, 300); // F4
        delay(310);

        playTone(261, 450); // C4 (Low)
        delay(460);

        stop();
        delay(200); // short pause between repetitions
    }
}

void Buzzer::playRoundStartMelody(unsigned int repeat)
{
    for (unsigned int i = 0; i < repeat; i++)
    {
        playTone(440, 200); // A4
        delay(210);

        playTone(523, 200); // C5
        delay(210);

        playTone(659, 200); // E5
        delay(210);

        playTone(784, 200); // G5
        delay(210);

        playTone(880, 200); // A5
        delay(210);

        stop();
        delay(200); // short pause between repetitions
    }
}

// *** CREDITS TO CHATGPT FOR THIS MELODY
// *** IMPERIAL MARCH MELODY
/**
 * @brief Plays the Imperial March melody.
 *
 * @param repeat Number of times to repeat the melody.
 */
void Buzzer::playImperialMarch(unsigned int repeat)
{
    for (unsigned int i = 0; i < repeat; i++)
    {
        playTone(440, 400); // A4
        delay(450);
        playTone(440, 400); // A4
        delay(450);
        playTone(440, 400); // A4
        delay(450);
        playTone(349, 300); // F4
        delay(350);
        playTone(523, 150); // C5
        delay(200);
        playTone(440, 400); // A4
        delay(450);
        playTone(349, 300); // F4
        delay(350);
        playTone(523, 150); // C5
        delay(200);
        playTone(440, 800); // A4
        delay(850);

        playTone(659, 400); // E5
        delay(450);
        playTone(659, 400); // E5
        delay(450);
        playTone(659, 400); // E5
        delay(450);
        playTone(698, 300); // F5
        delay(350);
        playTone(523, 150); // C5
        delay(200);
        playTone(415, 400); // G#4
        delay(450);
        playTone(349, 300); // F4
        delay(350);
        playTone(523, 150); // C5
        delay(200);
        playTone(440, 800); // A4
        delay(850);

        stop();
        delay(400); // pause between repetitions
    }
}