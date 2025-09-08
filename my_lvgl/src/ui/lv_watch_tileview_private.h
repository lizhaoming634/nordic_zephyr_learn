#ifndef __LV_WATCH_TILEVIEW_PRIVATE_H
#define __LV_WATCH_TILEVIEW_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

//#include <lvgl/src/core/lv_obj_private.h>
#include "lv_watch_tileview.h"
#include <lvgl.h>
#if 1

    typedef struct {
        lv_obj_t obj;
        lv_obj_t* tile_act;
        lv_obj_t* tile_center;
        lv_point_t scroll_center;

        uint8_t main_cnt;
        lv_dir_t cross_dir;
        uint8_t main_rotated : 1;
        uint8_t cross_overlapped : 1;

        lv_style_t style;
} watch_tileview_t;

typedef struct {
    lv_obj_t obj;
    lv_dir_t dir;
} watch_tileview_tile_t;

#endif


#endif // ! __WATCH_TILEVIEW_PRIVATE_H
