#pragma once
#include <zephyr/bluetooth/conn.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 每个命令的处理函数签名：
 * - conn：哪条连接上的数据（可用于定向回包）
 * - frame：完整协议帧（建议 16B，参见 BLE_FRAME_LEN）
 * - frame_len：帧长度
 * 返回值：保留，暂用 0 表示已处理
 */
typedef int (*ble_cmd_handler_t)(struct bt_conn *conn,
                                 const uint8_t *frame,
                                 uint16_t frame_len);

/* 初始化协议层（内部会把 RX 回调接到 transport） */
int  ble_proto_init(void);

/* 注册/覆盖某个命令的处理函数 */
int  ble_proto_register(uint8_t cmd, ble_cmd_handler_t handler);

#ifdef __cplusplus
}
#endif
