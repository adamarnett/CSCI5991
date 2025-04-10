#ifndef UARTRX_H
#define UARTRX_H

#include <zephyr/sys/printk.h>
#include <string.h>

#include <zephyr/drivers/uart.h>

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
static uint8_t uart0_rxBuf[48];

// buffer for use of data by anything other than the callback function
static char uart0_buf[48];

// flag for use by main
enum uartUpdate {
    UART_UPDATE,
    UART_NO_UPDATE
};
static enum uartUpdate uartNewMsg = UART_NO_UPDATE;

// callback (ISR) for uart0
void uart0_cb(const struct device *dev, struct uart_event *event, void *data);

// init for uart0 (to make things easier in main function)
int uart0_init();



#endif