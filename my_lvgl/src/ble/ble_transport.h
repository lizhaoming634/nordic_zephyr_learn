#pragma once
#include <zephyr/bluetooth/conn.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 收到手机写到 RX 特征的回调（完整一帧 16B） */
typedef void (*ble_rx_cb_t)(struct bt_conn *conn, const uint8_t *data, uint16_t len);

/* 初始化“通道”：注册回调，静态GATT服务已经在本文件里定义好 */
int ble_transport_init(ble_rx_cb_t on_rx);

/* 从 TX（Notify）发数据到手机；conn==NULL 表示发给所有已订阅连接 */
int ble_transport_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
