#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>

// get the i2c0 device from dt
//static struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device *const i2c0_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), quadalpha_70));

//const struct device *const i2c1_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c1), serlcd_72));

// characters
// 14 of the 16 bits, each corresponds to one of the segments
/*
The drawing isn't the worlds greatest, so descriptions of some of
the labels follow the drawing. The diagonal segments couldn't really
have their own boxes drawn... But the following is how segments will
correspond to bits in uint16_t that will represent a char for the 
display
 ┌──────────┐ 
 │    T0    │ 
┌└──────────┘┐
│  │D│  │D│  │
│U0│0│U1│1│U2│
│  │ │  │ │  │
└┬─┘─└┬┬┘─└─┬┘
 │ M0 ││ M1 │ 
┌┴─┐─┌┴┴┐─┌─┴┐
│  │ │  │ │  │
│L0│D│L1│D│L2│
│  │2│  │3│  │
└┌──────────┐┘
 │    B0    │ 
 └──────────┘

 Colon = C
 Period = P
 
  0b C P T0 U0   D0 U1 D1 U2   M0 M1 L0 D2   L1 D3 L2 B0
        Actually move to:
  0b P C D0 M0   L1 U0 D3 L0   D1 B0 U1 L2   D0 U2 M1 T0


T0 - top horizontal segment
U0 - left upper vertical segment
D0 - left upper diagonal segment
U1 - middle upper vertical segment
D1 - right upper diagonal segment
U2 - right upper vertical segment
M0 - left middle horizontal segment
M1 - right middle horizontal segment
L0 - left lower vertical segment
D2 - left lower diagonal segment
L1 - middle lower vertical segment
D3 - right lower diagonal segment
L2 - right lower vertical segment
B0 - bottom horizontal segment



character
 ^ | bits
   0b C P T0 U0   D0 U1 D1 U2   M0 M1 L0 D2   L1 D3 L2 B0
        Actually move to:
   0b P C D2 M0   L1 U0 D3 L0   D1 B0 U1 L2   D0 U2 M1 T0
─────────────────────────
before
 1 | 0000 0001 0000 0010
 2 | 0010 0001 1110 0011
 3 | 0010 0001 1100 0011
 4 | 0001 0001 1100 0010
 A | 0011 0001 1110 0010
 B | 0010 0101 1100 1011
 C | 0011 0000 0010 0001
 D | 0010 0101 0000 1011

 after
 1 | 0000 0000 0001 0100
 2 | 0001 0001 0101 0111
 3 | 0001 0000 0101 0111
 4 | 0001 0100 0001 0110
 A | 0001 0101 0001 0111
 B | 0001 1000 0111 0111
 C | 0000 0101 0100 0001
 D | 0000 1000 0111 0101

IF THIS WAS COMPLETE, IT SHOULD INDEX S.T. THE INDEX OF A CHAR IS 
THE SAME AS IT'S ASCII VALUE, EX charReprArr[41] = rep for "A"
*/
uint16_t charReprArr[10] = {
        0b0000000000010100, // 1
        0b0001000101000111, // 2
        0b0001000001010111, // 3
        0b0001010000010110, // 4
        0b0001010100010111, // A
        0b0001100001110111, // B
        0b0000010101000001, // C
        0b0000100001110101, // D
        0b0001110000000110, // NOT E, actually Y
        0b0000000000000000  // space
};

int main(void)
{       

        printk("Hello! Entered main.\n");


        k_msleep(500);

        if (!device_is_ready(i2c0_dev)) {
                printk("I2C_0 device not ready :(\n");
                return 0;
        } else {
                printk("I2C_0 device is ready!\n");
        }

        uint8_t data[4] = {0,0,0,0};

        snprintk(data, sizeof(data), "ABCD");
        auxdisplay_write(i2c0_dev, data, strlen(data));
        k_msleep(3000);
        auxdisplay_clear(i2c0_dev);
        
}
