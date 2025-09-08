// key_ebtn_thread.c — easy_button 线程轮询版（严格按 README 语义）
// 依赖：ebtn.c/.h、bit_array.h；backlight_ctrl.h

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "key_ebtn.h"        // ← 宏配置与 init 原型
#include "ebtn.h"            // easy_button
#include "backlight_ctrl.h"  // blctl_is_awake(), blctl_wake()

LOG_MODULE_REGISTER(key_ebtn, LOG_LEVEL_INF);

/* 1) 用 sw0 作为按键 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "sw0 alias 未定义：请在 overlay 把你的按键映射为 sw0"
#endif
static const struct gpio_dt_spec g_btn = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

/* 2) ebtn 参数（来自头文件里的宏） */
static const ebtn_btn_param_t s_param = EBTN_PARAMS_INIT(
    KEY_DEBOUNCE_PRESS_MS,     /* time_debounce（按下去抖） */
    KEY_DEBOUNCE_RELEASE_MS,   /* time_debounce_release（松开去抖） */
    KEY_CLICK_MIN_MS,          /* time_click_pressed_min */
    KEY_CLICK_MAX_MS,          /* time_click_pressed_max */
    KEY_MULTI_GAP_MS,          /* time_click_multi_max */
    KEY_KEEPALIVE_MS,          /* time_keepalive_period（长按心跳） */
    KEY_MAX_CONSEC             /* max_consecutive */
);

/* 3) 注册一个按键（ID=0） */
enum { KEY_ID_SW0 = 0 };
static ebtn_btn_t s_btns[] = {
    EBTN_BUTTON_INIT(KEY_ID_SW0, &s_param),
};

/* 4) 读电平回调：1=按下，0=松开（支持 ACTIVE_LOW） */
static uint8_t prv_get_state(struct ebtn_btn *btn)
{
    ARG_UNUSED(btn);
    int val = gpio_pin_get_dt(&g_btn);
    if (val < 0) val = 0;
    bool active = (g_btn.dt_flags & GPIO_ACTIVE_LOW) != 0;
    return active ? (val == 1) : (val == 0);
}

/* 5) 事件回调：长按靠 KEEPALIVE 的 keepalive_cnt；单/双击靠 ONCLICK */
static bool s_long_reported = false;

static void prv_evt(struct ebtn_btn *btn, ebtn_evt_t evt)
{
    switch (evt) {
    case EBTN_EVT_ONPRESS:
        s_long_reported = false;
        LOG_DBG("ONPRESS");
        break;

    case EBTN_EVT_KEEPALIVE:
        /* 第一帧 keepalive 视为长按成立（满足 keepalive 周期） */
        if (!s_long_reported && btn->keepalive_cnt >= 1) {
            s_long_reported = true;
            LOG_INF("Long press (keepalive_cnt=%u)", (unsigned)btn->keepalive_cnt);
        }
        break;

    case EBTN_EVT_ONRELEASE:
        LOG_DBG("ONRELEASE");
        /* 提示：若按住时长介于 CLICK_MAX_MS 与 KEEPALIVE_MS 之间，可能不产生任何事件 */
        break;

    case EBTN_EVT_ONCLICK: {
        uint16_t n = btn->click_cnt;   /* README：点击次数存在结构体里 */
        if (n == 1) {
            if (!blctl_is_awake()) {
                LOG_INF("Single click -> wake");
                blctl_wake();
            } else {
                LOG_INF("Single click (already awake)");
            }
        } else if (n == 2) {
            LOG_INF("Double click");
        } else {
            LOG_INF("%u-click", (unsigned)n);
        }
        } break;

    default:
        break;
    }
}

/* 6) 线程：每 KEY_POLL_MS 调一次 ebtn_process（传系统毫秒时钟） */
K_THREAD_STACK_DEFINE(key_stack, KEY_THREAD_STACK);
static struct k_thread key_thread_data;

static void key_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
    for (;;) {
        ebtn_process((ebtn_time_t)k_uptime_get_32());
        k_msleep(KEY_POLL_MS);
    }
}

/* 7) 对外初始化 */
int key_ebtn_thread_init(void)
{
    if (!gpio_is_ready_dt(&g_btn)) {
        LOG_ERR("sw0 not ready");
        return -ENODEV;
    }
    int ret = gpio_pin_configure_dt(&g_btn, GPIO_INPUT);
    if (ret) return ret;

    if (!ebtn_init(s_btns, EBTN_ARRAY_SIZE(s_btns),
                   NULL, 0,
                   prv_get_state, prv_evt)) {
        LOG_ERR("ebtn_init failed");
        return -EINVAL;
    }

    k_thread_create(&key_thread_data, key_stack, K_THREAD_STACK_SIZEOF(key_stack),
                    key_thread, NULL, NULL, NULL,
                    KEY_THREAD_PRIO, 0, K_NO_WAIT);
    k_thread_name_set(&key_thread_data, "key_ebtn");

    /* 拆成两条日志，避免 cbprintf 变参打包限制 */
    LOG_INF("key_ebtn ready: poll=%dms, deb(p/r)=%d/%d",
            (int)KEY_POLL_MS, (int)KEY_DEBOUNCE_PRESS_MS, (int)KEY_DEBOUNCE_RELEASE_MS);
    LOG_INF("click=[%d..%d]ms, gap=%dms, keepalive=%dms",
            (int)KEY_CLICK_MIN_MS, (int)KEY_CLICK_MAX_MS,
            (int)KEY_MULTI_GAP_MS, (int)KEY_KEEPALIVE_MS);
    return 0;
}
