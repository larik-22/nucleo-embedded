// #include <LiquidCrystal_I2C.h>
// #include "pins.h" // Include your custom pin definitions
// #include <Arduino.h>

// // Initialize the I2C LCD (address 0x27, 16 columns, 2 rows)
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// // Custom characters for the Dino and obstacles
// byte DINO_STANDING_PART_1[8] = {B00000, B00000, B00010, B00010, B00011, B00011, B00001, B00001};
// byte DINO_STANDING_PART_2[8] = {B00111, B00111, B00111, B00100, B11100, B11100, B11000, B01000};
// byte DINO_RIGHT_FOOT_PART_1[8] = {B00000, B00000, B00010, B00010, B00011, B00011, B00001, B00001};
// byte DINO_RIGHT_FOOT_PART_2[8] = {B00111, B00111, B00111, B00100, B11100, B11100, B11000, B00000};
// byte DINO_LEFT_FOOT_PART_1[8] = {B00000, B00000, B00010, B00010, B00011, B00011, B00001, B00000};
// byte DINO_LEFT_FOOT_PART_2[8] = {B00111, B00111, B00111, B00100, B11100, B11100, B11000, B01000};
// byte TWO_BRANCHES_PART_1[8] = {B00000, B00100, B00100, B10100, B10100, B11100, B00100, B00100};
// byte TWO_BRANCHES_PART_2[8] = {B00100, B00101, B00101, B10101, B11111, B00100, B00100, B00100};
// byte BIRD_WINGS_PART1[8] = {B00001, B00001, B00001, B00001, B01001, B11111, B00000, B00000};
// byte BIRD_WINGS_PART2[8] = {B00000, B10000, B11000, B11100, B11110, B11111, B00000, B00000};

// // Forward declaration for the gameOver function.
// void gameOver();

// // Game variables
// int dinoCol1 = 1;
// int dinoCol2 = 2;
// int dinoRow = 1;
// unsigned long timer1 = 0; // For dino foot animation timing using millis()
// int period1 = 100;        // Animation period in ms
// int dinoFlag = 1;         // Toggle flag for dino foot movement

// int obstacleRow = 0;  // Row where the obstacle is drawn (0: upper, 1: lower)
// int obstacleCol = 13; // Column position of the obstacle
// int period2 = 100;    // Obstacle movement period (will accelerate)
// unsigned long timer2 = 0;
// int flagDrawObstacle = 0; // Flag to indicate the obstacle should be drawn
// int varB = 1;             // Used in collision detection (branch)
// int varC = 2;             // Used in collision detection (branch)
// int jumpState = 0;        // 0 = on ground, 1 = jumping
// unsigned long timer3 = 0; // For score update timing
// int period3 = 100;        // Score update period in ms
// int score = 0;            // Main score counter
// int score2 = 0;           // Secondary score counter

// int randomObstacle = 0;     // Random number to decide which obstacle (bird or branch) to draw
// int birdCol = 13;           // Column for the bird obstacle
// int currentButtonState = 0; // Current reading from the jump button
// int lastButtonState = 0;    // Previous reading from the jump button
// int tempCol = 13;           // Temporary column for clearing old obstacle positions

// int acceleration = 1;     // How fast obstacles accelerate
// unsigned long timer4 = 0; // For jump sound timing
// int period4 = 800;        // Period for jump sound effect

// // Use pins from pins.h for the button and buzzer
// int buttonPin = PIN_BUTTON;
// int buzzerPin = PIN_BUZZER;

// // LED pins from pins.h
// int redLEDPin = PIN_RED;
// int greenLEDPin = PIN_GREEN;
// int blueLEDPin = PIN_BLUE;

// void setup()
// {
//     // Initialize button and buzzer pins
//     pinMode(buttonPin, INPUT);
//     pinMode(buzzerPin, OUTPUT);

//     // Initialize LED pins as outputs
//     pinMode(redLEDPin, OUTPUT);
//     pinMode(greenLEDPin, OUTPUT);
//     pinMode(blueLEDPin, OUTPUT);

//     // Initialize the I2C LCD
//     lcd.init();      // For many I2C LCD libraries, use init() instead of begin()
//     lcd.backlight(); // Turn on the backlight
//     lcd.clear();

//     // Create custom characters for the dino and obstacles
//     lcd.createChar(0, DINO_STANDING_PART_1);
//     lcd.createChar(1, DINO_STANDING_PART_2);
//     lcd.createChar(2, DINO_RIGHT_FOOT_PART_1);
//     lcd.createChar(3, DINO_RIGHT_FOOT_PART_2);
//     lcd.createChar(4, DINO_LEFT_FOOT_PART_1);
//     lcd.createChar(5, DINO_LEFT_FOOT_PART_2);
//     lcd.createChar(6, TWO_BRANCHES_PART_1);
//     lcd.createChar(7, TWO_BRANCHES_PART_2);

//     // Start game with LED green to indicate game is running
//     digitalWrite(greenLEDPin, HIGH);
//     digitalWrite(redLEDPin, LOW);
//     digitalWrite(blueLEDPin, LOW);
// }

// void loop()
// {
//     // Keep LED green during normal gameplay
//     digitalWrite(greenLEDPin, HIGH);
//     digitalWrite(redLEDPin, LOW);

//     // --- Dino Foot Animation ---
//     if (millis() > timer1 + period1)
//     {
//         timer1 = millis();
//         dinoFlag = (dinoFlag == 1) ? 2 : 1;
//     }

//     // --- Obstacle Movement ---
//     if (millis() > timer2 + period2)
//     {
//         timer2 = millis();
//         obstacleCol = obstacleCol - 1;
//         if (obstacleCol < 0)
//         {
//             obstacleCol = 13;
//             period2 = period2 - acceleration; // Increase speed
//             randomObstacle = random(0, 3);    // 0 = bird, 1 or 2 = branch
//         }

