#include <Arduino.h>

const int PWM_BITS = 8;     // duty range 0-255
const int PWM_PIN = 2;      // onboard LED
const int IN_PIN = 4;       // connect to PWM_PIN with a jumper wire

const float REPORT_PER = 1.0f;

uint8_t duty = 127;
uint16_t freq = 5000;

uint8_t input_duty = 0;
uint16_t input_freq = 0;

volatile uint32_t input_rise = 0;
volatile uint32_t input_fall = 0;
volatile uint32_t input_period = 1;
volatile uint32_t input_lo = 0;

uint32_t _timestamp = 0;
uint32_t _delta = 0;

void isr_input_edge();

void setup()
{
	// Setup serial monitor
	Serial.begin(115200);

	// Setup GPIO and interrupt
	ledcAttach(PWM_PIN, freq, PWM_BITS);
	ledcWrite(PWM_PIN, duty);

	// Attach interrupts to measure frequency and duty cycle
	attachInterrupt(digitalPinToInterrupt(IN_PIN), isr_input_edge, CHANGE);

	Serial.println("\nSetup done.");
}

void changeDutyCycle(int value)
{
	// Limit `value` to [0, 255]
	if(!(0 <= value && value <= 255)) return;
	
	// Change duty cycle using `ledc`
	duty = value;
	ledcWrite(PWM_PIN, duty);
	Serial.printf("duty cycle = %d / 255\n", duty);

	return;
}

void changeFrequency(int value)
{
	// Limit `value` to [100, 10k]
	if(!(100 <= value && value <= 10000)) return;

	// Change duty cycle using `ledc`
	freq = value;
	ledcChangeFrequency(PWM_PIN, freq, PWM_BITS);
	Serial.printf("frequency = %d\n", freq);

	return;
}

/// @brief Change `duty` or `freq` in the Serial Interface.
///
///	Gets 'd' or 'f' and an INT.
void getSerialInput()
{
	// Returns true if you send data to the dev board through the terminal
	if(!Serial.available()) return;

	// PARAMS: [d|f INT]
	String buf = Serial.readStringUntil('\n');
	int s_int = buf.substring(1).toInt();

	if(buf.substring(0, 1) == "d")
		changeDutyCycle(s_int);

	if(buf.substring(0, 1) == "f")
		changeFrequency(s_int);

	return;
}

/// @brief This interrupt is configured to execute on any edge of input signal.
///
/// Sets `input_fall` and `input_period` in microseconds.
void isr_input_edge()
{
	uint32_t mus = micros();

	// FALLING EDGE
	if(!digitalRead(IN_PIN)) {
		input_period = mus - input_fall;
		input_fall = mus;
	} else
	
	// RISING EDGE
	{
		input_lo = mus - input_fall;
		input_rise = mus;
	}

	return;
}

/// @brief Write `input_freq` and `input_duty` to the serial monitor once per second.
void displayIncomingPWM()
{
	uint32_t ms = millis();
	_delta = ms - _timestamp;

	// If a second has not elapsed, return.
	if(_delta < (uint32_t)(REPORT_PER * 1000.0f)) return;

	// Frequency is the number of periods per second
	input_freq = 1000000UL / input_period;

	// Duty Cycle is the duration HI / Period, out of 255
	double input_duty_pc = 1.0 - ((double)input_lo / (double)input_period);
	input_duty = (uint8_t)(input_duty_pc * 255.0);

	Serial.printf("ESP32: Freq: %d Hz, Duty: %2.1lf\n", input_freq, input_duty_pc * 100.0);
	//Serial.printf("DC = %2.1lf = %d / %d\n", input_duty_pc * 100.0, input_lo, input_period);

	_timestamp = ms;
	return;
}

void loop()
{
	// Get the input from the serial monitor for changing frequency and duty cycle
	getSerialInput();
	
	// Display the incoming PWM signal's duty cycle and frequency
	displayIncomingPWM();
}
