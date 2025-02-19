#include <zephyr/kernel.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

#define I2C_NODE DT_ALIAS(i2c0)

#define CLEAR_CMD 0x76

static const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

uint8_t count = 0;
uint16_t clear = 0x76;

int main(void)
{
        printk("Beginning main...\n\n");

        if (!device_is_ready(i2c0_dev)) {
                printk("I2C device not ready :(\n");
                return 0;
        }

        while (1) {
                printk("Loop iteration...\n");

                i2c_write(
                        i2c0_dev,
                        &clear,
                        2,
                        0x71
                );

                k_msleep(50);
                
                i2c_write(
                        i2c0_dev,
                        &count,
                        1,
                        0x71
                );

                k_msleep(1000);

                count++;
                if (count > 32) {
                        count = 0;
                }
        }



        return 0;
}
