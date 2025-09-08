#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_proto, LOG_LEVEL_INF);

#include "ble_proto.h"
#include "ble_transport.h"
#include "ble_defs.h"

static ble_cmd_handler_t cmd_table[256] = {0};  // 命令→处理函数

int ble_proto_register(uint8_t cmd, ble_cmd_handler_t handler)
{
    cmd_table[cmd] = handler;
    return 0;
}

/* 由 transport 层回调进来：一收到帧就做最小校验并分发 */
static void proto_on_rx(struct bt_conn *conn, const uint8_t *frame, uint16_t frame_len)
{
    if (frame_len < 1) {
        LOG_WRN("RX frame too short");
        return;
    }

    const uint8_t cmd = frame[0];
    LOG_INF("RX cmd=0x%02X len=%u", cmd, frame_len);

    ble_cmd_handler_t handler = cmd_table[cmd];
    if (handler) {
        (void)handler(conn, frame, frame_len);
    } else {
        LOG_WRN("No handler for cmd=0x%02X", cmd);
    }
}

int ble_proto_init(void)
{
    /* 把协议分发函数挂到通道层的 RX 回调 */
    return ble_transport_init(proto_on_rx);
}
