#include <Arduino.h>

// GPIO macros
#define LED_PIN 2
#define BUTTON_PIN 12
#define SPEAKER_PIN 18

// Keypad macros
#define ROWS 4
#define COLS 4

// Row pins and column pins for easy iteration of keypad
uint8_t rowPins[ROWS] = {13, 14, 27, 26};
uint8_t colPins[COLS] = {25, 33, 32, 4 };

// Characters for the keypad matrix
char keypad[ROWS][COLS] = {{'1', '2', '3', 'A'},
                           {'4', '5', '6', 'B'},
                           {'7', '8', '9', 'C'},
                           {'*', '0', '#', 'D'}};

void setup()
{
  // Open up serial communication to display debug info
  Serial.begin(9600); // Baud rate is 9600 to communicate via COM on Windows
}

void handleButtonPress()
{

}

char getKeypadPress()
{

}

void handleTone(char key)
{

}

void loop()
{
  // Handle things
  handleButtonPress();
  char key = getKeypadPress();
  handleTone(key);
}