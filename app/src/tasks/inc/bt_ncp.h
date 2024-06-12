#ifndef _BT_NCP_H_
#define _BT_NCP_H_

#include <stdint.h>

#define BT_NCP_STACK_SIZE (1024U)
#define BT_NCP_PRIO (2U)

typedef struct bt_ncp_msg {
    uint8_t hr;
    uint16_t rmssd;
    uint16_t ppg_amplitude;
    uint16_t epc;
} bt_ncp_msg_t;

typedef struct bt_ncp_ts_msg {
    int64_t timestamp;
    bt_ncp_msg_t msg;
} bt_ncp_ts_msg_t;

int bt_ncp_init();
int bt_ncp_wait_for_connection(uint32_t timeout_ms);
int bt_ncp_get_timestamped_msg(bt_ncp_ts_msg_t *p_msg);
void bt_ncp_run(void *p1, void *p2, void *p3);

#endif /* _BT_NCP_H_ */