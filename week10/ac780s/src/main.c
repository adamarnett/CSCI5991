#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>


// get the i2c0 device from dt
const static struct device *i2c0_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), ac780s_3c));



int main(void)
{

        if (!device_is_ready(i2c0_dev)) {
                printk("I2C_0 device not ready :(\n");
                return 0;
        } else {
                printk("I2C_0 device is ready!\n");
        }

        k_msleep(500);

        // this is the closest I can get to ¯\_(ツ)_/¯, since there's
        // no preprogrammed backslash in the AC780S
        uint8_t chars[] = {
            0xCD,
            0x5F,
            0x28,
            0xBC,
            0x29,
            0x5F,
            0x2F,
            0x2D
        };

        int err = auxdisplay_cursor_position_set(i2c0_dev,AUXDISPLAY_POSITION_ABSOLUTE, 0, 1);
        auxdisplay_write(i2c0_dev, chars, sizeof(chars));


        printk("err [%d]\n", err);


        //while (1) {
        //    
        //    auxdisplay_write(i2c0_dev, &achar, sizeof(uint8_t));
        //    
        //    if ((achar++) > 176) {
        //        achar = 0;
        //    }
        //    
        //    k_msleep(500);
        //}

        return 0;
}






























/*
#define AC780S_FUNC_SET 0x38        // 0b 0011 1000
#define AC780S_FUNC_SET_DELAY 100   // 100 microseconds
#define AC780S_DISP_ON 0x0C         // 0b 0000 1100
#define AC780S_DISP_ON_DELAY 100    // 100 microseconds
#define AC780S_ENTRY_MODE 0x06      // 0b 0000 0111
#define AC780S_ENTRY_MODE_DELAY 100 // 100 microseconds
#define AC780S_RET_HOME 0x02        // 0b 0000 0010
#define AC780S_CLEAR_DISP 0x01      // 0b 0000 0001

#define AC780S_CTL_WRITE_INST 0x00  // 0b 1100 0000
#define AC780S_CTL_WRITE_DISP 0x40  // 0b 1100 0000


// Y = 0x59 = 0b 0101 1001
// A = 0x41 = 0b 0100 0001
// : = 0x3A = 0b 0011 1010
// ; = 0x3B = 0b 0011 1011
// ) = 0x29 = 0b 0010 1001
//   = 0x10 = 0b 0001 0000

enum font_type {
    FONT_5X8_DOTS,
    FONT_5X11_DOTS
};

enum auxdisplay_ac780s_command {
    AC780S_CLEAR_DISPLAY = 0x01,
    AC780S_RETURN_HOME = 0x02
};

enum auxdisplay_ac780s_special_command {
    AC780S_ENTRY_MODE_SET = 0x04,
    AC780S_DISPLAY_ON_OFF = 0x08,
    AC780S_CURSOR_DISPLAY_SHIFT = 0x10,
    AC780S_FUNCTION_SET = 0x20
};

struct auxdisplay_ac780s_data {
	bool power;
	bool cursor;
	bool blinking;
    enum font_type font;
	uint16_t cursor_x;
	uint16_t cursor_y;
};

struct auxdisplay_ac780s_config {
	struct auxdisplay_capabilities capabilities;
	struct i2c_dt_spec bus;
	uint16_t command_delay_us;
	uint16_t special_command_delay_us;
};





static int auxdisplay_ac780s_send_command(const struct device *dev,
   const enum auxdisplay_ac780s_command command) {

}

static int
auxdisplay_ac780s_send_special_command(const struct device *dev,
				       const enum auxdisplay_ac780s_special_command command) {

}


static int auxdisplay_ac780s_send_display_state(const struct device *dev,
    const struct auxdisplay_ac780s_data *data) {

}

static int auxdisplay_ac780s_display_on(const struct device *dev) {
    uint8_t cmdf[1] = AC780S_DISP_ON;
    i2c_write(
            i2c0_dev,
            cmdf,
            sizeof(cmdf),
            0x3C
    );
    k_msleep(150);

}

static int auxdisplay_ac780s_display_off(const struct device *dev) {

}

static int auxdisplay_ac780s_cursor_set_enabled(const struct device *dev, bool enable) {

}

static int auxdisplay_ac780s_position_blinking_set_enabled(const struct device *dev, bool enable) {

}

static int auxdisplay_ac780s_cursor_position_set(const struct device *dev,
    enum auxdisplay_position type, int16_t x,
    int16_t y) {

}

static int auxdisplay_ac780s_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y) {

}

static int auxdisplay_ac780s_capabilities_get(const struct device *dev,
    struct auxdisplay_capabilities *capabilities) {

    }

static int auxdisplay_ac780s_clear(const struct device *dev) {

}

static int auxdisplay_ac780s_custom_character_set(const struct device *dev,
    struct auxdisplay_character *character) {

}

static void auxdisplay_ac780s_advance_current_position(const struct device *dev) {

}

static int auxdisplay_ac780s_write(const struct device *dev, const uint8_t *text, uint16_t len) {

}

auxdisplay_ac780s_init(const struct device *dev) {

    //const struct auxdisplay_ac780s_config *config = dev->config;

	//if (!device_is_ready(config->bus.bus)) {
	//	return -ENODEV;
	//}

}
*/