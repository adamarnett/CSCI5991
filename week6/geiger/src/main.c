#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/drivers/auxdisplay.h>

#include "uartRx.h"
#include "fram.h"


// not enough functions to justify a whole file for this peripheral - yet!
const struct device *const serLcd_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), serlcd_72));
uint8_t empty[] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
        0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
uint8_t welcome[] = {0x50,0x6F,0x77,0x65,0x72,0x69,0x6E,0x67,0x20,0x6F,0x6E,0x2E,0x2E,0x2E};
char status[14];

int main(void)
{       
        printk("hello!\n");

        int uartErr = uart0_init();
        if (uartErr) {
                printk("Error initializing UART\n");
                return 0;
        }

        int framErr = fram_init();
        if (framErr) {
                printk("Error initializing FRAM\n");
                return 0;
        }

        if ((!device_is_ready(serLcd_dev)) || auxdisplay_cursor_set_enabled(serLcd_dev, true)) {
                printk("Error initializing display\n");
		return 0;
	}

        printk("hellooo!\n");


        // clear display
        auxdisplay_write(
                serLcd_dev,
                empty,
                sizeof(empty)
        );

        // on message
        auxdisplay_write(
                serLcd_dev,
                welcome,
                sizeof(welcome)
        );


        while (1) {

                // clear display
                auxdisplay_write(
                        serLcd_dev,
                        empty,
                        sizeof(empty)
                );

                // display msg

                auxdisplay_write(
                        serLcd_dev,
                        uart0_buf,
                        10
                );

                k_msleep(777);


                char* data = strstr("uSv/hr, ", uart0_buf);
                
                if (data != NULL) {

                }



        }




        return 0;
}
