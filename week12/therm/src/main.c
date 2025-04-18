#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/auxdisplay.h>
#include <stdio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/drivers/sensor/sht4x.h>

//const static struct device *i2c0_disp = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), ac780s_3c));
//const static struct device *i2c0_disp = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), serlcd_72));
const static struct device *i2c1_therm = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c1), sht4x_44));

// define custom service UUID
#define BT_UUID_CUSTOM_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x90019001, 0x9001, 0x9001, 0x9001, 0x900190019001)

// init struct to hold custom uuid in little endian format
static const struct bt_uuid_128 custom_uuid = BT_UUID_INIT_128(
	BT_UUID_CUSTOM_SERVICE_VAL);

static const struct bt_uuid_128 temp_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x90019001, 0xFFFF, 0xFFFF, 0xFFFF, 0x900190019001));

static const struct bt_uuid_128 hum_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x90019001, 0x0000, 0x0000, 0x0000, 0x900190019001));

// values for each thing to transmit
static uint8_t temp_value[1] = {66};
static uint8_t hum_value[1] = {77};

// function to read temp and hum values over ble
static ssize_t read_temp(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
    void *buf, uint16_t len, uint16_t offset) {
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, temp_value, 1);
}

static ssize_t read_hum(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
    void *buf, uint16_t len, uint16_t offset) {
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, hum_value, 1);
}

// declare a custom service for temp and hum
BT_GATT_SERVICE_DEFINE(temp_svc,
	BT_GATT_PRIMARY_SERVICE(&custom_uuid),      // say it's the primary service of the device
	BT_GATT_CHARACTERISTIC(
        &temp_uuid.uuid,            // give it a characteristic
		BT_GATT_CHRC_READ,              // with read property
		BT_GATT_PERM_READ,              // with attribute read permission
		read_temp,                      // read callback function
        NULL,                           // write callback function
        temp_value                      // value for the characteristic
    ),
    BT_GATT_CHARACTERISTIC(
        &hum_uuid.uuid,
        BT_GATT_CHRC_READ,
        BT_GATT_PERM_READ,
        read_hum,
        NULL,
        hum_value
    )
);

// data packet struct for advertising
// not all attributes need to be listed here to appear in the attribute table
// removing things from here will remove avertised characteristics
// need to figure out what the data flags, ad general, no bredr line does...
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static void bt_ready() {

    int err = 0;

    printk("BLE init'ed\n");

    err = bt_le_adv_start(
        BT_LE_ADV_CONN_FAST_1,
        ad,
        ARRAY_SIZE(ad),
        NULL,
        0
    );

    if (err) {
        printk("Failed to start: err = [%d]\n", err);
        return;
    }

    printk("Success I think!\n");
}

int getMeasurement(struct sensor_value* temp, struct sensor_value* hum) {
    int err = 0;

    if (sensor_sample_fetch(i2c1_therm)) {
        printf("sample fetch failed :(\n");
        return 0;
    }

    err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_AMBIENT_TEMP, temp);
    if (err) {
        printk("Err [%d] in sht41\n", err);
    }
    err = sensor_channel_get(i2c1_therm, SENSOR_CHAN_HUMIDITY, hum);
    if (err) {
        printk("Err [%d] in sht41\n", err);
    }

    printf("SHT4X: %02d Temp. [C] ; %02d RH [%%]\n",
        temp->val1,
        hum->val1
    );

    return err;
}

int main(void) {

    //if (!device_is_ready(i2c0_disp)) {
	//	printf("i2c1 not ready :(\n");
	//	return 0;
	//}
    if (!device_is_ready(i2c1_therm)) {
		printf("i2c1 not ready :(\n");
		return 0;
	}

    struct sensor_value temp, hum;

    int err = 0;

    err = getMeasurement(&temp, &hum);
    if (err) {
        printk("Error getting measurement: err = [%d]\n", err);
        return -1;
    }

    err = bt_enable(NULL);
    if (err) {
        printk("BLE init failed :( err = [%d]\n", err);
    }

    bt_ready();

    while (1) {
        
        err = getMeasurement(&temp, &hum);
        if (err) {
            printk("Error getting measurement: err = [%d]\n", err);
            return -1;
        }

        temp_value[0] = temp.val1;
        hum_value[0] = hum.val1;


        k_msleep(3333);
    }


    return 0;
}
