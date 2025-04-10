#include <zephyr/kernel.h>

#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/sys/printk.h>


const struct device *const dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), serlcd_72));

int main(void)
{
        printk("HELLO WORLD!!! :D\n");
        int rc;
	
	uint8_t data[64];

	if (!device_is_ready(dev)) {
		//LOG_ERR("Auxdisplay device is not ready.");
                printk("AUX NOT READY TO BE PASSED :(\n");
		return 0;
	}

	rc = auxdisplay_cursor_set_enabled(dev, true);

	if (rc != 0) {
		//LOG_ERR("Failed to enable cursor: %d", rc);
                printk("CURSOR NOT ENABLED :(\n");
	}

	snprintk(data, sizeof(data), "Hello world from %s", CONFIG_BOARD);

        while (1) {
                rc = auxdisplay_write(dev, data, strlen(data));

	        if (rc != 0) {
		        //LOG_ERR("Failed to write data: %d", rc);
                        printk("FAILED TO WRITE :(\n");
	        }

                k_msleep(1000);
        }
	
	return 0;
}
