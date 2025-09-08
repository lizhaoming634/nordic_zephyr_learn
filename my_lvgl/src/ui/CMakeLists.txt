# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_lvgl)

# 添加所有UI源文件到构建系统
target_sources(app PRIVATE
    lv_watch_tileview.c
    touch_fix.c

    ui_app.c
    ui_time_display.c
    ui_main_view.c
    ui_steps_display.c
    ui_settings_sleep.c
)



# 添加 ui 目录到头文件搜索路径
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


