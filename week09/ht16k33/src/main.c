#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>

// get the quadalpha display on i2c0 device from dt
const struct device *const i2c0_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), quadalpha_70));

int main(void)
{       

        k_msleep(500);
    
        // turn on display, ensure it's ready to go
        if (!device_is_ready(i2c0_dev)) {
                printk("I2C_0 device not ready :(\n");
                return 0;
        } else {
                printk("I2C_0 device is ready!\n");
        }
    
        // buffer for data to send to display
        uint8_t data[4];

        while (1) {
                // put ABCD in buffer and send
                snprintk(data, sizeof(data), "ABCD");
                // write to display
                auxdisplay_write(i2c0_dev, data, 4*sizeof(uint8_t));
                // wait 3 seconds
                k_msleep(3000);
                // clear the display
                auxdisplay_clear(i2c0_dev);
                // wait 3 more seconds
                k_msleep(3000);
        }
        

        
}
