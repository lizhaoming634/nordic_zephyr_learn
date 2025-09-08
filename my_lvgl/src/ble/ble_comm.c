#include "ble_comm.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h> 
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_comm, LOG_LEVEL_INF);
#include "ble_defs.h"
#include "ble_defs.h"

static int advertise_start(void)
{
    /* 用你自定义服务的 128-bit UUID 填一段原始字节（小端） */
    const uint8_t uuid_adv[16] = { BT_UUID_TSVC_VAL };

    struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_UUID128_ALL, uuid_adv, sizeof(uuid_adv)),
    };

    int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("bt_le_adv_start failed: %d", err);
        return err;
    }
    LOG_INF("Advertising started");
    return 0;
}


/* 连接回调：断开后自动重启广播 */
static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connected failed (err %u)", err);
        (void)advertise_start();
        return;
    }
    LOG_INF("Connected");
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason 0x%02X)", reason);
    (void)advertise_start();
}

static struct bt_conn_cb s_conn_cb = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};

int ble_comm_advertise_restart(void)
{
    bt_le_adv_stop();
    return advertise_start();
}

int ble_comm_init(void)
{
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed: %d", err);
        return err;
    }
    LOG_INF("Bluetooth initialized");

    bt_conn_cb_register(&s_conn_cb);
    return advertise_start();
}
