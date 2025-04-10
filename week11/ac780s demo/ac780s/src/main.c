#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>

// get the i2c0 device from dt
const static struct device *i2c0_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), ac780s_3c));


int print_addr(
    const struct device* dev,
    uint8_t addr
) {
    // clear display
    auxdisplay_clear(dev);
    auxdisplay_cursor_position_set(
        dev,
        AUXDISPLAY_POSITION_ABSOLUTE,
        0,
        0
    );
    
    // write hex addr on top line
    uint8_t firstBuf[8];
    snprintk(firstBuf, sizeof(uint8_t)*8, "W0x%04x  ", addr);
    firstBuf[7] = 0x10;
    printk("W:0x%04x\n", addr);
    auxdisplay_write(i2c0_dev, firstBuf, 8*sizeof(uint8_t));
    // write addr to display on second
    uint8_t secondBuf[1] = {addr};
    auxdisplay_write(i2c0_dev, secondBuf, sizeof(uint8_t));

    return 0;
}




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

        auxdisplay_cursor_set_enabled(i2c0_dev, true);

        k_msleep(9001);

        // set custom char
        uint8_t dataa[] = {
            0 , 0 , 0 , 0 , 0 ,
            0 ,255, 0 ,255, 0 ,
            0 ,255, 0 ,255, 0 ,
            0 , 0 , 0 , 0 , 0 ,
           255, 0 , 0 , 0 ,255,
            0 ,255, 0 ,255, 0 ,
            0 , 0 ,255, 0 , 0 ,
            0 , 0 , 0 , 0 , 0 
        };

        struct auxdisplay_character newChar = {
            .character_code = 0b10010001,
            .index = 0b10010001,
            .data = dataa
        };
        struct auxdisplay_character* pointy = &newChar;

        int err = auxdisplay_custom_character_set(
            i2c0_dev,
            pointy
        );

        uint8_t x = 0;
        while (1) {

            print_addr(i2c0_dev, x);
            k_msleep(1000);

            x++;
        }





        return 0;
}
