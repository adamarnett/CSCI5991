#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>

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
    printk("Calling ac780s_emul_transfer...\n");
    return 0;
}

static int ac780s_emul_init() {
    printk("Calling ac780s_emul_init...\n");

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


// this *should* work just like how the ac780s driver get instantiated
//#define AC780S_EMUL_INST(inst)                                              \
//    static const struct ac780s_emul_config ac780s_emul_config_##inst = {    \
//        .placeholder = 0,                                                   \
//    };                                                                      \
//                                                                            \
//    static struct ac780s_emul_data ac780s_emul_data##inst;                  \
//                                                                            \
//    EMUL_DT_INST_DEFINE(inst, ac780s_emul_init, &emul_data_##inst,          \
//        &emul_config_##inst, &ac780s_emul_api, NULL);
//
//DT_INST_FOREACH_STATUS_OKAY(AC780S_EMUL_INST)
/*
But it doesn't...
adam@ap3561:~/zephyrProjects/ac780sTesting$ ./build/ac780sTesting/zephyr/zephyr.exe
WARNING: Using a test - not safe - entropy source
[00:00:00.000,000] <inf> emul: Registering 1 emulator(s) for i2c@100
[00:00:00.000,000] <wrn> emul: Cannot find emulator for 'i2c@100'           // in fact it's worse than the other way
[00:00:00.000,000] <inf> emul: Registering 0 emulator(s) for i2c@100
*** Booting nRF Connect SDK v2.9.99-bc89c45cf4b2 ***
*** Using Zephyr OS v4.0.99-10eb60b48111 ***
Running TESTSUITE auxdisplay_ac780s_tests
===================================================================
START - test_clear
Hello world!
Hello world! II
Hello world! III
Segmentation fault (core dumped)
*/


#define AC780S_EMUL_INST
EMUL_DT_DEFINE(
    DT_CHILD(DT_NODELABEL(i2c0), i2c_100),
    ac780s_emul_init,
    &emul_data,
    &emul_config,
    &ac780s_emul_api,
    NULL
);


