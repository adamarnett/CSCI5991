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
    int shutdown = 0;
    struct sensor_value newVal;
    
    //newVal.val1 = -30;
    //newVal.val2 = -30;
    //LOG_INF("Setting lower thresh...");
    //err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH, &newVal);
    //if (err) LOG_INF("ATTR SET err = [%d]", err);

    //newVal.val1 = 30;
    //newVal.val2 = 30;
    //LOG_INF("Setting upper thresh...");
    //err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH, &newVal);
    //if (err) LOG_INF("ATTR SET err = [%d]", err);

    while (1) {

        int x = 5;
        while (x > 0) {
            err = sensor_sample_fetch(i2c1_therm);
            if (err) LOG_INF("SAMPLE FETCH err = [%d]", err);

            err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            if (err) LOG_INF("SAMPLE GET err = [%d]", err);
            LOG_INF("Got value [%d.%d] from mcp9808 w/ 0.0625 resolution", temp.val1, temp.val2);


            k_msleep(3001);

            x--;
        }
        
        //newVal.val1 = 6;
        newVal.val1 = 0;
        //newVal.val2 = 0;
        newVal.val2 = 500000;
        err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_RESOLUTION, &newVal);
        if (err) LOG_INF("ATTR SET err = [%d]", err);


        int y = 5;
        while (y > 0) {
            err = sensor_sample_fetch(i2c1_therm);
            if (err) LOG_INF("SAMPLE FETCH err = [%d]", err);

            err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            if (err) LOG_INF("SAMPLE GET err = [%d]", err);
            LOG_INF("Got value [%d.%d] from mcp9808 w/ 0.5 resolution", temp.val1, temp.val2);
            k_msleep(3001);

            y--;
        }

        newVal.val1 = 0;
        //newVal.val2 = 0;
        newVal.val2 = 62500;
        err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_RESOLUTION, &newVal);
        if (err) LOG_INF("ATTR SET err = [%d]", err);

        k_msleep(9001);

        if (!shutdown) {
            LOG_INF("SHUTTING DOWN MCP9808");
            shutdown = 1;
            newVal.val1 = 1;
            newVal.val2 = 1;
            err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_CONFIGURATION, &newVal);
            if (err) LOG_INF("ATTR SET err = [%d]", err);
        } else {
            LOG_INF("UN-SHUTTING DOWN MCP9808");
            shutdown = 0;
            newVal.val1 = 0;
            newVal.val2 = 0;
            err = sensor_attr_set(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_CONFIGURATION, &newVal);
            if (err) LOG_INF("ATTR SET err = [%d]", err);
        }

    }

    return 0;
}
