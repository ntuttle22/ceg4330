#include <Arduino.h>

// GPIO macros
#define LED_PIN 2
#define BUTTON_PIN 4
#define BUTTON2_PIN 5
#define SPEAKER_PIN 18

// Keypad macros
#define ROWS 4
#define COLS 4

#define DB_THRESHOLD 0.01

// 1 for Keypad, 2 for Tone
int8_t test = 1;

// BUTTONS
uint32_t last_time = 0;
uint32_t down_time = 0;
bool btn_state = 0;
bool prev_state = 0;
uint32_t count = 0;

// TONE GENERATOR
bool tone_state = 0;
uint8_t octave = 0;

// Row pins and column pins for easy iteration of keypad
uint8_t rowPins[ROWS] = {32, 33, 25, 26};
uint8_t colPins[COLS] = {27, 14, 12, 13};

// Characters for the keypad matrix
char keypad[ROWS][COLS] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

void setup()
{
	// Open up serial communication to display debug info
	Serial.begin(9600); // Baud rate is 9600 to communicate via COM on Windows

	// Set the push-button pin as an input
	pinMode(BUTTON_PIN, INPUT_PULLDOWN);
	pinMode(BUTTON2_PIN, INPUT_PULLDOWN);

	// Set keypad rows as input, cols as output
	for(uint8_t r = 0; r < 4; r++) pinMode(rowPins[r], INPUT_PULLUP);
	for(uint8_t c = 0; c < 4; c++) pinMode(colPins[c], OUTPUT);
}

void handleButtonPress()
{
	uint32_t delta = millis() - last_time;

	// Debounce input
	if(delta < DB_THRESHOLD * 1000) return;

	// Toggle input state
	if(digitalRead(BUTTON_PIN) || digitalRead(BUTTON2_PIN))
		btn_state = 1;
	else btn_state = 0;

	// Record button press on rising edge
	if(btn_state && !prev_state) {
		prev_state = btn_state;

		// Record current time for debouncing
		last_time = millis();
		down_time = millis();

		count++;
		//Serial.printf("Count: %d @ %d ms\n", count, delta);

		if(digitalRead(BUTTON_PIN)) {
			 octave++;
			 Serial.println("octave++");
		}
		if(digitalRead(BUTTON2_PIN)){
			 octave--;
			 Serial.println("octave--");
		}
	}

	// Falling edge
	if(!btn_state && prev_state) {
		prev_state = btn_state;

		// Button press duration
		float dur = (float)(millis() - down_time) / 1000.0;
		//Serial.printf("Press: %f\n", dur);

		// Record current time for debouncing
		last_time = millis();
	}

	return;
}

char getKeypadPress()
{
	int8_t row = -1;
	int8_t col = -1;

	// Iterate over each column
	for(uint8_t i = 0; i < 4; i++)
	{
		// -- PIN ON --
		digitalWrite(colPins[i], LOW);

		// Find the connected row
		for(uint8_t j = 0; j < 4; j++)
		{
			if(!digitalRead(rowPins[j])) {
				col = i;
				row = j;
			}
		}

		// -- PIN OFF --
		digitalWrite(colPins[i], HIGH);

		if(col >= 0 || row >= 0) break;
	}

	// Check array bounds
	if( col < 0 || col > 3 ||
		row < 0 || row > 3 )
	{
		//Serial.printf("Keypress OoB.\n");
		return 0x00;
	}

	// Convert keypress to char
	char key = 0x00;

	key = keypad[row][col];
	if(test == 1) Serial.println(key);

	return key;
}

double _keyToFreq(char key)
{
	int8_t n = 0;

	switch(key) {
		case '1' : n = 40; break;
		case '2' : n = 41; break;
		case '3' : n = 42; break;
		case '4' : n = 43; break;
		case '5' : n = 44; break;
		case '6' : n = 45; break;
		case '7' : n = 46; break;
		case '8' : n = 47; break;
		case '9' : n = 48; break;
		case '*' : n = 49; break;
		case '0' : n = 50; break;
		case '#' : n = 51; break;
	}

	// Change octave
	n += octave * 12;

	// FREQ is given by f(n) = 440 * 2^( (n - 49) / 12 )
	double freq = pow(2.0, (n - 49.0) / 12.0);
	freq *= 440.0;

	//Serial.printf("FREQ: %f\n", freq);

	return freq;
}

void handleTone(char key)
{
	// Toggle TONE off and suppress warnings
	if(!key) {
		if(tone_state) noTone(SPEAKER_PIN);
		tone_state = 0;
		return;
	}

	// Get frequency as an integer
	uint32_t freq = _keyToFreq(key);
	if(!tone_state) Serial.printf("FREQ: %d Hz\n", freq);

	// TONE on
	tone(SPEAKER_PIN, freq);
	tone_state = 1;

	return;
}

void loop()
{
	// Handle things
	handleButtonPress();
	char key = getKeypadPress();
	if(test == 2) handleTone(key);

}
