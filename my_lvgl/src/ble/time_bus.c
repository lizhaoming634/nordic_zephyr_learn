/* time_bus.c — NCS 3.0.2 兼容的 Zbus Listener 实现 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(time_bus, LOG_LEVEL_INF);

#include <zephyr/posix/time.h>
#include <zephyr/zbus/zbus.h>
#include <errno.h>

#include "time_bus.h"
#include "ui_time_display.h"

/* 回调：有新消息发布到 time_chan 时触发 */


/* 定义通道，并把 Listener 作为观察者挂上去 */
ZBUS_CHAN_DEFINE(time_chan,
    struct time_model,           /* 消息类型 */
    NULL,                        /* validator */
    NULL,                        /* user_data */
    ZBUS_OBSERVERS(time_listener),
    ZBUS_MSG_INIT(.epoch = 0)    /* 初始消息 */
);


static void time_update_cb(const struct zbus_channel *chan)
{
    const struct time_model *msg;
    if (&time_chan == chan) {
        msg = zbus_chan_const_msg(chan); // Direct message access
        LOG_INF("time_bus: got epoch=%lld", (long long)msg->epoch);
    }

   
    /* 尝试设置系统 CLOCK_REALTIME（如果 POSIX 时钟可用会成功） */
    struct timespec ts = { .tv_sec = (time_t)msg->epoch, .tv_nsec = 0 };
    if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
        LOG_INF("clock_settime OK: %ld", (long)msg->epoch);
    } else {
        LOG_ERR("clock_settime failed, errno=%d", errno);
    }
     /* 2) 让 UI 立刻刷新一次（内部用 lv_async_call 切回 LVGL 线程） */
    ui_time_display_refresh();
}

/* 定义 Listener */
ZBUS_LISTENER_DEFINE(time_listener, time_update_cb);

