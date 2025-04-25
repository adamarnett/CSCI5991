#include <zephyr/fff.h>
#include <zephyr/ztest.h>
//#include <zephyr/device.h>
#include "i2c_wrapper.h"
//#include <zephyr/drivers/auxdisplay.h>

DEFINE_FFF_GLOBALS;

// Fake i2c_write
FAKE_VALUE_FUNC(int, i2c_write_dt_wrapper,
    void *, const uint8_t *, uint32_t);

// Example function under test
int my_sensor_send(void *spec, const uint8_t *data, uint32_t len) {
    printk("Hello world!\n");
    return i2c_write_dt_wrapper(spec, data, len);
}

// setup function
static void setup() {
    RESET_FAKE(i2c_write_dt_wrapper);
}

ZTEST(framework_tests, test_assert)
{
	zassert_equal(1,1, "1!=1");

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};

    //i2c_write_dt_wrapper.return_val = 0;

    int ret = -1;

    ret = my_sensor_send(
        0,
        data,
        sizeof(data)
    );

    zassert_equal(ret, 0, "Expected i2c_write_dt to return 0 :(");
}

ZTEST_SUITE(framework_tests, NULL, NULL, NULL, NULL, NULL);