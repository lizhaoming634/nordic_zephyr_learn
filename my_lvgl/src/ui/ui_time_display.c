#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ui_time_display, LOG_LEVEL_INF);

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/time.h>
#include <stdio.h>


static lv_obj_t   *s_time_label;
static lv_obj_t   *s_date_label;
static lv_timer_t *s_timer;

/* —— 从 CLOCK_REALTIME 读当前 epoch（秒） —— */
static bool read_realtime(time_t *out)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        *out = (time_t)ts.tv_sec;
        return true;
    }
    return false;
}

/* —— 画面更新：只用 CLOCK_REALTIME —— */
static void do_update_labels(void)
{
    if (!s_time_label || !s_date_label) return;

    time_t now;
    if (!read_realtime(&now)) {
        /* 时钟尚未设置成功（或未启用 POSIX 时钟） */
        lv_label_set_text(s_time_label, "--:--:--");
        lv_label_set_text(s_date_label, "----/--/--");
        LOG_WRN("clock_gettime(CLOCK_REALTIME) failed");
        return;
    }

    struct tm tm_local;
    if (!localtime_r(&now, &tm_local)) {
        lv_label_set_text(s_time_label, "--:--:--");
        lv_label_set_text(s_date_label, "----/--/--");
        LOG_WRN("localtime_r failed");
        return;
    }

    char tbuf[16], dbuf[20];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d",
             tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec);
    snprintf(dbuf, sizeof(dbuf), "%04d-%02d-%02d",
             tm_local.tm_year + 1900, tm_local.tm_mon + 1, tm_local.tm_mday);

    lv_label_set_text(s_time_label, tbuf);
    lv_label_set_text(s_date_label, dbuf);
    lv_obj_invalidate(lv_screen_active());

    //LOG_INF("Update UI: %s %s", dbuf, tbuf);
}

/* LVGL 定时器：每秒刷新一次（运行在 LVGL 线程） */
static void timer_cb(lv_timer_t *t)
{
    ARG_UNUSED(t);
    do_update_labels();
}

/* 把“立即刷新”切回 LVGL 线程，避免跨线程操作 LVGL */
static void _async_refresh(void *user) { ARG_UNUSED(user); do_update_labels(); }

void ui_time_display_refresh(void)
{
    lv_async_call(_async_refresh, NULL);
}

/* 初始化：两个标签 + 1s 定时器 */
int ui_time_display_init(lv_obj_t* parent)
{
    s_time_label = lv_label_create(parent);
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_text(s_time_label, "--:--:--");
    lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_18, LV_PART_MAIN);

    s_date_label = lv_label_create(parent);
    lv_obj_align(s_date_label,LV_ALIGN_CENTER, 0, 20);
    lv_label_set_text(s_date_label, "----/--/--");
    lv_obj_set_style_text_font(s_date_label, &lv_font_montserrat_18, LV_PART_MAIN);
    s_timer = lv_timer_create(timer_cb, 1000, NULL);
    LOG_INF("UI time/date labels created, timer started");

    /* 启动时先刷一次（如果时钟没设过，会显示占位符并打印警告） */
    do_update_labels();
    return 0;
}
