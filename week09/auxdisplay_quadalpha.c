
#define DT_DRV_COMPAT sparkfun_quadalpha

#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_quadalpha, CONFIG_AUXDISPLAY_LOG_LEVEL);

/*
 * system startup command is 
 * 0 0 1 0 X X X S 
 * where S is 1 for turn on system oscillator and 0 is for turn off
*/
#define QUADALPHA_SYS_ON 0x21
//const uint8_t CMD_SYS_ON[] = {0b00100001};


/*
 * dimming set command is 
 * 1 1 1 0 P3 P2 P1 P0
 * where P3, P2, P1, and P0 represent the duty cycle in binary with
 * P3 being the most significant bit and P0 being the least
 * duty cycle = (1+0bP3P2P1P0)/16, so if all are zero duty cycle is
 * 1/16 (min brightness) and if all are 1 it's 16/16 (max brightness)
*/
#define QUADALPHA_BRIGHT_MAX 0xEF
//const uint8_t CMD_BRIGHT_MAX[] = {0b11101111};

/*
 * display setup/blinking set command is
 * 1 0 0 0 X B1 B0 D
 * where B1 and B0 are the blinking frequency in binary and D is 
 * display on or off (0 for off, 1 for on) if B1 and B0 are both
 * zero, there is no blinking, if both are 1 the display will blink
 * at a rate of 2Hz
 * this command is neccessary to turn on the display (D = 1) 
*/
#define QUADALPHA_DISP_ON_NO_BLINK 0x81
//const uint8_t CMD_DISP_ON_NO_BLINK[] = {0b10000001};

/*
 * none of these really supported lol 
*/
# define QUADALPHA_CUSTOM_CHAR_MAX_COUNT 0
# define QUADALPHA_CUSTOM_CHAR_WIDTH 0
# define QUADALPHA_CUSTOM_CHAR_HEIGHT 0

// 
struct auxdisplay_quadalpha_data {
	bool power;
};

struct auxdisplay_quadalpha_config {
    struct auxdisplay_capabilities capabilities;
	struct i2c_dt_spec bus;
	uint16_t command_delay_ms;
	uint16_t special_command_delay_ms;
};


static int auxdisplay_quadalpha_display_on(const struct device *dev) {

    const struct auxdisplay_quadalpha_config *config = dev->config;

    int err = 0;

    // write to turn on brighness (required before display on command)
    err = i2c_write_dt(
        &config->bus,
        QUADALPHA_BRIGHT_MAX,
        sizeof(QUADALPHA_BRIGHT_MAX)
    );

    if (err) {
        return err;
    }

    // turn on display, set blinking rate to zero
    err = i2c_write_dt(
        &config->bus,
        QUADALPHA_DISP_ON_NO_BLINK,
        sizeof(QUADALPHA_DISP_ON_NO_BLINK)
    );

    if (err) {
        return err;
    }

    return 0;
}

