

#include "uartRx.h"

// callback (ISR) for uart0
void uart0_cb(const struct device *dev, struct uart_event *event, void *data) {

    switch (event->type) {
            // UART_RX_RDY --> data was received and...
            //      receive timeout occured or
            //      buffer is full
            case UART_RX_RDY:
                    printk("RXed data\n");
                    printk("isrrx[%s]\n",uart0_rxBuf);
                    // clear main loop buffer
                    memset(uart0_buf, 0, 48*sizeof(char));
                    printk("isrbb[%s]\n",uart0_buf);
                    // copy received data from rx buffer to main loop buffer
                    strncpy(uart0_buf, uart0_rxBuf, 48*sizeof(uint8_t));
                    printk("isr2bb[%s]\n",uart0_buf);
                    // clear rx buffer
                    memset(uart0_rxBuf, 0, 48*sizeof(uint8_t));
                    printk("isrrx2[%s]\n",uart0_rxBuf);

                    uartNewMsg = UART_UPDATE;
                    
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