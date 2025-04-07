#ifndef GLOBALS_H
#define GLOBALS_H

#include <LiquidCrystal_I2C.h>
#include "Buzzer.h"
#include "RGBLed.h"
#include "Whadda.h"
#include "Button.h"

// -----------------------------------------------------------------------------
// Component Instances - External Declarations
// -----------------------------------------------------------------------------
extern LiquidCrystal_I2C lcd;
extern RGBLed rgbLed;
extern Buzzer buzzer;
extern Whadda whadda;
extern Button button;

// -----------------------------------------------------------------------------
// Global Variables
// -----------------------------------------------------------------------------
extern bool showTimer;
extern bool gameStarted;
extern bool allChallengesComplete;
extern unsigned long gameStartTime;

#endif // GLOBALS_H