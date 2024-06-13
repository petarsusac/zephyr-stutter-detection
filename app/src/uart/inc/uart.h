#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define UART_MAX_MSG_LEN (8U)

typedef enum uart_cmd
{
    UART_CMD_CONN_OK = 0x01,
    UART_CMD_START = 0x02,
    UART_CMD_DATA = 0x03,
} uart_cmd_t;

typedef void (*uart_rx_cb_t)(const uint8_t *p_data, size_t len);

int uart_register_rx_cb(uart_rx_cb_t p_cb);
void uart_disable(void);
int uart_send_cmd(uart_cmd_t cmd);
int uart_send_data(const uint8_t *p_data, size_t len);

#endif /*_UART_H_ */