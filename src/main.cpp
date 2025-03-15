#include <Arduino.h>
#include <TM1638plus.h>
#include <LiquidCrystal_I2C.h>
#include <RGBLed.h>
#include <Buzzer.h>
#include <Whadda.h>

const int STB_PIN = 8;
const int CLK_PIN = 9;
const int DIO_PIN = 10;
const int RGB_RED = 11;
const int RGB_GREEN = 6;
const int RGB_BLUE = 5;
const int BUZZER_PIN = 2;
const int BTN_PIN = 3;
const int POT_PIN = A0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RGBLed rgbLed(RGB_RED, RGB_GREEN, RGB_BLUE);
Buzzer buzzer(BUZZER_PIN);
Whadda whadda(STB_PIN, CLK_PIN, DIO_PIN);

void setup()
{
  Serial.begin(9600);
  whadda.displayBegin();
  whadda.setLEDs(0xFF00);
  lcd.init();
  lcd.backlight();
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
}

void loop()
{
  // check if button is pressed
  if (digitalRead(BTN_PIN) == LOW)
  {
    // play a tone
    buzzer.playTone(1000, 500);
    buzzer.playImperialMarch();
    // display message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed!");
    delay(1000);
    lcd.clear();
  }

  int potValue = analogRead(POT_PIN);
  int mappedValue = map(potValue, 0, 1023, 60, 200);

  // display the value on the LCD
  lcd.setCursor(0, 1);
  lcd.print("Pot Value: ");
  lcd.print(mappedValue);
  lcd.print("   ");

  Serial.print("Pot Value: ");
  Serial.println(mappedValue);
}