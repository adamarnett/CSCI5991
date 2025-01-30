/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   5000
#define SLEEP_TIME_MS_II 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

int toggleLEDs(void) {
	// similar to OUTTGL on the AVR-BLE?
	ret = gpio_pin_toggle_dt(&led);
	ret2 = gpio_pin_toggle_dt(&led2);
	// error checking
	if (ret < 0 || ret2 < 0) {
		return 0;
	}
	else {
		return 1;
	}
}

int main(void)
{
	// init variables
	int ret, ret2;
	bool led_state = true;

	// check if pins are ready? perhaps a devicetree thing?
	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&led2)) {
		return 0;
	}
	
	// similar to setting a GPIO pin as output on the AVR-BLE?
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	ret2 = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	// error checking
	if (ret < 0 || ret2 < 0) {
		return 0;
	}

	while (1) {
		// toggle leds --> see function above main
		if (toggleLEDs()) {
			return 0;
		}
		
		// change state
		led_state = !led_state;
		// logging? isn't there like a zephyr logger or something?
		// is that this?
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		// _delay_ms() on the AVR-BLE
		k_msleep(SLEEP_TIME_MS);
		
		// rinse and repeat with a different constant :)
		if (toggleLEDs()) {
			return 0;
		}	
		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS_II);
		if (toggleLEDs()) {
			return 0;
		}
		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS_II);
		


	}
	return 0;
}
