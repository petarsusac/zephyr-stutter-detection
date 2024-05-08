#ifndef _BT_NCP_H_
#define _BT_NCP_H_

#include <stdint.h>

typedef struct bt_ncp_msg {
    uint8_t data_1;
} bt_ncp_msg_t;

typedef struct bt_ncp_ts_msg {
    char timestamp[20];
    bt_ncp_msg_t msg;
} bt_ncp_ts_msg_t;

int bt_ncp_init();
int bt_ncp_wait_for_connection(uint32_t timeout_ms);
int bt_ncp_get_timestamped_msg(bt_ncp_ts_msg_t *p_msg);
void bt_ncp_run(void *p1, void *p2, void *p3);

#endif /* _BT_NCP_H_ */