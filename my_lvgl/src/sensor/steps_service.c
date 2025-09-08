#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_REGISTER(steps_app, LOG_LEVEL_INF);

#include "bmi270_hal.h"
#include "../third_party/bosch_bmi270/bmi270.h"  
#include "app/backlight_ctrl.h"
/* ========== zbus：步数消息（UI订阅者已在别处实现） ========== */
struct steps_msg { uint32_t steps; };

static void steps_ui_listener_cb(const struct zbus_channel *chan);
ZBUS_LISTENER_DEFINE(steps_ui_listener, steps_ui_listener_cb);

ZBUS_CHAN_DEFINE(steps_chan, struct steps_msg, NULL, NULL,
                 ZBUS_OBSERVERS(steps_ui_listener),
                 ZBUS_MSG_INIT(.steps = 0));

extern void ui_steps_display_set_latest(uint32_t steps);
static void steps_ui_listener_cb(const struct zbus_channel *chan)
{
    if (chan != &steps_chan) return;
    const struct steps_msg *m = zbus_chan_const_msg(chan);
    ui_steps_display_set_latest(m->steps);
}
static inline void publish_steps(uint32_t steps)
{
    const struct steps_msg m = { .steps = steps };
    (void)zbus_chan_pub(&steps_chan, &m, K_NO_WAIT);
}


/* ========== INT1 GPIO 定义（与 overlay 的 irq-gpios 一致） ========== */
#define BMI270_NODE DT_NODELABEL(bmi270)
static const struct gpio_dt_spec s_int1 = GPIO_DT_SPEC_GET(BMI270_NODE, irq_gpios);

/* ========== 线程 & 去抖参数 ========== */
#define STEP_STACK      2048
#define STEP_PRIO       7
#define MIN_STEP_MS     300   /* 两步最小间隔：调 250~400ms 过滤轻微晃动 */

K_THREAD_STACK_DEFINE(step_stack, STEP_STACK);
static struct k_thread step_thread;
static K_SEM_DEFINE(s_irq_sem, 0, 64);

static void int1_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev); ARG_UNUSED(cb); ARG_UNUSED(pins);
    k_sem_give(&s_irq_sem);
}
static struct gpio_callback s_cb;

/* ========== 主线程入口：中断→判位→步数 & 抬腕 ========== */
static void step_thread_entry(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

    if (bmi270_steps_init() != 0) {
        LOG_ERR("bmi270_steps_init failed");
        return;
    }

    /* 配置 INT1：与设备树 irq-gpios=GPIO_ACTIVE_HIGH 对齐 → 上升沿触发 */
    if (!device_is_ready(s_int1.port)) {
        LOG_ERR("INT1 port not ready");
        return;
    }

    int ret;
    ret = gpio_pin_configure_dt(&s_int1, GPIO_INPUT);
    if (ret) {
        LOG_ERR("INT1 configure failed: %d", ret);
        return;
    }

    /* 正确的回调初始化流程：先 init，再 add，最后开中断 */
    gpio_init_callback(&s_cb, int1_isr, BIT(s_int1.pin));

    ret = gpio_add_callback(s_int1.port, &s_cb);
    if (ret) {
        LOG_ERR("INT1 add callback failed: %d", ret);
        return;
    }

    ret = gpio_pin_interrupt_configure_dt(&s_int1, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret) {
        LOG_ERR("INT1 interrupt configure failed: %d", ret);
        return;
    }

    LOG_INF("INT1 ready (edge-to-active)");

    uint32_t total = 0;
    int64_t  last_step_ms = -100000;

    while (1) {
        /* 合并“边沿风暴”：先取一次，再把短时间多余 token 清空 */
        k_sem_take(&s_irq_sem, K_FOREVER);
        while (k_sem_take(&s_irq_sem, K_NO_WAIT) == 0) { /* drain */ }

        uint16_t st = 0;
        if (bmi270_steps_get_int_status(&st) != 0) {
            continue;
        }

        /* 1) 单步事件（这版 SDK 中 Detector/Counter 共用 0x02 状态位） */
        if (st & BMI270_STEP_CNT_STATUS_MASK) {
            int64_t now = k_uptime_get();
            if ((now - last_step_ms) >= MIN_STEP_MS) {
                last_step_ms = now;
                total++;
                publish_steps(total);
                LOG_INF("step +1 (total=%u)", total);
            }
        }

        /* 2) 抬腕手势：命中状态位再读手势输出，pivot_up(=2) 则亮屏 */

        if (st & BMI270_WRIST_GEST_STATUS_MASK) {
            uint8_t g = 0;
            if (bmi270_read_wrist_gesture(&g) == 0 && g == 2 /* pivot_up */) {
                /* 如果屏幕没有亮 */
                if (!blctl_is_awake()) {
                    blctl_wake();
                }          
                LOG_INF("wrist pivot_up -> wake");
            }
        }
    }
}

/* ========== 对外启动入口 ========== */
int steps_service_start(void)
{
    k_thread_create(&step_thread, step_stack, STEP_STACK,
                    step_thread_entry, NULL, NULL, NULL,
                    STEP_PRIO, 0, K_NO_WAIT);
    k_thread_name_set(&step_thread, "steps");
    return 0;
}