static int auxdisplay_quadalpha_write(const struct device *dev, const uint8_t *text, uint16_t len) {
    
    // this should be moved to an enum
    // representations of all characters that can (currently) be displayed
    uint16_t charReprArr[10] = {
        0b0000000000010100, // 1
        0b0001000101000111, // 2
        0b0001000001010111, // 3
        0b0001010000010110, // 4
        0b0001010100010111, // A
        0b0001100001110111, // B
        0b0000010101000001, // C
        0b0000100001110101, // D
        0b0001110000000110, // NOT E, actually Y
        0b0000000000000000  // space
    };
    
    // get config from device so we can access the i2c bus in i2c_write_dt
    const struct auxdisplay_quadalpha_config *config = dev->config;

    // error checking, don't want to try to write to segments that
    // don't exist
    if (len > 4) {
        // return invalid argument error
        return -EINVAL;
    }

    // buffer to hold write address [0] and data about display [1-13]
    uint8_t writeBuf[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // int for error checking
    int err = 0;

    // read data from display RAM, do this so we can preserve
    // what's on any digits we don't write to
    for (int i = 0; i < 13; i++) {
            err = i2c_write_read_dt(
                    &config->bus,
                    writeBuf,
                    sizeof(uint8_t),
                    writeBuf + i + 1,
                    sizeof(uint8_t)
            );
            if (err) {
                return err;
            }
            writeBuf[0] += 1;
    }
    // reset write address
    writeBuf[0] = 0;

    // overwrite what's currently on display with what we 
    // want to write
    uint16_t charRep = 0;
    for (int i = 0; i < len; i++) {
            // this is what should be here
            // incoming data should be ascii values
            //uint16_t charRep = charReprArr[text[i]];

            // incomplete version because I don't want to make 
            // representations of all the different characters it 
            // takes foreverrrrr
            // so 1-4, A-D, and Y (as E) are supported
            // Y is supported as E so I could print YAY without
            // adding another if
            if (text[i] >= 65 && text[i] <= 69) {
                    charRep = charReprArr[text[i]-61];
            }
            else if (text[i] >= 49 && text[i] <= 52) {
                    charRep = charReprArr[text[i]-49];
            }
            for (int j = 0; j < 14; j++) {

                    // for bits 0-13
                    // RAM byte = (uint16_t bit) - (uint16_t bit)%2
                    // RAM bit = (((uint16_t bit)%2) * 4) + digit
                    // also note that writeBuf is not 0 indexed because the 
                    // starting write address must come before the data

                    // also also, clear any bit that may or may not be 
                    // set, don't want residual segments illuminated
                    // from last write to display
                    
                    // this clears last segments
                    writeBuf[(j - (j%2)) + 1] &= ~((0x01) << (((j%2) * 4) + i));
                    
                    // this illuminates new segments if necessary
                    if ((charRep & 0x01)) {
                            writeBuf[(j - (j%2)) + 1] |= ((0x01) << (((j%2) * 4) + i));
                    }
                    
                    // move to the next bit in the representation
                    charRep = charRep >> 1;
            }
            // reset the representation
            charRep = 0;
    }
    
    // re-write the RAM
    err = i2c_write_dt(
            &config->bus,
            writeBuf,
            sizeof(writeBuf)
    );
    
    // will be 0 if write was successful
    return err;
}

static int auxdisplay_quadalpha_clear(const struct device *dev) {

    // write 4 spaces to the display
    int err = auxdisplay_quadalpha_write(
        dev,
        "    ",
        4
    );

    return err;
}

static int auxdisplay_quadalpha_init(const struct device *dev) {
    
    // get config from dt device to access i2c bus
    const struct auxdisplay_quadalpha_config *config = dev->config;
    // get data to set power variable to true
    struct auxdisplay_quadalpha_data *data = dev->data;
    data->power = true;
    
    // is the i2c_spec in devicetree ready?
    if (!device_is_ready(config->bus.bus)) {
        // if not return no such device error
        return -ENODEV;
    }

    int err = 0;

    // write command to turn on system oscillator
    err = i2c_write_dt(
        &config->bus,
        QUADALPHA_SYS_ON,
        sizeof(QUADALPHA_SYS_ON)
    );
    
    // if can't initialize, likely due to some kind of i2c
    // communication error
    if (err) {
        // return IO error
        return -EIO;
    }

    return 0;
}

// A bunch of funcitons that I either can't or haven't bothered to implement. They seem
// to need to be defined to allow the device api struct to be aligned correctly in memory
// however, so here they are. They don't match some function signatures for the generic API,
// so they generate some compiler warnings.

static int auxdisplay_quadalpha_display_off(const struct device *dev) {

    return 0;
}

static int auxdisplay_quadalpha_cursor_set_enabled(const struct device *dev) {

    return 0;
}

static int auxdisplay_quadalpha_position_blinking_set_enabled(const struct device *dev) {

    return 0;
}

static int auxdisplay_quadalpha_cursor_position_set(const struct device *dev) {

    return 0;
}

static int auxdisplay_quadalpha_cursor_position_get(const struct device *dev) {

    return 0;
}

static int auxdisplay_quadalpha_capabilities_get(const struct device *dev) {

    return 0;
}

static int auxdisplay_quadalpha_custom_character_set(const struct device *dev) {

    return 0;
}

// init the quadalpha implementation of the auxdisplay generic API. might have
// used incorrect terminology there, but I know what it's doing in my head.
static DEVICE_API(auxdisplay, auxdisplay_quadalpha_auxdisplay_api) = {
    .display_on = auxdisplay_quadalpha_display_on,
    .display_off = auxdisplay_quadalpha_display_off,
    .cursor_set_enabled = auxdisplay_quadalpha_cursor_set_enabled,
    .position_blinking_set_enabled = auxdisplay_quadalpha_position_blinking_set_enabled,
    .cursor_position_set = auxdisplay_quadalpha_cursor_position_set,
    .cursor_position_get = auxdisplay_quadalpha_cursor_position_get,
    .capabilities_get = auxdisplay_quadalpha_capabilities_get,
    .clear = auxdisplay_quadalpha_clear,
    .custom_character_set = auxdisplay_quadalpha_custom_character_set,
    .write = auxdisplay_quadalpha_write,
};

#define AUXDISPLAY_QUADALPHA_INST(inst)                                                               \
	static const struct auxdisplay_quadalpha_config auxdisplay_quadalpha_config_##inst = {           \
		.capabilities = {                                                                  \
				.columns = DT_INST_PROP(inst, columns),                            \
				.rows = DT_INST_PROP(inst, rows),                                  \
				.mode = 0,                                                         \
				.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.custom_characters = QUADALPHA_CUSTOM_CHAR_MAX_COUNT,                 \
				.custom_character_width = QUADALPHA_CUSTOM_CHAR_WIDTH,                \
				.custom_character_height = QUADALPHA_CUSTOM_CHAR_HEIGHT,              \
			},                                                                         \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.command_delay_ms = DT_INST_PROP(inst, command_delay_ms),                          \
		.special_command_delay_ms = DT_INST_PROP(inst, special_command_delay_ms),          \
	};                                                                                         \
                                                                                                   \
	static struct auxdisplay_quadalpha_data auxdisplay_quadalpha_data_##inst;                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &auxdisplay_quadalpha_init, NULL, &auxdisplay_quadalpha_data_##inst, \
			      &auxdisplay_quadalpha_config_##inst, POST_KERNEL,                       \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_quadalpha_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_QUADALPHA_INST)
