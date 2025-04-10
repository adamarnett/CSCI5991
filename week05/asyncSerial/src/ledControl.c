
#include <zephyr/drivers/gpio.h>


// dt aliases for leds
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

// gpio specs for leds
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

// array for organization
void* ledArr[] = {&led0, &led1, &led2, &led3};

// turn on an led at a certain index
void ledOn(uint8_t index) {
    gpio_pin_set_dt(ledArr[index], 1);
}

// turn off an led at a certain index
void ledOff(uint8_t index) {
    gpio_pin_set_dt(ledArr[index], 0);
}

// setup leds
// returns 0 on error, otherwise 1
int setupLeds() {
    if (
        !gpio_is_ready_dt(&led0) || 
		!gpio_is_ready_dt(&led1) || 
		!gpio_is_ready_dt(&led2) || 
		!gpio_is_ready_dt(&led3)
    ) {
        return 0;
    }

    if (
        gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) ||
		gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE)
    ) {
        return 0;
    }

    ledOff(0);
    ledOff(1);
    ledOff(2);
    ledOff(3);

    return 1;
}

// flash endlessly to indicate an error
void errorLed(uint8_t index) {

    switch (index)
    {
    case 4:
        while (1) {
            ledOn(1);
            ledOn(2);
            k_msleep(50);
            ledOff(1);
            ledOff(2);
            k_msleep(50);
        }

    case 5:
        while (1) {
            ledOn(2);
            ledOn(3);
            k_msleep(50);
            ledOff(2);
            ledOff(3);
            k_msleep(50);
        }
    
    default:
        while (1) {
            ledOn(index);
            k_msleep(50);
            ledOff(index);
            k_msleep(50);
        }
    };
    
}