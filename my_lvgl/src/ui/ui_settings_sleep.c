// ui_settings_sleep.c
#include "ui_settings_sleep.h"
#include "ui_main_view.h"
#include <zephyr/logging/log.h>
#include <lvgl.h>
LOG_MODULE_REGISTER(ui_sleep, LOG_LEVEL_INF);

// 来自 backlight_ctrl.c
extern void     blctl_set_timeout_s(uint32_t seconds, bool persist);
extern uint32_t blctl_get_timeout_s(void);

// 选项与秒数映射（最后一项 Never=0）
static const char *opt_str =
    "10s\n"
    "20s\n"
    "30s\n"
    "40s\n"
    "50s\n"
    "Never";
static const uint16_t k_opt_sec[] = {10, 20, 30, 40, 50, 0};
#define OPT_COUNT (sizeof(k_opt_sec)/sizeof(k_opt_sec[0]))

static lv_obj_t *s_roller;

// —— 点击“确认”：仅保存并返回（不再调用 blctl_wake）——
static void on_confirm_clicked(lv_event_t *e)
{
    ARG_UNUSED(e);
    uint16_t idx = lv_roller_get_selected(s_roller);
    if (idx >= OPT_COUNT) idx = OPT_COUNT - 1;

    uint32_t sec = k_opt_sec[idx];
    blctl_set_timeout(sec, true);   // 持久化保存；backlight_ctrl 内部负责重排auto-off

    // 返回主界面
    back_main_view(e);
   
}

lv_obj_t * ui_create_sleep_setting_page(lv_obj_t *parent)
{
    // Roller
    s_roller = lv_roller_create(parent);
    lv_roller_set_options(s_roller, opt_str, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_style_text_align(s_roller, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_roller);
    LOG_INF("test1");
    // 进入页面时：根据当前超时，直接在线性表里找匹配项；找不到就兜底到 30s（或你想要的默认）
    uint32_t cur_sec = blctl_get_timeout();   // 0=Never
    uint16_t idx = 0;
    bool matched = false;
    for (uint16_t i = 0; i < OPT_COUNT; i++) {
        if (k_opt_sec[i] == cur_sec) { idx = i; matched = true; break; }
    }
    if (!matched) {
        // 兜底策略：就近选择，或固定落到 30s
        // 这里演示“固定落到30s”
        for (uint16_t i = 0; i < OPT_COUNT; i++) {
            if (k_opt_sec[i] == 30) { idx = i; break; }
        }
    }
    lv_roller_set_selected(s_roller, idx, LV_ANIM_OFF);
    LOG_INF("test2");
    // 确认按钮（底部居中）
    lv_obj_t *confirm_btn = lv_button_create(parent);
    lv_obj_set_width(confirm_btn, lv_pct(80));
    lv_obj_align(confirm_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(confirm_btn, on_confirm_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(confirm_btn);
    lv_label_set_text(label, "confirm");
    lv_obj_center(label);

    return parent;
}