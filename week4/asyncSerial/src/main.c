#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#include "ledControl.c"


// uart0 device, pins p0.05-8 by default on nRF52840
static const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static const struct uart_config uart0_cfg = {
        .baudrate = 9600,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

// buffer for storing received uart data
static uint8_t uart0_rxBuf[48] = {0};

// another buffer
static uint8_t broadcastBuf[48] = {0};


/*
 * 
 * Geiger counter async serial out      nRF52840 default uart0
 *      <"grn" text on pcb>
 *                      RTS#  ------------->    p0.05 RTS
 *                      RXD   ------------->    p0.08 RX
 *                      TXD   ------------->    p0.06 TX
 *                      VCC   ---x
 *                      CTS#  ------------->    p0.07 CTS
 *                      GND   ------------->    GND
 *      <"blk" text on pcb>
 * 
 * Data should arrive about once per second from the geiger counter (956ms according
 * to the saleae), and should look like:
 *      Normal:
 *      CPS, 1, CPM, 20, uSv/hr, 0.11, SLOW<\r><\n>
 *      Radium in glass ampule:
 *      CPS, 15, CPM, 175, uSv/hr, 0.99, SLOW<\r><\n>
 *      Uranium Ore:
 *      CPS, 110, CPM, 6852, uSv/hr, 39.05, FAST<\r><\n>
 * 
 * 
*/


// callback (ISR) for uart0
// dev is a pointer to the uart device (uart0_dev)
// event is a pointer to the event coming from the uart device (data about what happened)
// data option is data passed to the function from somewhere else... not sure where that is though...
static void uart0_cb(const struct device *dev, struct uart_event *event, void *data) {

        switch (event->type) {
                // UART_RX_RDY --> data was received and...
                //      receive timeout occured or
                //      buffer is full
                case UART_RX_RDY:
                        // copy received data from rx buffer
                        strncpy(broadcastBuf, uart0_rxBuf, 48*sizeof(uint8_t));
                        // clear buffer
                        memset(uart0_rxBuf, 0, 48*sizeof(uint8_t));
                        
                        break;
                default:
                        
                        break;
                
                
        }
}


int main(void)
{
        // setup leds and turn them all off
        // returns 0 on error, but what do I do with that info? Flash an
        // led to signal an error? I wouldn't be able to...
        setupLeds();


        // check if uart is ready to go
        if (!device_is_ready(uart0_dev)) {
                // flash led labeled LED3 on board endlessly if error
                errorLed(1);
        }


        // setup uart
        // uart0_dev is the uart device pointer
        // &uart0_cfg is the configuration struct to apply to the uart0_dev device
        if (uart_configure(uart0_dev, &uart0_cfg) == -ENOSYS) {
                // flash led labeled LED2 on board endlessly if error
                errorLed(1);
        }


        // setup uart callback
        // uart0_dev is the uart device pointer
        // uart0_cb is the callback function
        // NULL is where you could put a void pointer to "user data" that can be
        // passed to the callback function
        int err = uart_callback_set(uart0_dev, uart0_cb, NULL);
        if (err == -ENOSYS) {
                // flash led labeled LED3 on board endlessly if error
                errorLed(2);
        } else if (err == -ENOTSUP) {
                // flash led labeled LED4 on board endlessly if error
                errorLed(3);
        }


        // enable the callback by passing it the rx buffer defined near the top
        // uart0_dev is the uart device pointer
        // uart0_rxBuf is the buffer for receiving data
        // third arg is the size of the buffer
        // fourth arg is the timeout, microseconds delay after receiving one byte of data
        // in the buffer before triggering the UART_RX_RDY event
        err = uart_rx_enable(uart0_dev, uart0_rxBuf, sizeof(uart0_rxBuf), 950);
        if (err == -ENOSYS) {
                // flash led labeled LED2 and LED3 on board endlessly if error
                errorLed(4);
        } else if (err == -ENOTSUP) {
                // flash led labeled LED3 and LED4 on board endlessly if error
                errorLed(5);
        }


        while (1) {
                for (int i = 0; i < 48; i++) {
                        if ((char)broadcastBuf[i] == ',') {
                                ledOn(0);
                        } else {
                                ledOff(0);
                        }
                        k_msleep(33);
                        ledOff(0);
                        k_msleep(33);

                        if (i%24 == 0) {
                                ledOn(3);
                        } else if (i%11+1 == 1) {
                                ledOff(3);
                        }
                        
                }
        }

        return 0;
}
