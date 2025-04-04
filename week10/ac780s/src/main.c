#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>

// get the i2c0 device from dt
const static struct device *i2c0_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), ac780s_3c));

int main(void)
{

        if (!device_is_ready(i2c0_dev)) {
                printk("I2C_0 device not ready :(\n");
                return 0;
        } else {
                printk("I2C_0 device is ready!\n");
        }

        k_msleep(500);

        // this is the closest I can get to ¯\_(ツ)_/¯, since there's
        // no preprogrammed backslash in the AC780S...
        uint8_t chars[] = {
            0xCD,
            0x5F,
            0x28,
            0xBC,
            0x29,
            0x5F,
            0x2F,
            0x2D
        };

        int err = auxdisplay_cursor_position_set(i2c0_dev,AUXDISPLAY_POSITION_ABSOLUTE, 0, 1);
        auxdisplay_write(i2c0_dev, chars, sizeof(chars));

        k_msleep(900);

        auxdisplay_position_blinking_set_enabled(i2c0_dev, true);

        err = auxdisplay_cursor_position_set(i2c0_dev,AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
        auxdisplay_write(i2c0_dev, chars, sizeof(chars)-2);

        k_msleep(1800);

        auxdisplay_position_blinking_set_enabled(i2c0_dev, false);

        uint8_t achar = 0x21;


        while (1) {
            
            auxdisplay_write(i2c0_dev, &achar, sizeof(uint8_t));
            
            if ((achar++) > 176) {
                achar = 0;
            }
            
            k_msleep(200);
        }

        return 0;
}
