// backlight_ctrl.c
#include "backlight_ctrl.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#if IS_ENABLED(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

LOG_MODULE_REGISTER(blctl, LOG_LEVEL_INF);

/* ==== Devicetree：要求 overlay 里有 aliases { bl = &backlight_pwm; } 且 chosen: zephyr,display ==== */
#define BACKLIGHT_NODE   DT_ALIAS(backlight)
#define DISP_NODE DT_CHOSEN(zephyr_display)

static const struct pwm_dt_spec backlight_pwm = PWM_DT_SPEC_GET(BACKLIGHT_NODE);
static const struct device       *display     = DEVICE_DT_GET(DISP_NODE);


/* ==== 默认参数（可在 prj.conf 用 -D 覆盖） ==== */
#ifndef BLCTL_TIMEOUT_S_DEFAULT
#define BLCTL_TIMEOUT_S_DEFAULT  20     /* 0=永不自动熄灭 */
#endif
#ifndef BLCTL_BRIGHTNESS_DEFAULT
#define BLCTL_BRIGHTNESS_DEFAULT 100   /* 0~100 (%) */
#endif
#ifndef BLCTL_WAKE_THROTTLE_MS
#define BLCTL_WAKE_THROTTLE_MS 1000   // 1s 内只生效一次；可在 prj.conf 用 -DBLCTL_WAKE_THROTTLE_MS=xxx 覆盖
#endif

static uint32_t s_last_wake_ms = 0;
/* ==== 状态 & 设置（带持久化） ==== */
static uint32_t s_timeout_s      = BLCTL_TIMEOUT_S_DEFAULT;
static uint8_t  s_brightness_pct = BLCTL_BRIGHTNESS_DEFAULT;
static bool     s_awake          = false;
  
static struct k_work_delayable s_off_work;

/* ==== 内部工具 ==== */
static inline uint32_t duty_ns(uint32_t period_ns, uint8_t pct)
{
    pct = CLAMP(pct, 0, 100);
    if (pct == 0)       return 0;
    if (pct >= 100)     return period_ns;
    return (uint32_t)((uint64_t)period_ns * pct / 100U);
}


#if IS_ENABLED(CONFIG_SETTINGS)
/* settings: /blctl/{timeout_s,brightness_pct} */
static int blctrl_settings_set(const char *name, size_t len, settings_read_cb read_cb, 
                                                                        void *cb_arg)
{ 
    if (!strcmp(name, "timeout_s") && len == sizeof(uint32_t)) {
        (void)read_cb(cb_arg, &s_timeout_s, sizeof(uint32_t));
        return 0;
    } else if (!strcmp(name, "brightness_pct") && len == sizeof(uint8_t)) {
        (void)read_cb(cb_arg, &s_brightness_pct, sizeof(uint8_t));
        s_brightness_pct = CLAMP(s_brightness_pct, 0, 100);
        return 0;
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(blctl, "blctl", NULL, blctl_settings_set, NULL, NULL);
static inline void blctl_save_timeout(void)
{
    (void)settings_save_one("blctl/timeout_s", &s_timeout_s, sizeof(s_timeout_s));
}
static inline void blctl_save_brightness(void)
{
    (void)settings_save_one("blctl/brightness_pct", &s_brightness_pct, sizeof(s_brightness_pct));
}
#else
static inline void blctl_save_timeout(void)     {}
static inline void blctl_save_brightness(void)  {}
#endif

/* ==== 自动熄灭 ==== */
static void off_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    /* 先关背光，再 blank 屏 */
    if (device_is_ready(backlight_pwm.dev)) {
        (void)pwm_set_dt(&backlight_pwm, backlight_pwm.period, 0);
    }
    if (device_is_ready(display)) {
        (void)display_blanking_on(display);
    }
    s_awake = false;
    LOG_INF("BL OFF, display blank");
}

/* ==== 实现 ==== */
int blctl_init(void)
{
#if IS_ENABLED(CONFIG_SETTINGS)
    (void)settings_subsys_init();
    (void)settings_load();             /* 可能会覆盖默认 s_timeout_s / s_brightness_pct */
#endif
    if (!device_is_ready(backlight_pwm.dev)) {
        LOG_ERR("PWM backlight not ready");
        return -ENODEV;
    }
    if (!device_is_ready(display)) {
        LOG_WRN("Display not ready");
    }

    k_work_init_delayable(&s_off_work, off_work_handler);
    blctl_wake();
    LOG_INF("blctl ready: timeout=%us, brightness=%u%%", s_timeout_s, s_brightness_pct);
    return 0;    
}

void blctl_wake(void)
{
    uint32_t now = k_uptime_get_32();

    /* 若已是亮屏且在节流窗口内，直接返回：避免频繁 display/pwm/重排定时器 */
    if (s_awake && BLCTL_WAKE_THROTTLE_MS > 0 &&
        (uint32_t)(now - s_last_wake_ms) < BLCTL_WAKE_THROTTLE_MS) {
        return;
    }
    s_last_wake_ms = now;

    if (device_is_ready(display)) {
        int r = display_blanking_off(display);
        if (r) LOG_WRN("display_blanking_off ret=%d", r);
    }
    if (device_is_ready(backlight_pwm.dev)) {
        int r = pwm_set_dt(&backlight_pwm, backlight_pwm.period, duty_ns(backlight_pwm.period, s_brightness_pct));
        if (r) LOG_WRN("pwm_set_dt ON ret=%d", r);
    }
    s_awake = true;

    if (s_timeout_s == 0) {
        (void)k_work_cancel_delayable(&s_off_work);
    } else {
        k_work_reschedule(&s_off_work, K_SECONDS(s_timeout_s));
    }
}

void blctl_blank(void)
{
    (void)k_work_cancel_delayable(&s_off_work);

    if (device_is_ready(backlight_pwm.dev)) {
        (void)pwm_set_dt(&backlight_pwm, backlight_pwm.period, 0);
    }
    if (device_is_ready(display)) {
        (void)display_blanking_on(display);
    }
    s_awake = false;
    LOG_INF("BL OFF, display blank");
}

bool blctl_is_awake(void)
{
    return s_awake;
}

int blctl_set_timeout(uint32_t sec, bool persist)
{
    s_timeout_s = sec;
    if (persist) blctl_save_timeout();

    if (s_awake) {
        if (s_timeout_s == 0) {
            (void)k_work_cancel_delayable(&s_off_work);
        } else {
            k_work_reschedule(&s_off_work, K_SECONDS(s_timeout_s));
        }
    }
    return 0;
}

uint32_t blctl_get_timeout(void)
{
    return s_timeout_s;
}

int blctl_set_brightness(uint8_t pct, bool persist)
{
    s_brightness_pct = CLAMP(pct, 0, 100);
    if (persist) blctl_save_brightness();

    if (s_awake && device_is_ready(backlight_pwm.dev)) {
        (void)pwm_set_dt(&backlight_pwm, backlight_pwm.period, duty_ns(backlight_pwm.period, s_brightness_pct));
    }
    return 0;
}

uint8_t blctl_get_brightness(void)
{
    return s_brightness_pct;
}

