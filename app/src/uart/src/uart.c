#include "uart.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart, LOG_LEVEL_DBG);

static const struct device *const p_uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart3));

static void char_rx_cb(const struct device *dev, void *user_data);

static uart_rx_cb_t p_user_cb;
static uint8_t rx_buf[UART_RX_BUF_LEN];
static size_t rx_buf_idx;

int uart_register_rx_cb(uart_rx_cb_t p_cb)
{
    int err;

    p_user_cb = p_cb;
    rx_buf_idx = 0;

    if (!device_is_ready(p_uart_dev))
    {
        LOG_ERR("UART dev error");
        return -1;
    }

    err = uart_irq_callback_user_data_set(p_uart_dev, char_rx_cb, NULL);
    if (err < 0)
    {
        LOG_ERR("UART IRQ cb register error %d", err);
    }

    uart_irq_rx_enable(p_uart_dev);

    return err;
}

int uart_send_cmd(uart_cmd_t cmd)
{
    uart_poll_out(p_uart_dev, (uint8_t) cmd);
    uart_poll_out(p_uart_dev, UART_STOP_BYTE);

    return 0;
}

int uart_send_data(const uint8_t *p_data, size_t len)
{
    uart_poll_out(p_uart_dev, UART_CMD_DATA);

    for (size_t i = 0; i < len; i++)
    {
        uart_poll_out(p_uart_dev, p_data[i]);
    }

    uart_poll_out(p_uart_dev, UART_STOP_BYTE);

    return 0;
}

static void char_rx_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(dev)) {
		return;
	}

    if (!uart_irq_rx_ready(dev)) {
		return;
	}

    // Read UART FIFO byte by byte
    while (1 == uart_fifo_read(dev, &c, 1))
    {
        // If the end of message is received, call the user cb and clear the
        // buffer
        if (UART_STOP_BYTE == c)
        {
            if (p_user_cb)
            {
                p_user_cb(rx_buf, rx_buf_idx);
            }

            rx_buf_idx = 0;
        }
        else if (rx_buf_idx < UART_RX_BUF_LEN)
        {
            rx_buf[rx_buf_idx++] = c;
        }
        else
        {
            LOG_WRN("UART char dropped");
        }
	}
}
