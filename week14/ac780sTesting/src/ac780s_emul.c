#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(temp_sensor_emul, LOG_LEVEL_DBG);

struct ac780s_emul_data {
	bool power;
	bool cursor;
	bool blinking;
    bool increment;
	uint16_t cursor_x;
	uint16_t cursor_y;
};

struct ac780s_emul_config {
	uint16_t placeholder;
};

static int ac780s_emul_transfer(
    const struct emul *target,
    struct i2c_msg *msgs,
    int num_msgs,
    int addr
) {
    return 0;
}

static int ac780s_emul_init() {
    return 0;
}

static struct i2c_emul_api ac780s_emul_api = {
    .transfer = ac780s_emul_transfer,
};

static struct ac780s_emul_data emul_data = {
    .power = false,
	.cursor = false,
	.blinking = false,
    .increment = false,
	.cursor_x = 0,
	.cursor_y = 0,
};

static const struct ac780s_emul_config emul_config = {
    .placeholder = 0,
};

EMUL_DT_DEFINE(
    DT_CHILD(DT_NODELABEL(i2c0), i2c_100),
    ac780s_emul_init,
    &emul_data,
    &emul_config,
    &ac780s_emul_api,
    NULL
);
