#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

// Define pins
#define IR_PIN 12
#define LED_PIN 2

/// IR Pulse-Position Modulation encoding scheme.
enum IR {
    // INTERVAL +- ERROR
    START = 13500, START_E = 1350,
    ZERO = 1125, ZERO_E = 115,
    ONE = 2250, ONE_E = 225,
    HOLD = 11250, HOLD_E = 1125,

    /// HEX : 2B CMD : 2B ADDR
    N_1 = 0xBA45FF00, N_2 = 0xB946FF00, N_3 = 0xB847FF00,
    N_4 = 0xBB44FF00, N_5 = 0xBF40FF00, N_6 = 0xBC43FF00,
    N_7 = 0xF807FF00, N_8 = 0xEA15FF00, N_9 = 0xF609FF00,
    STAR = 0xE916FF00, N_0 = 0xE619FF00, HASH = 0xF20DFF00,
                    D_U = 0xE718FF00,
    D_L = 0xF708FF00, D_OK = 0xE31CFF00, D_R = 0xA55AFF00,
                    D_D = 0xAD52FF00,
};

/// IR DATA FRAME
/// [ 8b ADDR | 8b ~ADDR | 8b CMD | 8b ~CMD | 1b READY ]
volatile static struct IRFrame {
    uint32_t BUFFER;
    bool READY;
} Frame = {0x00000000, false};

// -- GLOBAL -----------------------------------------------------------------

hw_timer_t *timer = NULL;
uint32_t timer_freq = 10000;

// SLEEP MODE
float time_to_sleep = 10.0f;
uint32_t wake_time = 0;

// LED
bool led_enabled = false;
bool led_state = false;
uint32_t led_freq = 2;
uint8_t led_dc = 128;

// IR_EDGE
volatile uint32_t last_time = 0;
volatile uint8_t frame_counter = 0;

// ---------------------------------------------------------------------------

/// @brief Decode pulse-modulated signal from the IR receiver.
/// Modifies `IRFrame::Frame.BUFFER` and sets `Frame.READY`.
/// @note This ISR is configured to run every time the IR receiver experiences a falling edge.
void irEdge()
{
    uint32_t mus = micros();
    uint32_t delta = mus - last_time;
    last_time = mus;

    if((IR::START - IR::START_E) <= delta && delta <= (IR::START + IR::START_E)) {
        Frame.BUFFER = 0x00000000;
        frame_counter = 0;
    } else

    if((IR::ZERO - IR::ZERO_E) <= delta && delta <= (IR::ZERO + IR::ZERO_E)) {
        frame_counter += 1;
    } else

    if((IR::ONE - IR::ONE_E) <= delta && delta <= (IR::ONE + IR::ONE_E)) {
        Frame.BUFFER = Frame.BUFFER | ((uint32_t)0b1 << frame_counter);
        frame_counter += 1;
    }

    if(frame_counter >= 32) {
        frame_counter = 0;
        Frame.READY = true;
    }

    return;
}

/// @brief Toggle the onboard LED on/off.
/// @note LED is active low on GPIO_2.
/// @note `analogWrite()` now sets brightness by `led_dc`
void led_toggle()
{
	if(led_state) {
		led_state = 0;
        pinMode(LED_PIN, OUTPUT);
		digitalWrite(LED_PIN, HIGH);
	} else {
		led_state = 1;
		//digitalWrite(LED_PIN, LOW);
        analogWrite(LED_PIN, led_dc);
	}
    return;
}

/// @brief Toggle the onboard LED at variable frequency. Only when `led_enabled`.
/// @note This ISR is configured to run every time the timer alarm is triggered.
void onTimer()
{
    if(!led_enabled && !led_state) return;

    led_toggle();
    return;
}

/// @brief Convert IR frame data to a String for output.
/// @param base `BIN`, `HEX`, or 1-36
/// @return Annotated String object.
String frameToStr(uint8_t base)
{
    String output;
    String str = String(Frame.BUFFER, base);

    if(base == BIN)
    {
        output = "";
        for(uint8_t i = 0; i < 32; i++) {
            if(i % 4 == 0) output += ' ';
            output += str.charAt(i);
        }
        output.trim();
        return output;
    }

    if(base == HEX)
    {
        output = str;
        output.trim();
        output.toUpperCase();
        output = "0x" + output;
        return output;
    }

    output = str;
    return output;
}

// ---------------------------------------------------------------------------

void setup()
{
    // Open up serial communication to display debug info
	Serial.begin(9600); // Baud rate is 9600 to communicate via COM on Windows

    // Attach an interrupt to execute when the IR pin experiences a falling edge
    attachInterrupt(digitalPinToInterrupt(IR_PIN), irEdge, FALLING);
    pinMode(IR_PIN, INPUT);

    // Initialize a timer and attach `onTimer()` interrupt
    timer = timerBegin(timer_freq);
    timerAttachInterrupt(timer, onTimer);
    timerAlarm(timer, (timer_freq / led_freq), true, 0);

    // Set the onboard LED pin to be an output
	pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
	led_state = 0;

    // Enable wakeup from light sleep
    esp_sleep_enable_gpio_wakeup();

    Serial.println("\nSetup done!");
}

void loop()
{
    // Enter light sleep after 5 seconds of inactivity
    uint32_t delta = micros() - wake_time;

    if(delta >= (uint32_t)(time_to_sleep * 1000000.0f)) {
        Serial.printf("Entering sleep mode due to inactivity...\n");
        Serial.flush();

        // Disable interrupts
        detachInterrupt(digitalPinToInterrupt(IR_PIN));

        // Disable LED
        if(led_state) led_toggle();
        if(led_enabled) led_enabled = false;

        // Enable wakeup from IR_PIN
        gpio_wakeup_enable((gpio_num_t)IR_PIN, GPIO_INTR_LOW_LEVEL);

        // Start sleeping
        esp_light_sleep_start();

        // Upon waking, set things back up
        Serial.println("HELLO_WORLD!");
        attachInterrupt(digitalPinToInterrupt(IR_PIN), irEdge, FALLING);
        Frame.BUFFER = 0x00;
        Frame.READY = false;
        wake_time = micros();
    }

    // Enable/disable the LED when OK command is received
    if(Frame.READY && Frame.BUFFER == IR::D_OK) {
        Serial.println("IR_CMD: OK");
        led_enabled = !led_enabled;
        led_toggle();
    }

    // Increase/decrease `led_freq` when right/left arrow is received
    if(Frame.READY)
    {
        if(Frame.BUFFER == IR::D_R) {
            Serial.println("IR_CMD: \u21E8");
            if(led_freq < 256) led_freq *= 2;
            timerAlarm(timer, (timer_freq / led_freq), true, 0);
        }

        if(Frame.BUFFER == IR::D_L) {
            Serial.println("IR_CMD: \u21E6");
            if(led_freq > 1) led_freq /= 2;
            timerAlarm(timer, (timer_freq / led_freq), true, 0);
        }
    }

    // Decrease/increase `led_dc` [0, 255] when up/down arrow is received
    if(Frame.READY)
    {
        if(Frame.BUFFER == IR::D_U) {
            Serial.println("IR_CMD: \u21E7");
            led_dc -= 16;
        }

        if(Frame.BUFFER == IR::D_D) {
            Serial.println("IR_CMD: \u21E9");
            led_dc += 16;
        }
    }

    if(Frame.READY) {
        // Serial output
        Serial.println("IR_FRAME: { " + frameToStr(BIN) + " }");
        Serial.println("IR_HEX: " + frameToStr(HEX));

        wake_time = micros();
        Frame.READY = false;
    }
}
