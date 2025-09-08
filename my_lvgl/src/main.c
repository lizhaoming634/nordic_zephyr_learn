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
#include <zephyr/posix/time.h>
#include "ble/ble_proto.h"
#include "ble/ble_comm.h"'
#include "sensor/steps_service.h"
#include "ui/ui_app.h"
#include "app/key_ebtn.h"
#include "app/backlight_ctrl.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);


int main(void)
{

	ble_proto_init();        /* 挂上协议调度（把 RX 接走） */
    ble_comm_init();         /* 开蓝牙 + 广播 + 连接回调 */
	steps_service_start();
	blctl_init();
    key_ebtn_thread_init();   /* 线程版按键处理（无中断） */
	ui_app_init();
	    

	while(1)
	{
		// lv_chart_set_next(ui_Chart1, series1, 100);
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
