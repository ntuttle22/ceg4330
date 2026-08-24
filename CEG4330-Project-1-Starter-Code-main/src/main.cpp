#include <Arduino.h>
#define LED_PIN 2

void setup()
{
  // Open up serial communication to display debug info
  Serial.begin(9600); // Baud rate is 9600 to communicate via COM on Windows

  // Set the onboard LED pin to be an output
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  // Write to the serial monitor the status of the LED and turn the LED on  (active low)
  Serial.write("Onboard LED: ON\n");
  digitalWrite(LED_PIN, LOW);
  delay(1000); // Wait for 1 second
  
  // Write to the serial monitor the status of the LED and turn the LED off (active low)
  Serial.write("Onboard LED: OFF\n");
  digitalWrite(LED_PIN, HIGH);
  delay(1000); // Wait for 1 second
}