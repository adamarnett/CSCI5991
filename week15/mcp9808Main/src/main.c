#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>

const static struct device *i2c1_therm = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c1), mcp9808_18));

int main(void)
{

    if (!device_is_ready(i2c1_therm)) {
		printf("i2c1 not ready :(\n");
		return 0;
	}

    struct sensor_value temp;

    k_msleep(2000);

    int err = sensor_sample_fetch(i2c1_therm);
    if (err) {
        printk("Err [%d] in mcp9808\n", err);
    }

    err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (err) {
        printk("Err [%d] in mcp9808\n", err);
    }

    printk("Got value [%d.%d] from mcp9808\n", temp.val1, temp.val2);

    while (1) {
        err = sensor_sample_fetch(i2c1_therm);
        if (err) {
            printk("Err [%d] in mcp9808\n", err);
        }

        err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        if (err) {
            printk("Err [%d] in mcp9808\n", err);
        }
        printk("Got value [%d.%d] from mcp9808\n", temp.val1, temp.val2);
        k_msleep(9001);
    }

    return 0;
}
