# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_lvgl)


set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
    ${LVGL_DIR}/examples/
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_BENCHMARK app PRIVATE
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_avatar.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_lvgl_logo_argb.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_lvgl_logo_rgb.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_12_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_16_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_28_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/lv_demo_benchmark.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_WIDGETS app PRIVATE
    ${LVGL_DIR}/demos/widgets/assets/img_clothes.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_avatar.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_needle.c
    ${LVGL_DIR}/demos/widgets/assets/img_lvgl_logo.c
    ${LVGL_DIR}/demos/widgets/lv_demo_widgets.c
)

target_sources_ifdef(CONFIG_LV_BUILD_EXAMPLES app PRIVATE
    ${LVGL_DIR}/examples/widgets/tileview/lv_example_tileview_1.c
    ${LVGL_DIR}/examples/widgets/arc/lv_example_arc_1.c
    ${LVGL_DIR}/examples/widgets/arc/lv_example_arc_2.c

)

# 先加入第三方 Bosch 库（提供实现与公共包含路径）
add_subdirectory(src/third_party/bosch_bmi270)

add_subdirectory(src/ui)
add_subdirectory(src/ble)
add_subdirectory(src/sensor)
add_subdirectory(src/app)


include_directories(src/)