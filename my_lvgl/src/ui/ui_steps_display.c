#include "ui_steps_display.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ui_steps_display, LOG_LEVEL_INF);

static lv_obj_t *s_steps_label;
static atomic_t  s_latest_steps = ATOMIC_INIT(0);
static uint32_t  s_last_drawn   = 0;

static void _do_update(void){
    if(!s_steps_label) {
        LOG_WRN("steps label not ready yet");
        return;
    }
    uint32_t steps = (uint32_t)atomic_get(&s_latest_steps);
    if (steps == s_last_drawn) {
        LOG_DBG("steps unchanged: %u (skip draw)", (unsigned)steps);
        return;
    }
    s_last_drawn = steps;
    lv_label_set_text_fmt(s_steps_label, "%u", steps);
    LOG_DBG("steps label updated: %u", (unsigned)steps);
}

static void _async_cb(void *user){ (void)user; _do_update(); }

int ui_steps_display_init(lv_obj_t *parent){
    s_steps_label = lv_label_create(parent);
    lv_obj_align(s_steps_label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(s_steps_label, "0");
    s_last_drawn = 0;
    LOG_INF("steps label created: %p", s_steps_label);
    _do_update();
    return 0;
}

void ui_steps_display_set_latest(uint32_t steps){
    atomic_set(&s_latest_steps, (atomic_val_t)steps);
    LOG_DBG("ui request to show steps=%u", (unsigned)steps);
    lv_async_call(_async_cb, NULL);  /* 切回 UI 线程更新 */
}
