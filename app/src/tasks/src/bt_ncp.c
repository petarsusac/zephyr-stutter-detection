#include "bt_ncp.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "uart.h"

LOG_MODULE_REGISTER(bt_ncp, LOG_LEVEL_DBG);

static K_SEM_DEFINE(conn_wait_sem, 0, 1);

static bool bt_connected;

static void uart_rx_cb(const uint8_t *p_data, size_t len);

int bt_ncp_init()
{
    int err;

    bt_connected = false;

    err = uart_register_rx_cb(uart_rx_cb);
	
    return err;
}

int bt_ncp_wait_for_connection(uint32_t timeout_ms)
{
    int ret;

    ret = uart_send_cmd(UART_CMD_START);

    if (0 == ret)
    {
        ret = k_sem_take(&conn_wait_sem, K_MSEC(timeout_ms));

        if (0 == ret)
        {
            bt_connected = true;
        }
        else
        {
            LOG_ERR("Timed out waiting for BT connection");
        }
    }

    return ret;
}

static void uart_rx_cb(const uint8_t *p_data, size_t len)
{
    if (!p_data)
    {
        return;
    }

    uart_cmd_t cmd = (uart_cmd_t) p_data[0];

    switch (cmd)
    {
        case UART_CMD_CONN_OK:
            k_sem_give(&conn_wait_sem);
        break;

        case UART_CMD_DATA:
            LOG_DBG("Received data of length %u", len-1);
        break;

        default:
            // Ignore other commands
        break;
	}
}

