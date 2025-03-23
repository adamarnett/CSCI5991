#include "ht16k33.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>




#define CMD_STARTUP 0b00100000
#define CMD_SETUP 0b10000000

// get the i2c0 device from dt
static struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

//static const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

#define I2C1_NODE DT_NODELABEL(vk16k33)
static struct i2c_dt_spec i2c1_dev = I2C_DT_SPEC_GET(I2C1_NODE);

int initI2C() {
    // check if i2c is ready to go
    if (!device_is_ready(i2c0_dev)) {
        printk("I2C device not ready :(\n");
        return 0;
    }
    
    if (!device_is_ready(i2c1_dev.bus)) {
        printk("I2C device not ready :(\n");
        return 0;
    }
    
    return 1;
}

void writeHT16K33() {

    uint8_t three = 3;

    uint8_t testBuf[] = {
        0b00000110001101, // S
        0b00000101110110, // H
        0b01001000001001, // I
        0b01001000000001  // T
    };

    

    i2c_write_dt(
        &i2c1_dev,
        (CMD_STARTUP | 1),
        1
    );

    k_msleep(2);

    i2c_write_dt(
        &i2c1_dev,
        (CMD_SETUP | 1),
        1
    );

    k_msleep(200);


    while (1) {
        i2c_write_dt(
            &i2c1_dev,
            testBuf,
            4
        );
    }
    
    

    //i2c_write(
    //    i2c0_dev,
    //    &three,
    //    1,
    //    0x71
    //);

    //return 0;
}