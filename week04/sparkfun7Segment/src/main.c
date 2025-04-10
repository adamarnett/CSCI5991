#include <zephyr/kernel.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>


/*
Writes "CPM" to display, clears it, writes some number, clears that, repeats

CPM stands for counts per minute, and the m is really two lower case n's, since
the display can't really do an m.

*/

// define the node alias for i2c
#define I2C_NODE DT_ALIAS(i2c0)

// get the i2c0 device from dt
static const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

// random things to write to display
uint8_t count = 0;
uint8_t count2 = 1;
uint8_t cChar = 0x0C;
uint8_t pChar = 0x70; // shows "P"
uint8_t mChar = 0x4E; // shows "n", write twice to get m

// display clear command
uint8_t clear = 0x76;

int main(void)
{
        printk("Beginning main...\n\n");

        // check if i2c is ready to go
        if (!device_is_ready(i2c0_dev)) {
                printk("I2C device not ready :(\n");
                return 0;
        }

        while (1) {
                printk("Loop iteration...\n");

                // write clear command to display
                i2c_write(
                        i2c0_dev,       // i2c controller device
                        &clear,         // address of clear command
                        1,              // num bytes to write
                        0x71            // i2c target address
                );

                i2c_write(
                        i2c0_dev,
                        &cChar,
                        1,
                        0x71
                );

                i2c_write(
                        i2c0_dev,
                        &pChar,
                        1,
                        0x71
                );

                i2c_write(
                        i2c0_dev,
                        &mChar,
                        1,
                        0x71
                );

                i2c_write(
                        i2c0_dev,
                        &mChar,
                        1,
                        0x71
                );

                k_msleep(500);

                i2c_write(
                        i2c0_dev,
                        &clear,
                        1,
                        0x71
                );

                i2c_write(
                        i2c0_dev,
                        &count2,
                        1,
                        0x71
                );

                i2c_write(
                        i2c0_dev,
                        &count,
                        1,
                        0x71
                );
                
                k_msleep(500);
                
                if (count++ > 15) {
                        count = 0;
                }
        }



        return 0;
}
