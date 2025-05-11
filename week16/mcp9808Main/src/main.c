#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);


const static struct device *i2c1_therm = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c1), mcp9808_18));

int main(void)
{

    if (!device_is_ready(i2c1_therm)) {
		printf("i2c1 not ready :(\n");
		return 0;
	}

    struct sensor_value temp;

    k_msleep(4000);

    int err = 0;

    //err = sensor_sample_fetch(i2c1_therm);
    //if (err) {
    //    //printk"Err [%d] in mcp9808\n", err);
    //}
    //err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    //if (err) {
    //    //printk"Err [%d] in mcp9808\n", err);
    //}

    //printk"Got value [%d.%d] from mcp9808\n", temp.val1, temp.val2);

    while (1) {
        int x = 1;
        while (x > 0) {
            err = sensor_sample_fetch(i2c1_therm);
            LOG_INF("SAMPLE FETCH err = [%d]//////////////////////////////", err);

            err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            LOG_INF("SAMPLE GET err = [%d]//////////////////////////////", err);

            k_msleep(3001);

            x--;
        }


        struct sensor_value newRes;
        newRes.val1 = 6;
        newRes.val2 = 0;
        err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_HYSTERESIS, &newRes);
        LOG_INF("ATTR SET err = [%d]//////////////////////////////", err);


        int y = 1;
        while (y > 0) {
            err = sensor_sample_fetch(i2c1_therm);
            LOG_INF("SAMPLE FETCH err = [%d]//////////////////////////////", err);

            err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            LOG_INF("SAMPLE GET err = [%d]//////////////////////////////", err);
            //printk"Got value [%d.%d] from mcp9808 w/ 0.5 resolution\n", temp.val1, temp.val2);
            k_msleep(3001);

            y--;
        }

        newRes.val1 = 0;
        newRes.val2 = 0;
        err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_HYSTERESIS, &newRes);
        LOG_INF("ATTR SET err = [%d]//////////////////////////////", err);

        break;
    }

    return 0;
}
