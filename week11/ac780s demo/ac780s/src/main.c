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
    //printk("W:0x%04x\n", addr);
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

        // set custom chars
        uint8_t sadArr[] = {
            0 ,255, 0 ,255, 0 ,
           255, 0 , 0 , 0 ,255,
            0 , 0 , 0 , 0 , 0 ,
            0 ,255, 0 ,255, 0 ,
            0 , 0 , 0 , 0 , 0 ,
            0 ,255,255,255, 0 ,
           255, 0 , 0 , 0 ,255,
            0 , 0 , 0 , 0 , 0 
        };
        uint8_t backslashArr[] = {
            0 , 0 , 0 , 0 , 0 ,
            0 , 0 , 0 , 0 ,255,
            0 , 0 , 0 ,255, 0 ,
            0 , 0 ,255, 0 , 0 ,
            0 ,255, 0 , 0 , 0 ,
           255, 0 , 0 , 0 , 0 ,
            0 , 0 , 0 , 0 , 0 ,
            0 , 0 , 0 , 0 , 0 
        };
        uint8_t angyArr[] = {
           255, 0 , 0 , 0 ,255,
            0 ,255, 0 ,255, 0 ,
            0 , 0 , 0 , 0 , 0 ,
            0 ,255, 0 ,255, 0 ,
            0 , 0 , 0 , 0 , 0 ,
            0 ,255,255,255, 0 ,
           255, 0 , 0 , 0 ,255,
            0 , 0 , 0 , 0 , 0 
        };
        uint8_t happyArr[] = {
            0 , 0 , 0 , 0 , 0 ,
            0 ,255, 0 ,255, 0 ,
            0 ,255, 0 ,255, 0 ,
            0 , 0 , 0 , 0 , 0 ,
           255, 0 , 0 , 0 ,255,
            0 ,255,255,255, 0 ,
            0 , 0 , 0 , 0 , 0 ,
            0 , 0 , 0 , 0 , 0 
        };

        struct auxdisplay_character backslash = {
            .character_code = 99,
            .index = 0,
            .data = backslashArr
        };
        struct auxdisplay_character happy = {
            .character_code = 99,
            .index = 1,
            .data = happyArr
        };
        struct auxdisplay_character angy = {
            .character_code = 99,
            .index = 2,
            .data = angyArr
        };
        struct auxdisplay_character sad = {
            .character_code = 99,
            .index = 3,
            .data = sadArr
        };

        int err = 0;

        err = auxdisplay_custom_character_set(
            i2c0_dev,
            &backslash
        );
        if (err) {
            printk("ERR [%d]\n", err);
        }

        err = auxdisplay_custom_character_set(
            i2c0_dev,
            &happy
        );
        if (err) {
            printk("ERR [%d]\n", err);
        }

        err = auxdisplay_custom_character_set(
            i2c0_dev,
            &angy
        );
        if (err) {
            printk("ERR [%d]\n", err);
        }

        err = auxdisplay_custom_character_set(
            i2c0_dev,
            &sad
        );
        if (err) {
            printk("ERR [%d]\n", err);
        }

        printk("char code of backslash: [%d]\n", backslash.character_code);
        printk("char code of happy: [%d]\n", happy.character_code);
        printk("char code of angy: [%d]\n", angy.character_code);
        printk("char code of sad: [%d]\n", sad.character_code);

        // this is the closest I can get to ¯\_(ツ)_/¯, since there's
        // no preprogrammed backslash in the AC780S...
        uint8_t chars[] = {
            0x2D,
            0x00,
            0x5F,
            0x28,
            0xBC,
            0x29,
            0x5F,
            0x2F//,
            //0x2D
        };

        err = auxdisplay_write(i2c0_dev, chars, sizeof(chars));
        if (err) {
            printk("ERR [%d]\n", err);
        }

        k_msleep(9001);

        uint8_t x = 0;
        while (1) {

            print_addr(i2c0_dev, x);
            k_msleep(1000);

            x++;
        }


        return 0;
}
