#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

//#include "ht16k33.h"

// get the i2c0 device from dt
static struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));




// display uses 24-pin package, so target address is
// 1 1 1 0 0 A1 A0 R/W
// where A1 and A0 are set to 0 by default, but two jumpers on the
// board (labeled A1 and A0) can be soldered to change those to 1
// so the address should be ((0x70 << 1) & 0x01) for read, and 
// (0x70 << 1) for write
#define TARGET_ADDR_WRITE       (uint16_t)(0x70)//(0x70 << 1)
#define TARGET_ADDR_READ        (uint16_t)((0x70 << 1) & 0x01)

// system startup command is 
// 0 0 1 0 X X X S 
// where S is 1 for turn on system oscillator and 0 is for turn off
//#define CMD_SYS_ON              (uint8_t)0x21//0b00100001
const uint8_t CMD_SYS_ON[] = {0b00100001};

// dimming set command is 
// 1 1 1 0 P3 P2 P1 P0
// where P3, P2, P1, and P0 represent the duty cycle in binary with
// P3 being the most significant bit and P0 being the least
// duty cycle = (1+0bP3P2P1P0)/16, so if all are zero duty cycle is
// 1/16 (min brightness) and if all are 1 it's 16/16 (max brightness)
//#define CMD_BRIGHT_MAX          (uint8_t)0b11101111
const uint8_t CMD_BRIGHT_MAX[] = {0b11101111};
//#define CMD_BRIGHT_MIN          (uint8_t)0b11100000

// display setup/blinking set command is
// 1 0 0 0 X B1 B0 D
// where B1 and B0 are the blinking frequency in binary and D is 
// display on or off (0 for off, 1 for on) if B1 and B0 are both
// zero, there is no blinking, if both are 1 the display will blink
// at a rate of 2Hz
// this command is neccessary to turn on the display (D = 1)
//#define CMD_DISP_ON_NO_BLINK    (uint8_t)0b10000001
const uint8_t CMD_DISP_ON_NO_BLINK[] = {0b10000001};

// display data address pointer command is
// 0 0 0 0 A3 A2 A1 A0
// where A3, A2, A1, and A0 are the display RAM addess in binary
// since the rest is just zero, this can be &'ed with whatever 
// address the pointer should be changed to
// still defining something for consistency amoung the commands
//#define CMD_CHANGE_RAM_POINTER  (uint8_t)0b00000000
const uint8_t CMD_CHANGE_RAM_POINTER[] = {0b00000000};

int main(void)
{       

        printk("Hello! Entered main.\n");


        k_msleep(500);

        if (!device_is_ready(i2c0_dev)) {
                printk("I2C device not ready :(\n");
                return 0;
        } else {
                printk("I2C device is ready!\n");
        }

        

        k_msleep(500);


        // Initialization

        printk("Turn on system oscillator\n");
        // write command to turn on system oscillator
        //const uint8_t* cmd_start[1] = {(uint8_t)0x21};
        i2c_write(
                i2c0_dev,
                CMD_SYS_ON,
                sizeof(CMD_SYS_ON),
                TARGET_ADDR_WRITE
        );

        // might need to do ROW/INT set here, but arduino lib 
        // does not do that so I won't either

        printk("Set LED brightness\n");
        // set LED brightness
        i2c_write(
                i2c0_dev,
                CMD_BRIGHT_MAX,
                sizeof(CMD_BRIGHT_MAX),
                TARGET_ADDR_WRITE
        );

        printk("Turn on display, set blinking to 0Hz\n");
        // turn on display, set blinking rate to zero
        i2c_write(
                i2c0_dev,
                CMD_DISP_ON_NO_BLINK,
                sizeof(CMD_DISP_ON_NO_BLINK),
                TARGET_ADDR_WRITE
        );



        // Display Data Rewrite
        
        // buffer to hold RAM address and data to be written
        uint8_t writeBuf[2] = {CMD_CHANGE_RAM_POINTER, 0xFF};

        // set RAM address and write data
        printk("Write 0xFF to address 0x00 in RAM\n");
        i2c_write(
                i2c0_dev,
                writeBuf,
                sizeof(writeBuf),
                TARGET_ADDR_WRITE
        );

        while (1) {
                k_msleep(9999999);
        }


        return 0;
}
