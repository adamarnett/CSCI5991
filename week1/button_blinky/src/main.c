

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   	40
#define SLEEP_TIME_MS_LONG  400

// enum for LED state
enum LED_state {
	spin,
	flash,
	on,
	off
};

// global state variable
enum LED_state state = on;

// global state updated variable
int update = 0;

// init int for setting LEDs
//		bit 0 = led0
//		bit 1 = led1
// 		bit 2 = led2
//		bit 3 = led3
// 0b1111 = all leds on
// 0b0001 = just led0 on
int ledPins = 0;

/* The devicetree node identifier for the "led0" alias. 
(plus led1, led2, and led3)*/
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)


static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static const struct gpio_dt_spec sw1 = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});
static const struct gpio_dt_spec sw2 = GPIO_DT_SPEC_GET_OR(SW2_NODE, gpios, {0});
static const struct gpio_dt_spec sw3 = GPIO_DT_SPEC_GET_OR(SW3_NODE, gpios, {0});

// struct to hold info about interrupt (callback)
static struct gpio_callback sw0_cb_data;
static struct gpio_callback sw1_cb_data;
static struct gpio_callback sw2_cb_data;
static struct gpio_callback sw3_cb_data;


// create logger, can be named arbitrarily
LOG_MODULE_REGISTER(logger, LOG_LEVEL_DBG);

/*
int setLEDs(int tgl0, int tgl1, int tgl2, int tgl3)

set (turn on) or unset (turn off) onboard LEDs

This really should take an array of ints

int tgl0 - if 0, do not set LED 0, else set it
int tgl1 - if 1, do not set LED 0, else set it
int tgl2 - if 2, do not set LED 0, else set it
int tgl3 - if 3, do not set LED 0, else set it

returns 0 on error, 1 for no error
*/
int setLEDs(int ledPins) {

    // set led 0
	//LOG_DBG("ledPins 0: [%d]", (ledPins & (1 << 0)));
	if ( gpio_pin_set_dt(&led0, (ledPins & (1 << 0)) ) < 0) {
		// return error if led 0 could not be toggled
		return 0;
	}
    // set led 1
	//LOG_DBG("ledPins 1: [%d]", (ledPins & (1 << 1)));
	if ( gpio_pin_set_dt(&led1, (ledPins & (1 << 1))) < 0) {
		// return error if led 1 could not be toggled
		return 0;
	}
    // set led 2
	if ( gpio_pin_set_dt(&led2, (ledPins & (1 << 2))) < 0) {
		// return error if led 2 could not be toggled
		return 0;
	}
    // set led 3
	if ( gpio_pin_set_dt(&led3, (ledPins & (1 << 3))) < 0) {
		// return error if led 3 could not be toggled
		return 0;
	}


	// return 1 for no error
	return 1;
}

void sw0_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	//LOG_DBG("Button pressed at %" PRIu32 " ", k_cycle_get_32());
	//LOG_DBG("Button that was pressed is %s \n", dev->name);
	LOG_DBG("SW0 pressed.");
	update = 1;
	state = on;
}

void sw1_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_DBG("SW1 pressed.");
	update = 1;
	state = off;
}

void sw2_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_DBG("SW2 pressed.");
	update = 1;
	state = spin;
}

void sw3_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_DBG("SW3 pressed.");
	update = 1;
	state = flash;
}

// funciton for leds "spinning"
void stateSpin(void) {
	LOG_DBG("Entering stateSpin...");
	while (!update) {
		// turn on only LED 1
        // set bit corresponding to led0 to 1 to turn on, all others to 0
        ledPins = 1;
        // call function to set leds
		setLEDs(ledPins);
        // wait 42 ms
		k_msleep(SLEEP_TIME_MS);
        
        //turn on only LED 2
        ledPins = ledPins << 1;
		setLEDs(ledPins);
		k_msleep(SLEEP_TIME_MS);

        // turn on LED 4 --> skip 3 to go in a little circle
        ledPins = ledPins << 2;
        setLEDs(ledPins);
		k_msleep(SLEEP_TIME_MS);
        
        // turn on LED 3 --> go back to 3
        ledPins = ledPins >> 1;
		setLEDs(ledPins);
		k_msleep(SLEEP_TIME_MS);
	}
}

