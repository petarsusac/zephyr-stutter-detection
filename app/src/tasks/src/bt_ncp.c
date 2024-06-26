#include "bt_ncp.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/drivers/gpio.h>

#include "uart.h"

#define QUEUE_SIZE (5U)
#define PAYLOAD_LENGTH (7U)
#define STATUS_LED_PERIOD_MS (500U)

LOG_MODULE_REGISTER(bt_ncp, CONFIG_APP_LOG_LEVEL);

static void uart_rx_cb(const uint8_t *p_data, size_t len);
static inline bool parse_msg(bt_ncp_msg_t *msg, const uint8_t *p_data, size_t len);
static void status_led_tmr_cb(struct k_timer *p_tmr);

static K_SEM_DEFINE(conn_wait_sem, 0, 1);

K_MSGQ_DEFINE(msg_queue, sizeof(bt_ncp_msg_t), QUEUE_SIZE, 4);
K_MSGQ_DEFINE(ts_msg_queue, sizeof(bt_ncp_ts_msg_t), QUEUE_SIZE, 4);

static K_TIMER_DEFINE(status_led_tmr, status_led_tmr_cb, NULL);

#ifdef CONFIG_EXTERNAL_RTC
static const struct device *const p_rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc_ext));
#else
static const struct device *const p_rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc));
#endif /* CONFIG_EXTERNAL_RTC */

static struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led), gpios);

static bool bt_connected;

int bt_ncp_init()
{
    int err;

    bt_connected = false;

    err = uart_register_rx_cb(uart_rx_cb);

    if (0 == err)
    {
        err = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT);
    }
	
    return err;
}

int bt_ncp_wait_for_connection(uint32_t timeout_ms)
{
    int ret;

    gpio_pin_set_dt(&status_led, 1);
    k_timer_start(&status_led_tmr, K_MSEC(STATUS_LED_PERIOD_MS), K_MSEC(STATUS_LED_PERIOD_MS));
    
    ret = k_sem_take(&conn_wait_sem, K_MSEC(timeout_ms));

    if (0 == ret)
    {
        bt_connected = true;
    }
    else
    {
        LOG_ERR("Timed out waiting for BT connection");
        uart_disable();
    }

    k_timer_stop(&status_led_tmr);
    gpio_pin_set_dt(&status_led, bt_connected);

    return ret;
}

int bt_ncp_get_timestamped_msg(bt_ncp_ts_msg_t *p_msg)
{
    return k_msgq_get(&ts_msg_queue, p_msg, K_NO_WAIT);
}

void bt_ncp_run(void *p1, void *p2, void *p3)
{
    int err;
    bt_ncp_ts_msg_t ts_msg;
    struct rtc_time rtc_ts;

    for (;;)
    {
        err = k_msgq_get(&msg_queue, &ts_msg.msg, K_FOREVER);

        if (0 == err)
        {
            err = rtc_get_time(p_rtc_dev, &rtc_ts);
            if (0 == err)
            {
                ts_msg.timestamp = timeutil_timegm64(rtc_time_to_tm(&rtc_ts));
            }

            k_msgq_put(&ts_msg_queue, &ts_msg, K_NO_WAIT);
        }
    }
}

static void uart_rx_cb(const uint8_t *p_data, size_t len)
{
    bt_ncp_msg_t msg;

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
            if (parse_msg(&msg, &p_data[1], len-1))
            {
                k_msgq_put(&msg_queue, &msg, K_NO_WAIT);
            }
            else
            {
                LOG_ERR("Unable to parse message payload");
            }
        break;

        default:
            // Ignore other commands
        break;
	}
}

// Parse the received message into bt_ncp_msg_t struct. `p_data` should point to
// the message payload (without the command byte). Return true if parsing is
// successful.
static inline bool parse_msg(bt_ncp_msg_t *msg, const uint8_t *p_data, size_t len)
{
    if (msg && p_data && (PAYLOAD_LENGTH == len))
    {
        msg->hr = p_data[0];
        msg->rmssd = ((uint16_t) p_data[2] << 8) | p_data[1];
        msg->ppg_amplitude = ((uint16_t) p_data[4] << 8) | p_data[3];
        msg->epc = ((uint16_t) p_data[6] << 8) | p_data[5];

        return true;
    }

    return false;
}

static void status_led_tmr_cb(struct k_timer *p_tmr)
{
    gpio_pin_toggle_dt(&status_led);
}

