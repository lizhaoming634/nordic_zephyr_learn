#ifndef UI_TIME_DISPLAY_H
#define UI_TIME_DISPLAY_H

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <stdint.h>

int ui_time_display_init(lv_obj_t* parent);
/* 立刻刷新一次（异步/同步皆可用在此演示） */
void ui_time_display_refresh(void);
/* 来自 zbus 的“新时间”钩子：把 epoch 喂给 UI 作为锚点 */
void ui_time_display_on_epoch(int64_t epoch_s);

#endif /* UI_TIME_DISPLAY_H */