// funciton for leds flashing
void stateFlash(void) {
	LOG_DBG("Entering stateFlash...");
	while (!update) {

		// turn off all leds
		// 15 = 0b0000 = all off
		ledPins = 0;
		// call function to set leds
		setLEDs(ledPins);

		k_msleep(SLEEP_TIME_MS_LONG);
        
		// turn on all leds
        // 15 = 0b1111 = all on
		ledPins = 15;
		// call function to set leds
		setLEDs(ledPins);

		k_msleep(SLEEP_TIME_MS_LONG);

	}
}

// funciton for all leds on
void stateOn(void) {
	LOG_DBG("Entering stateOn...");

	// 15 = 0b1111 = all on
	ledPins = 15;
	// call function to set leds
	setLEDs(ledPins);

	while (!update) {
        // wait 42 ms
		k_msleep(SLEEP_TIME_MS);
	}
}

// funciton for all leds off
void stateOff(void) {
	LOG_DBG("Entering stateOff...");

	// 0 = 0b0000 = all off
	ledPins = 0;
	// call function to set leds
	setLEDs(ledPins);

	while (!update) {
        // wait 42 ms
		k_msleep(SLEEP_TIME_MS);
	}
}

int main(void)
{
	LOG_DBG("Entering main...");

	// check if pins are ready? perhaps a devicetree thing?
	if (
		!gpio_is_ready_dt(&led0) || 
		!gpio_is_ready_dt(&led1) || 
		!gpio_is_ready_dt(&led2) || 
		!gpio_is_ready_dt(&led3) ||
		!gpio_is_ready_dt(&sw0)  ||
		!gpio_is_ready_dt(&sw1)  ||
		!gpio_is_ready_dt(&sw2)  ||
		!gpio_is_ready_dt(&sw3)
		) {
		return 0;
	}
	
	LOG_DBG("gpio_is_ready_dt returned normally...");


	// similar to setting a GPIO pin as output on the AVR-BLE
	// error checking
	if (
		gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&sw0, GPIO_INPUT) 		 ||
		gpio_pin_configure_dt(&sw1, GPIO_INPUT) 		 ||
		gpio_pin_configure_dt(&sw2, GPIO_INPUT) 		 ||
		gpio_pin_configure_dt(&sw3, GPIO_INPUT)
		) {
		return 0;
	}

	LOG_DBG("gpio_pin_configure_dt returned normally...");

	// allow/setup interrupts
	if (
		gpio_pin_interrupt_configure_dt(&sw0,
			GPIO_INT_EDGE_FALLING) ||
		gpio_pin_interrupt_configure_dt(&sw1,
			GPIO_INT_EDGE_FALLING) ||
		gpio_pin_interrupt_configure_dt(&sw2,
			GPIO_INT_EDGE_FALLING) ||
		gpio_pin_interrupt_configure_dt(&sw3,
			GPIO_INT_EDGE_FALLING)
	) {
		return 0;
	}

	LOG_DBG("gpio_pin_interrupt_configure_dt returned normally...");

	// needs error checking, assign func to interrupt
	gpio_init_callback(&sw0_cb_data, sw0_pressed, BIT(sw0.pin));
	gpio_init_callback(&sw1_cb_data, sw1_pressed, BIT(sw1.pin));
	gpio_init_callback(&sw2_cb_data, sw2_pressed, BIT(sw2.pin));
	gpio_init_callback(&sw3_cb_data, sw3_pressed, BIT(sw3.pin));
	gpio_add_callback(sw0.port, &sw0_cb_data);
	gpio_add_callback(sw1.port, &sw1_cb_data);
	gpio_add_callback(sw2.port, &sw2_cb_data);
	gpio_add_callback(sw3.port, &sw3_cb_data);

	LOG_DBG("gpio_add_callback returned normally...");


    
    // make sure all LEDs are off
	setLEDs(ledPins);
    // wait 42 ms
	k_msleep(SLEEP_TIME_MS);

	LOG_DBG("Entering while (1)...");

	while (1) {

		update = 0;
        
		switch (state)
		{
		case on:
			stateOn();
			break;
		case off:
			stateOff();
			break;
		case spin:
			stateSpin();
			break;
		case flash:
			stateFlash();
			break;
		}

	}
    // shouldn't ever hit this
	return 0;
}
