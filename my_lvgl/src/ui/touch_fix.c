#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(touch_fix, LOG_LEVEL_INF);
/* 通过 alias(input) 锁定你的触摸设备 */
static const struct device *const touch_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_keyboard_scan));

/* 根据 st7789v 的 y-offset=20 做补偿：Y' = Y - 20 */
#define TOUCH_Y_OFFSET 20

/* 如果你在 prj.conf 里配置了 LVGL 分辨率，可用宏；否则写死 280 */
#ifndef CONFIG_LVGL_VER_RES_MAX
#define CONFIG_LVGL_VER_RES_MAX 280
#endif

static void touch_fix_cb(struct input_event *evt, void *user_data)
{
    ARG_UNUSED(user_data);

    /* 只处理来自 cst816s 的绝对坐标事件 */
    if (evt->dev != touch_dev) {
        return;
    }
     // 触摸到任意坐标/按键事件时，立刻重置灭屏倒计时（并点亮）
    if ((evt->type == INPUT_EV_KEY) ||
        (evt->type == INPUT_EV_ABS && (evt->code == INPUT_ABS_X || evt->code == INPUT_ABS_Y))) {
        blctl_wake();
    }
    if (evt->type == INPUT_EV_ABS && evt->code == INPUT_ABS_Y) {
        int32_t v = evt->value - TOUCH_Y_OFFSET;
        if (v < 0) v = 0;
        if (v > (CONFIG_LVGL_VER_RES_MAX - 1)) v = (CONFIG_LVGL_VER_RES_MAX - 1);
        evt->value = v;
    }
}

/* 全局注册输入回调 */
INPUT_CALLBACK_DEFINE(NULL, touch_fix_cb, NULL);
