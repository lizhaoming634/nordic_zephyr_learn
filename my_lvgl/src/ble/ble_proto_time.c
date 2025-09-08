#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_proto_time, LOG_LEVEL_INF);

#include <zephyr/posix/time.h>
#include "ble_defs.h"
#include "ble_proto.h"
#include "ble_transport.h"
#include "time_bus.h"

/* BCD 工具 */
static inline int     bcd2u(uint8_t b){ return (b>>4)*10 + (b & 0x0F); }
static inline uint8_t u2b(int v)      { return (uint8_t)(((v/10)<<4) | (v%10)); }

/* 0x01: 设置时间（BCD在 [1..6]），通过 zbus 发布给 time_bus 统一设置 CLOCK_REALTIME */
static int handle_set_time(struct bt_conn *conn, const uint8_t *frame, uint16_t frame_len)
{
    if (frame_len != BLE_FRAME_LEN) return 0;

    int y  = bcd2u(frame[1]); /* 00..99 → 2000+y */
    int m  = bcd2u(frame[2]);
    int d  = bcd2u(frame[3]);
    int hh = bcd2u(frame[4]);
    int mm = bcd2u(frame[5]);
    int ss = bcd2u(frame[6]);

    /* 简单合法性检查 */
    if (m < 1 || m > 12 || d < 1 || d > 31 || hh > 23 || mm > 59 || ss > 59) {
        LOG_WRN("SET_TIME invalid: %02d-%02d %02d:%02d:%02d", m, d, hh, mm, ss);
        return 0;
    }

    struct tm t = {0};
    t.tm_year = (2000 + y) - 1900;
    t.tm_mon  = m - 1;
    t.tm_mday = d;
    t.tm_hour = hh;
    t.tm_min  = mm;
    t.tm_sec  = ss;

    time_t epoch = mktime(&t);
    if (epoch == (time_t)-1) {
        LOG_WRN("mktime failed %04d-%02d-%02d %02d:%02d:%02d", 2000+y, m, d, hh, mm, ss);
        return 0;
    }

    struct time_model msg = { .epoch = (int64_t)epoch };
    int err = zbus_chan_pub(&time_chan, &msg, K_MSEC(50));
    if (err) {
        LOG_ERR("zbus publish failed: %d", err);
    } else {
        LOG_INF("SET_TIME epoch=%lld published", (long long)msg.epoch);
    }
    return 0;
}

/* 0x41: 获取时间 —— 读 CLOCK_REALTIME，编码成 16B 帧，用 TX/Notify 回手机 */
static int handle_get_time(struct bt_conn *conn, const uint8_t *frame, uint16_t frame_len)
{
    uint8_t rsp[BLE_FRAME_LEN] = {0};
    rsp[0] = CMD_GET_TIME;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        LOG_WRN("clock_gettime failed, errno=%d", errno);
        return 0;
    }
    struct tm tm_local;
    if (!localtime_r(&ts.tv_sec, &tm_local)) {
        LOG_WRN("localtime_r failed");
        return 0;
    }

    int year = tm_local.tm_year + 1900;
    rsp[1] = u2b(year % 100);
    rsp[2] = u2b(tm_local.tm_mon + 1);
    rsp[3] = u2b(tm_local.tm_mday);
    rsp[4] = u2b(tm_local.tm_hour);
    rsp[5] = u2b(tm_local.tm_min);
    rsp[6] = u2b(tm_local.tm_sec);

    ble_transport_send(conn, rsp, sizeof(rsp));
    LOG_INF("GET_TIME -> notify %02X %02X %02X %02X %02X %02X %02X ...",
            rsp[0], rsp[1], rsp[2], rsp[3], rsp[4], rsp[5], rsp[6]);
    return 0;
}

/* 模块初始化：向调度层注册两个命令 */
static int timesync_proto_init(const struct device *unused)
{
    ARG_UNUSED(unused);
    ble_proto_register(CMD_SET_TIME, handle_set_time);
    ble_proto_register(CMD_GET_TIME, handle_get_time);
    return 0;
}
SYS_INIT(timesync_proto_init, APPLICATION, 50);
