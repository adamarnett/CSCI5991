#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/drivers/auxdisplay.h>
#include <string.h>
#include <zephyr/drivers/uart.h>

//#include "uartRx.h"
#include "fram.h"


// not enough functions to justify a whole file for this peripheral - yet!
const struct device *const serLcd_dev = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(i2c0), serlcd_72));
uint8_t empty[] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
        0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
uint8_t welcome[] = {0x50,0x6F,0x77,0x65,0x72,0x69,0x6E,0x67,0x20,0x6F,0x6E,0x2E,0x2E,0x2E};
char status[14];


///////////////////////////////////////////////////////////////////////////////////////////////////////////// uart0 device, pins p0.05-8 by default on nRF52840
static const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static const struct uart_config uart0_cfg = {
        .baudrate = 9600,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

// buffer for storing received uart data
static uint8_t uart0_rxBuf[48];

// buffer for use of data by anything other than the callback function
volatile static char uart0_buf[48];
// callback (ISR) for uart0
void uart0_cb(const struct device *dev, struct uart_event *event, void *data) {

        switch (event->type) {
                // UART_RX_RDY --> data was received and...
                //      receive timeout occured or
                //      buffer is full
                case UART_RX_RDY:
                        //printk("RXed data\n");
                        //printk("isrrx[%s]\n",uart0_rxBuf);
                        // clear main loop buffer
                        memset(uart0_buf, 0, 48*sizeof(char));
                        //printk("isrbb[%s]\n",uart0_buf);
                        // copy received data from rx buffer to main loop buffer
                        strncpy(uart0_buf, uart0_rxBuf, 48*sizeof(uint8_t));
                        printk("isr2bb[%s]\n",uart0_buf);
                        // clear rx buffer
                        memset(uart0_rxBuf, 0, 48*sizeof(uint8_t));
                        //printk("isrrx2[%s]\n",uart0_rxBuf);
                        
                        break;
    
                case UART_RX_DISABLED:
                        printk("Reenabling...\n");
                        int err = uart_rx_enable(uart0_dev, uart0_rxBuf, sizeof(uart0_rxBuf), SYS_FOREVER_MS);
                        if (err == -ENOSYS) {
                                printk(" UART RX NOT ENABLED - ENOSYS \n");
                                // flash led labeled LED2 and LED3 on board endlessly if error
                                //errorLed(4);
                        } else if (err == -ENOTSUP) {
                                printk(" UART RX NOT ENABLED - ENOTSUP \n");
                                // flash led labeled LED3 and LED4 on board endlessly if error
                                //errorLed(5);
                        }
    
                        break;
                default:
                        break;
                
                
        }
    }
    
    // init function for uart, returns non zero on error
    int uart0_init() {
    
        if (!device_is_ready(uart0_dev)) {
            printk("UART0 DEVICE NOT READY\n");
            return 1;
        }
    
        // setup uart
        // uart0_dev is the uart device pointer
        // &uart0_cfg is the configuration struct to apply to the uart0_dev device
        if (uart_configure(uart0_dev, &uart0_cfg) == -ENOSYS) {
            printk(" UART NOT CONFIGURED PROPERLY \n");
            return 1;
        }
    
        // setup uart callback
        // uart0_dev is the uart device pointer
        // uart0_cb is the callback function
        // NULL is where you could put a void pointer to "user data" that can be
        // passed to the callback function
        int err = uart_callback_set(uart0_dev, uart0_cb, NULL);
        if (err == -ENOSYS) {
                printk(" UART CALLBACK NOT SET - ENOSYS \n");
                return 1;
        } else if (err == -ENOTSUP) {
                printk(" UART CALLBACK NOT SET - ENOTSUP \n");
                return 1;
        }
    
        // enable the callback by passing it the rx buffer defined near the top
        // uart0_dev is the uart device pointer
        // uart0_rxBuf is the buffer for receiving data
        // third arg is the size of the buffer
        // fourth arg is the timeout, microseconds delay after receiving one byte of data
        // in the buffer before triggering the UART_RX_RDY event
        err = uart_rx_enable(uart0_dev, uart0_rxBuf, sizeof(uart0_rxBuf), SYS_FOREVER_MS);
        if (err == -ENOSYS) {
                printk(" UART RX NOT ENABLED - ENOSYS \n");
                return 1;
        } else if (err == -ENOTSUP) {
                printk(" UART RX NOT ENABLED - ENOTSUP \n");
                return 1;
        }
    
        return 0;
    }
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void readToSerial() {}




int main(void)
{

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

        k_msleep(2222);

        //auxdisplay_clear(serLcd_dev);

        printk("hi\n");

        volatile struct uartData rx;
        rx.cps = 0;
        rx.cpm = 0;
        rx.usph = 0;
        rx.speed = 0;
        volatile uint8_t auxBuf[48];
        uint8_t decimal;

        while (1) {
                //printk("hi\n");


                memset(auxBuf, 0, sizeof(auxBuf));
                printk("ONE -- Auxbuf is '%s'\n", auxBuf);
                printk("ONE -- uarbuf is '%s'\n", uart0_buf);
                k_msleep(30);


                sscanf(
                        uart0_buf,
                        "CPS, %d, CPM, %d, uSv/hr, %d.%d, %c",
                        &rx.cps,
                        &rx.cpm,
                        &rx.usph,
                        &decimal,
                        &rx.speed
                );

                printk("TWO -- Auxbuf is '%s'\n", auxBuf);
                printk("TWO -- uarbuf is '%s'\n", uart0_buf);
                k_msleep(30);


                snprintf(
                        auxBuf,
                        sizeof(auxBuf),
                        "CPS:%02d CPM:%04d uSv/hr: %02d.%02d    ",
                        rx.cps,
                        rx.cpm,
                        rx.usph,
                        decimal
                );

                printk("THR -- Auxbuf is '%s'\n", auxBuf);
                printk("THR -- uarbuf is '%s'\n", uart0_buf);
                k_msleep(30);


                // display msg
                printk("Attempting write of '%s'\n", auxBuf);
                auxdisplay_clear(serLcd_dev);
                auxdisplay_write(
                        serLcd_dev,
                        auxBuf,
                        32
                );

                printk("FOU -- Auxbuf is '%s'\n", auxBuf);
                printk("FOU -- uarbuf is '%s'\n", uart0_buf);


                k_msleep(30);



        }




        return 0;
}
