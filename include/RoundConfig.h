#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

// TODO identify the common parts and move here;
namespace RoundConfig
{
    // Shared Timing Constants
    constexpr unsigned long INTRO_DURATION = 1500;
    constexpr unsigned long SUCCESS_BEEP_DURATION = 300;
    constexpr unsigned long ONE_MS_DURATION = 1000;
    constexpr unsigned long RETRY_DURATION = 1000;
    constexpr unsigned long RESTART_EFFECT_DURATION = 1500;
    constexpr unsigned long RESTART_BLINK_INTERVAL = 200;

    // Other shared constants can go here, e.g. LED timings, debounce delay, etc.
    constexpr unsigned long LED_ON_TIME = 300;
    constexpr unsigned long LED_OFF_TIME = 200;
    constexpr unsigned long BUTTON_DEBOUNCE_DELAY = 200;
}

#endif
