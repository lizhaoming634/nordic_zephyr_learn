// ui_settings_sleep.h
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create the "Sleep (screen timeout)" settings page.
 * The page contains a Roller with options:
 *   10s / 30s / 1 min / 3 min / 5 min / Never
 * It reads current timeout from bl_idle_get_timeout_ms(),
 * and on change calls bl_idle_set_timeout_ms() + bl_idle_on_user_activity().
 *
 * @param parent  LVGL parent object (screen, tab, or container)
 * @return        The created container object
 */
lv_obj_t * ui_create_sleep_setting_page(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
