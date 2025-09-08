#include "ui_app.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>

#include "ui_time_display.h"
#include "ui_main_view.h"
#include "ui_app.h"

LOG_MODULE_REGISTER(ui_app, LOG_LEVEL_INF);



#define UI_THREAD_STACK_SIZE 1024 * 8
#define UI_THREAD_PRIORITY 2

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
const struct device *input_dev   = DEVICE_DT_GET(DT_CHOSEN(zephyr_keyboard_scan));


/* UI 侧实现回调（示例） */


void ui_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1); ARG_UNUSED(arg2); ARG_UNUSED(arg3);
    k_thread_name_set(k_current_get(), "ui_thread_id");
    LOG_INF("UI thread start");

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device is not ready");
        return;
    }
    if (!device_is_ready(input_dev)) {
        LOG_WRN("Input device not ready (ok if none)");
    }
    display_blanking_off(display_dev);

    ui_show_main_with_watch_tileview();  // 这里面应当调用 ui_steps_display_init() 创建步数控件
    lv_timer_handler();                  // 做一次首帧渲染

    while (1) {
        k_sleep(K_MSEC(30));
        lv_timer_handler();
    }
}

static K_THREAD_STACK_DEFINE(ui_stack, UI_THREAD_STACK_SIZE);
static struct k_thread ui_thread_t;

void ui_app_init(void)
{
    /* create UI thread dynamically */
    k_thread_create(&ui_thread_t, ui_stack, K_THREAD_STACK_SIZEOF(ui_stack),
                    ui_thread, NULL, NULL, NULL,
                    UI_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&ui_thread_t, "ui");
}
