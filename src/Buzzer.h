/**
 * @class Buzzer
 * @brief Encapsulates the behavior of a simple buzzer component.
 */
class Buzzer
{
public:
    /**
     * @brief Constructs a Buzzer object.
     *
     * @param pin The Arduino pin connected to the buzzer.
     */
    Buzzer(int pin);

    /**
     * @brief Initializes the buzzer by setting the pin mode.
     */
    void begin();

    /**
     * @brief Plays a tone on the buzzer.
     *
     * @param frequency The tone frequency in Hertz.
     * @param duration  Optional tone duration in milliseconds.
     *                  If 0 or not provided, tone will continue until stop() is called.
     */
    void playTone(unsigned int frequency, unsigned long duration = 0);

    /**
     * @brief Stops the buzzer tone.
     */
    void stop();

    /**
     * @brief Plays a winning melody.
     *
     * @param repeat Number of times to repeat the melody.
     */
    void playWinMelody(unsigned int repeat = 1);

    /**
     * @brief Plays a losing melody.
     *
     * @param repeat Number of times to repeat the melody.
     */
    void playLoseMelody(unsigned int repeat = 1);

    /**
     * @brief Plays the Imperial March melody.
     *
     * @param repeat Number of times to repeat the melody.
     */
    void playImperialMarch(unsigned int repeat = 1);

private:
    int _pin;
};
