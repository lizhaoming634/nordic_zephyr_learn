/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <lv_demos.h>
#include <lv_examples.h>
#include <lvgl_mem.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <hal/nrf_gpio.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define LCD_BLK_PIN NRF_GPIO_PIN_MAP(0, 29)   // Backlight pin

#define UI_THREAD_STACK_SIZE 4096
#define UI_THREAD_PRIORITY 2

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
const struct device *input_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_keyboard_scan));

extern void ui_thread(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(ui_thread_id, UI_THREAD_STACK_SIZE, 
				ui_thread, NULL, NULL, NULL,
				UI_THREAD_PRIORITY, 0, 0);

				
void ui_thread(void *arg1, void *arg2, void *arg3)
{
	nrf_gpio_cfg_output(LCD_BLK_PIN);
	nrf_gpio_pin_set(LCD_BLK_PIN);

	if (!device_is_ready(input_dev)) {
		LOG_ERR("Input device is not ready\n");
		return;
	}
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device is not ready\n");
		return;
	}

	lv_example_arc_1();
	lv_timer_handler();
	display_blanking_off(display_dev);


	while (1) {
		k_sleep(K_MSEC(30));
		lv_timer_handler();
	}
}
		
void printf_thread(void *arg1, void *arg2, void *arg3)
{ 
	while(1)
	{
		k_sleep(K_MSEC(2000));
		LOG_INF("printf_thread\n");
	}
}
K_THREAD_DEFINE(printf_thread_id, 1024, 
				printf_thread, NULL, NULL, NULL,
				3, 0, 0);
				
int main(void)
{
	while(1)
	{
		// lv_chart_set_next(ui_Chart1, series1, 100);
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
