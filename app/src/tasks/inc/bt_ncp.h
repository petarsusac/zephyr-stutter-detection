#ifndef _BT_NCP_H_
#define _BT_NCP_H_

#include <stdint.h>

int bt_ncp_init();
int bt_ncp_wait_for_connection(uint32_t timeout_ms);

#endif /* _BT_NCP_H_ */