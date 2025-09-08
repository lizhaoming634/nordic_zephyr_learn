#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_transport, LOG_LEVEL_INF);

#include <zephyr/bluetooth/gatt.h>
#include <string.h>
#include "ble_defs.h"
#include "ble_transport.h"

/* 128-bit UUID 对象 */
static struct bt_uuid_128 UUID_SVC = BT_UUID_INIT_128(BT_UUID_TSVC_VAL);
static struct bt_uuid_128 UUID_RX  = BT_UUID_INIT_128(BT_UUID_TRX_VAL);
static struct bt_uuid_128 UUID_TX  = BT_UUID_INIT_128(BT_UUID_TTX_VAL);



/* 客户端是否打开了 TX 通知（CCC） */
static bool s_notify_enabled = false;

/* 上层回调（协议层注册进来） */
static ble_rx_cb_t s_on_rx = NULL;

/* --- CCC 回调：记录是否启用通知 --- */
static void tx_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    s_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("TX notify %s", s_notify_enabled ? "ENABLED" : "DISABLED");
}

/* --- RX 写回调：收到手机写入的帧，直接上抛给协议层 --- */
static ssize_t rx_write_cb(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr,
                           const void *buf, uint16_t len,
                           uint16_t offset, uint8_t flags)
{
    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (s_on_rx == NULL) {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }
    /* 这里不强制 len==16，让协议层自己校验（便于将来扩帧） */
    s_on_rx(conn, buf, len);
    return len;
}

/* --- GATT 数据库：一个Service + 两个Char（RX/写、TX/通知+CCC） --- */
BT_GATT_SERVICE_DEFINE(timesync_svc,
    BT_GATT_PRIMARY_SERVICE(&UUID_SVC.uuid),

    BT_GATT_CHARACTERISTIC(&UUID_RX.uuid,
        BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
        BT_GATT_PERM_WRITE,
        NULL, rx_write_cb, NULL),

    BT_GATT_CHARACTERISTIC(&UUID_TX.uuid,
        BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
        NULL, NULL, NULL),

    BT_GATT_CCC(tx_ccc_cfg_changed,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

/* timesync_svc.attrs 布局：
 * [0]=Primary Service, [1]=RX Decl, [2]=RX Value,
 * [3]=RX UserDesc,     [4]=TX Decl, [5]=TX Value,
 * [6]=TX UserDesc,     [7]=TX CCC
 *
 * 我们要通知 TX Value，所以用 attrs[5]。
 * 若以后改了表结构，请同步更新该索引。
 */
#define TX_VALUE_ATTR   (&timesync_svc.attrs[5])

int ble_transport_init(ble_rx_cb_t on_rx)
{
    s_on_rx = on_rx;
    return 0;
}

int ble_transport_send(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
    if (!s_notify_enabled) {
        LOG_WRN("TX notify not enabled, drop len=%u", len);
        return -EACCES;
    }
    /* conn==NULL 表示发给所有已订阅连接；更细粒度可传入具体连接 */
    int err = bt_gatt_notify(conn, TX_VALUE_ATTR, data, len);
    if (err) {
        LOG_WRN("bt_gatt_notify failed: %d", err);
    }
    return err;
}
