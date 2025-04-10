/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   42

/* The devicetree node identifier for the "led0" alias. 
(plus led1, led2, and led3)*/
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

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
	LOG_DBG("ledPins 0: [%d]", (ledPins & (1 << 0)));
	if ( gpio_pin_set_dt(&led0, (ledPins & (1 << 0)) ) < 0) {
		// return error if led 0 could not be toggled
		return 0;
	}
    // set led 1
	LOG_DBG("ledPins 1: [%d]", (ledPins & (1 << 1)));
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

int main(void)
{

	// check if pins are ready? perhaps a devicetree thing?
	if (
		!gpio_is_ready_dt(&led0) || 
		!gpio_is_ready_dt(&led1) || 
		!gpio_is_ready_dt(&led2) || 
		!gpio_is_ready_dt(&led3)
		) {
		return 0;
	}
	
	// similar to setting a GPIO pin as output on the AVR-BLE
	// error checking
	if (
		gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE) < 0 || 
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) < 0 || 
		gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) < 0 || 
		gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE) < 0
		) {
		return 0;
	}

    // init array for setting LEDs
	int ledPins = 0;
    // make sure all LEDs are off
	setLEDs(ledPins);
    // wait 42 ms
	k_msleep(SLEEP_TIME_MS);



	while (1) {
		
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
    // shouldn't ever hit this
	return 0;
}
