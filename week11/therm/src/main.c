#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/auxdisplay.h>
#include <stdio.h>

#include <zephyr/drivers/sensor/sht4x.h>

//const static struct device *i2c0_disp = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), ac780s_3c));
const static struct device *i2c0_disp = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), serlcd_72));
const static struct device *i2c1_therm = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c1), sht4x_44));

int main(void) {

    if (!device_is_ready(i2c0_disp)) {
		printf("i2c1 not ready :(\n");
		return 0;
	}
    if (!device_is_ready(i2c1_therm)) {
		printf("i2c1 not ready :(\n");
		return 0;
	}

    struct sensor_value temp, hum;

    int err = 0;

    uint8_t aux_buf[17];
    memset(aux_buf, 0, sizeof(aux_buf));


    while (1) {

        if (sensor_sample_fetch(i2c1_therm)) {
			printf("sample fetch failed :(\n");
			return 0;
		}

        err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        if (err) {
            printk("Err [%d] in sht41\n", err);
        }
		err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_HUMIDITY, &hum);
        if (err) {
            printk("Err [%d] in sht41\n", err);
        }

        printf("SHT4X: %02d Temp. [C] ; %02d RH [%%]\n",
            temp.val1,
            hum.val1
        );

        
        snprintk(
            aux_buf, 
            sizeof(uint8_t)*17, 
            " %02u.%01u  C%02u.%01u %%RH",
            temp.val1, (temp.val2/100000),
            hum.val1, (hum.val2/100000)
        );

        //for (int i = 0; i < 17; i++) {
        //    printk("%c", aux_buf[i]);
        //    if (i == 7) printk("\n");
        //}
        //printk("\n");

        aux_buf[6] = 0xDF; // "°" doesn't translate right
        auxdisplay_clear(i2c0_disp);
        err = auxdisplay_write(i2c0_disp, aux_buf, 16*sizeof(uint8_t));
        if (err) {
            printk("Err [%d] in auxdisplay\n", err);
        }

        memset(aux_buf, 0, sizeof(aux_buf));

        k_msleep(3333);
    }


    return 0;
}



/*

why does this:
snprintk(
    aux_buf, 
    sizeof(uint8_t)*17, 
    " %02u.%01u °C %02u.%01u %%RH",
    temp.val1, (temp.val2/100000),
    hum.val1, (hum.val2/100000)
);
aux_buf[6] = 0xDF;

put "23.9 °- "
    "C 31.4 %"

on the display???
*/