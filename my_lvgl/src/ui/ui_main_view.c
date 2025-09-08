#include "lv_watch_tileview.h"
#include "ui_time_display.h"
#include "ui_steps_display.h"
#include "ui_settings_sleep.h"
#include "app/backlight_ctrl.h"
/* ------- screens ------- */
static lv_obj_t* scr_main = NULL;
static lv_obj_t* scr_menu = NULL;

/* ------- fwd decls ------- */
static void on_open_menu(lv_event_t* e);

static void build_scr_menu(void);
static void build_scr_main(void);
static void fill_tile(lv_obj_t* tile, const char* title, lv_color_t color);
static void add_open_button_to_up(lv_obj_t* up_tile);

/* ========================================================= */
/*  Only paint background + title on a tile (no buttons here) */
/* ========================================================= */
static void fill_tile(lv_obj_t* tile, const char* title, lv_color_t color)
{
    lv_obj_set_style_bg_color(tile, color, 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);

    lv_obj_t* label = lv_label_create(tile);
    lv_label_set_text(label, title);
    lv_obj_center(label);
}

/* ========================================================= */
/*  Add the "Open Menu" button only to the UP tile           */
/*  (avoid focus → auto scroll by clearing focus flags)      */
/* ========================================================= */


static lv_obj_t * slider_label;
static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target_obj(e);
    blctl_set_brightness((uint8_t)lv_slider_get_value(slider), true);
}

static void add_open_button_to_up(lv_obj_t* up_tile)
{
    lv_obj_t* btn_open = lv_button_create(up_tile);
    lv_obj_align(btn_open, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t* lab_o = lv_label_create(btn_open);
    lv_label_set_text(lab_o, "Open Menu");
    lv_obj_center(lab_o);


    lv_obj_add_event_cb(btn_open, on_open_menu, LV_EVENT_CLICKED, NULL);

    lv_obj_t* slider = lv_slider_create(up_tile);
    lv_slider_set_value(slider, (int32_t)blctl_get_brightness(), LV_ANIM_OFF);
    lv_obj_set_size(slider, 180, 20);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void add_time_display(lv_obj_t* tile)
{
    ui_time_display_init(tile);
}

static void add_steps_display(lv_obj_t* tile)
{
    ui_steps_display_init(tile);
}

/* ========================================================= */
/*  Menu screen: a simple back button                        */
/* ========================================================= */
static void build_scr_menu(void)
{
    if (scr_menu && lv_obj_is_valid(scr_menu)) return;   // 已创建就不重复
    scr_menu = lv_obj_create(NULL);                      // 一定要创建屏幕
    lv_obj_clear_flag(scr_menu, LV_OBJ_FLAG_SCROLLABLE);
    ui_create_sleep_setting_page(scr_menu);
}

/* ========================================================= */
/*  Main screen with watch_tileview                          */
/* ========================================================= */
static void build_scr_main(void)
{
    if (scr_main) return;

    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_main, LV_OPA_COVER, 0);

    /* 1) Create the watch_tileview and fill the screen */
    lv_obj_t* tv = watch_tileview_create(scr_main);
    lv_obj_set_size(tv, lv_pct(100), lv_pct(100));
    lv_obj_center(tv);

    /* 2) Layout plan: 1 left, 1 right, 1 up, 1 down */
    const uint8_t tile_cnts[4] = { 1, 1, 1, 1 }; // {left, right, up, down}
    lv_obj_t* tiles[5]; /* total = left + center + right + up + down = 5 */

    watch_tileview_add_tiles(
        tv, tiles, tile_cnts,
        LV_DIR_HOR,   /* main_dir: horizontal */
        true,         /* main_rotated / loop on main axis */
        true          /* cross_overlapped */
    );

    /* 3) Name tiles for readability */
    const uint8_t left_cnt = tile_cnts[0];
    const uint8_t right_cnt = tile_cnts[1];
    const uint8_t up_cnt = tile_cnts[2];
    const uint8_t down_cnt = tile_cnts[3];
    const uint8_t main_cnt = left_cnt + right_cnt + 1;

    const uint8_t idx_left = 0;                 /* first left */
    const uint8_t idx_center = left_cnt;          /* center always at left_cnt */
    const uint8_t idx_right = left_cnt + 1;      /* first right */
    const uint8_t idx_up = main_cnt + 0;      /* first up */
    const uint8_t idx_down = main_cnt + up_cnt; /* first down */

    lv_obj_t* left = tiles[idx_left];
    lv_obj_t* center = tiles[idx_center];
    lv_obj_t* right = tiles[idx_right];
    lv_obj_t* up = tiles[idx_up];
    lv_obj_t* down = tiles[idx_down];

    /* 4) Paint each tile (no buttons here) */
    fill_tile(center, "CENTER", lv_palette_main(LV_PALETTE_BLUE));
    fill_tile(left, "LEFT", lv_palette_main(LV_PALETTE_GREY));
    fill_tile(right, "RIGHT", lv_palette_main(LV_PALETTE_TEAL));
    fill_tile(up, "UP", lv_palette_main(LV_PALETTE_ORANGE));
    fill_tile(down, "DOWN", lv_palette_main(LV_PALETTE_RED));

    /* 5) Only UP gets the "Open Menu" button (and it's non-focusable) */
    add_open_button_to_up(up);
    add_time_display(center);
    add_steps_display(left);
    /* 6) Force start at center as the last step (no animation, no jump) */
    watch_tileview_set_start_tile(tv, center);
}

/* ========================================================= */
/*  Event callbacks                                          */
/* ========================================================= */
static void on_open_menu(lv_event_t* e)
{
    (void)e;
    build_scr_menu();
    lv_screen_load_anim(scr_menu, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, false);
}

void back_main_view(lv_event_t* e)
{
    (void)e;
    if (!scr_main) build_scr_main();
    lv_screen_load_anim(scr_main, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, true);
}

/* ========================================================= */
/*  Public entry                                             */
/* ========================================================= */

void ui_show_main_with_watch_tileview(void)
{
    build_scr_main();
    lv_screen_load_anim(scr_main, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

}
