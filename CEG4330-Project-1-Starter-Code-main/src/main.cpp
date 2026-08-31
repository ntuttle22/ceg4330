#include <Arduino.h>

#define LED_PIN 2
#define BTN_PIN 12

#define LO_TIME 1 * 1000000
#define HI_TIME 2 * 1000000

unsigned long last_time;
bool led_state;

void led_toggle();
void led_hi_duty();
void led_lo_duty();
void led_nb();
void led_block();

void setup()
{
	// Open up serial communication to display debug info
	Serial.begin(9600); // Baud rate is 9600 to communicate via COM on Windows

	// Set the onboard LED pin to be an output
	pinMode(LED_PIN, OUTPUT);
	// Set the push-button pin as an input
	pinMode(BTN_PIN, INPUT_PULLUP);

	digitalWrite(LED_PIN, LOW);
	led_state = 0;

	// Record the program setup time in microseconds
	last_time = micros();
}

void loop()
{
	// Starter code
	//led_block();

	//led_nb();

	if(digitalRead(BTN_PIN)) {
		led_hi_duty();
	} else {
		led_lo_duty();
	}
}

void led_toggle()
{
	if(led_state) {
		led_state = 0;
		digitalWrite(LED_PIN, LOW);
	} else {
		led_state = 1;
		digitalWrite(LED_PIN, HIGH);
	}
}

void led_hi_duty()
{
	// Determine how much time has passed since the last loop
	long c_time = micros();

	if(led_state && abs(((long)c_time - (long)last_time)) > 1000) {
		last_time = c_time;
		led_toggle();
	} else
	if(!led_state && abs(((long)c_time - (long)last_time)) > 3000) {
		last_time = c_time;
		led_toggle();
	}
}

void led_lo_duty()
{
	// Determine how much time has passed since the last loop
	long c_time = micros();

	if(led_state && abs(((long)c_time - (long)last_time)) > 3000) {
		last_time = c_time;
		led_toggle();
	} else
	if(!led_state && abs(((long)c_time - (long)last_time)) > 1000) {
		last_time = c_time;
		led_toggle();
	}
}

void led_nb()
{
	// Determine how much time has passed since the last loop
	long c_time = micros();

	if(led_state && abs(((long)c_time - (long)last_time)) > LO_TIME) {
		last_time = c_time;
		led_toggle();
	} else
	if(!led_state && abs(((long)c_time - (long)last_time)) > HI_TIME) {
		last_time = c_time;
		led_toggle();
	}
}

void led_block()
{
	// Write to the serial monitor the status of the LED and turn the LED on	(active low)
	Serial.write("Onboard LED: ON\n");
	digitalWrite(LED_PIN, LOW);
	delay(1000); // Wait for 1 second
	// Write to the serial monitor the status of the LED and turn the LED off (active low)
	Serial.write("Onboard LED: OFF\n");
	digitalWrite(LED_PIN, HIGH);
	delay(1000); // Wait for 1 second
}