//         tempCol = obstacleCol + 1;
//         // Clear previous obstacle positions on both rows
//         lcd.setCursor(tempCol, 1);
//         lcd.print(" ");
//         lcd.setCursor(tempCol, 0);
//         lcd.print(" ");

//         // Clear dino previous positions (if needed)
//         lcd.setCursor(0, 1);
//         lcd.print(" ");
//         lcd.setCursor(0, 0);
//         lcd.print(" ");

//         flagDrawObstacle = 1;
//     }

//     // --- Draw the Dino ---
//     if (jumpState == 0)
//     { // On ground
//         if (dinoFlag == 1)
//         {
//             lcd.setCursor(dinoCol1, dinoRow);
//             lcd.write(byte(2));
//             lcd.setCursor(dinoCol2, dinoRow);
//             lcd.write(byte(3));
//         }
//         else if (dinoFlag == 2)
//         {
//             lcd.setCursor(dinoCol1, dinoRow);
//             lcd.write(byte(4));
//             lcd.setCursor(dinoCol2, dinoRow);
//             lcd.write(byte(5));
//         }
//     }

//     // --- Draw the Obstacle ---
//     if (flagDrawObstacle == 1)
//     {
//         if (randomObstacle == 1)
//         {
//             obstacleRow = 1;
//             lcd.createChar(6, TWO_BRANCHES_PART_1);
//             lcd.setCursor(obstacleCol, obstacleRow);
//             lcd.write(byte(6));
//         }
//         else if (randomObstacle == 2)
//         {
//             obstacleRow = 1;
//             lcd.createChar(7, TWO_BRANCHES_PART_2);
//             lcd.setCursor(obstacleCol, obstacleRow);
//             lcd.write(byte(7));
//         }
//         else
//         { // Bird obstacle
//             birdCol = obstacleCol - 1;
//             obstacleRow = 0;
//             lcd.createChar(6, BIRD_WINGS_PART1);
//             lcd.setCursor(birdCol, obstacleRow);
//             lcd.write(byte(6));

//             lcd.createChar(7, BIRD_WINGS_PART2);
//             lcd.setCursor(obstacleCol, obstacleRow);
//             lcd.write(byte(7));
//         }

//         flagDrawObstacle = 0;
//     }

//     // --- Collision Detection ---
//     // Bird collision (upper row) when button is pressed (dino is jumping)
//     if (digitalRead(buttonPin) == HIGH &&
//         (obstacleCol == 1 || obstacleCol == 2 || birdCol == 1 || birdCol == 2) &&
//         obstacleRow == 0)
//     {
//         gameOver();
//     }

//     // Branch collision (ground) detection
//     if ((obstacleCol == varB || obstacleCol == varC) && obstacleRow == 1)
//     {
//         int notes[] = {200, 150};
//         for (int i = 0; i < 2; i++)
//         {
//             tone(buzzerPin, notes[i], 250);
//             delay(200);
//         }
//         gameOver();
//     }

//     // --- Dino Jump ---
//     if (digitalRead(buttonPin) == HIGH)
//     {
//         // Move collision detection markers out of range when jumping
//         varB = 50;
//         varC = 50;

//         if (jumpState == 0)
//         {
//             lcd.setCursor(0, 1); // Clear the lower row
//             lcd.print("    ");
//         }
//         jumpState = 1;

//         // Draw dino in jump position (upper row)
//         lcd.setCursor(dinoCol1, 0);
//         lcd.write(byte(2));
//         lcd.setCursor(dinoCol2, 0);
//         lcd.write(byte(3));

//         if (millis() > timer4 + period4)
//         {
//             timer4 = millis();
//             int note[] = {600};
//             tone(buzzerPin, note[0], 150);
//             delay(20);
//         }
//     }
//     else
//     {
//         // Reset collision markers when not jumping
//         varB = 1;
//         varC = 2;
//         jumpState = 0;
//     }

//     // --- Score Update ---
//     if (millis() > timer3 + period3)
//     {
//         timer3 = millis();
//         lcd.setCursor(14, 1);
//         lcd.print(score);

//         score = score + 1;

//         if (score == 100)
//         {
//             int notes[] = {800, 900};
//             for (int i = 0; i < 2; i++)
//             {
//                 tone(buzzerPin, notes[i], 150);
//                 delay(150);
//             }
//             score = 0;
//             score2 = score2 + 1;
//             if (score2 == 100)
//             {
//                 score2 = 0;
//             }
//         }

//         lcd.setCursor(14, 1);
//         lcd.print(score);
//         lcd.setCursor(14, 0);
//         lcd.print(score2);

//         // Detect button state change (for clearing parts of the display)
//         currentButtonState = digitalRead(buttonPin);
//         if (currentButtonState != lastButtonState)
//         {
//             lcd.setCursor(1, 0);
//             lcd.print("  ");
//         }
//         lastButtonState = currentButtonState;
//     }
// }

// // Function to handle game over: shows "GAME OVER", sounds the buzzer, sets LED red, and resets game variables.
// void gameOver()
// {
//     // Set LED to red to indicate game over
//     digitalWrite(greenLEDPin, LOW);
//     digitalWrite(redLEDPin, HIGH);

//     lcd.clear();
//     lcd.setCursor(5, 0);
//     lcd.print("GAME OVER");
//     delay(2000);
//     lcd.clear();
//     obstacleCol = 15;
//     period2 = 100;
//     score = 0;
//     score2 = 0;
//     period2 = 100;

//     // Reset LED back to green as the game restarts
//     digitalWrite(redLEDPin, LOW);
//     digitalWrite(greenLEDPin, HIGH);
// }
