#ifndef UI_STEPS_DISPLAY_H
#define UI_STEPS_DISPLAY_H

#include <lvgl.h>
#include <stdint.h>

int  ui_steps_display_init(lv_obj_t *parent);
void ui_steps_display_set_latest(uint32_t steps);

#endif
