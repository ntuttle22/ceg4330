#include <Arduino.h>

const int PWM_PIN = 9;			// OC1A, the Timer1 output pin
const int IN_PIN = 2;			 // connect to PWM_PIN with a jumper wire

const float REPORT_PER = 1.0f;

uint8_t duty = 127;
uint16_t freq = 5000;

uint16_t input_freq = 0;
uint8_t input_duty = 0;
double input_duty_pc = 0;

volatile uint32_t input_rise = 0;
volatile uint32_t input_fall = 0;
volatile uint32_t input_period = 100;
volatile uint32_t input_lo = 0;

uint32_t _timestamp = 0;
uint32_t _delta = 0;

void isr_input_edge();

void setup()
{
	// Setup serial monitor
	Serial.begin(115200);

	// Setup Timer1 for PWM: phase correct, prescaler 8.
	// The timer counts 0 -> ICR1 -> 0 at 2 MHz, so one period is ICR1 microseconds.
	TCCR1A = _BV(COM1A1) | _BV(WGM11);	// PWM output on OC1A
	TCCR1B = _BV(WGM13) | _BV(CS11);		// top = ICR1, prescaler 8
	ICR1 = 1000000 / freq;					// period (us)
	OCR1A = (unsigned long)duty * ICR1 / 255;	// high time (us)

	// Setup GPIO and interrupt
	pinMode(PWM_PIN, OUTPUT);
	pinMode(IN_PIN, INPUT);
	attachInterrupt(digitalPinToInterrupt(IN_PIN), isr_input_edge, CHANGE);

	Serial.println("\nSetup done.");
}

void changeDutyCycle(long value)
{
	// Limit `value` to [0, 255]
	if(!(0 <= value && value <= 255)) return;

	duty = value;
	OCR1A = (unsigned long)duty * ICR1 / 255;
	Serial.print("duty = ");
	Serial.println(duty);

	return;
}

void changeFrequency(long value)
{
	// Limit `value` to [100, 10k]
	if(!(100 <= value && value <= 10000)) return;

	freq = value;
	ICR1 = 1000000 / freq;
	Serial.print("frequency = ");
	Serial.println(freq);

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
	if(100UL <= input_period && input_period <= 10000UL && input_lo <= input_period) {
		input_freq = 1000000UL / input_period;

		// Duty Cycle is the duration HI / Period, out of 255
		input_duty_pc = 1.0 - ((double)input_lo / (double)input_period);
		input_duty = (uint8_t)(input_duty_pc * 255.0);
	}

	//Serial.printf("ESP32: Freq: %d Hz, Duty: %2.1lf\n", input_freq, input_duty_pc * 100.0);
	Serial.print("Uno:");
		Serial.print("Freq:");
		Serial.print(input_freq);
		Serial.print(" Hz, ");
		Serial.print("Duty: ");
		Serial.print(input_duty_pc * 100.0);
	Serial.println("");

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
