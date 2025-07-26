/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <app_event_manager.h>

#define MODULE main

#include <caf/events/module_state_event.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/display/cfb.h>

LOG_MODULE_REGISTER(MODULE);


static void bmi270_init(void)
{
    const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(bmi270));
	struct sensor_value full_scale, sampling_freq, oversampling;

    if (dev == NULL) {
        printk("\nError: no device found.\n");
        return;
    }

    if (!device_is_ready(dev)) {
        printk("\nError: device %s is not ready.\n", dev->name);
        return;
    }   

    	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 2;            /* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);

/* Setting scale in degrees/s to match the sensor scale */
	full_scale.val1 = 500;          /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change sampling frequency to
	 * 0.0Hz before changing other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);

    printk("Found device \%s\", getting sensor data...\n", dev->name);
}

static void display_init(void)
{
	const struct device *display = DEVICE_DT_GET(DT_NODELABEL(ssd1306));
	if (!device_is_ready(display)) {
		LOG_ERR("Display not ready");
		return;
	}
	if (cfb_framebuffer_init(display)) {
		LOG_ERR("Framebuffer initialization failed!");
		return;
	}
	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);
		cfb_set_kerning(display, 0);
	cfb_framebuffer_set_font(display, 8);
}
int main(void)
{
	int ret;
	display_init();

	bmi270_init();
	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Event Manager not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}

	module_set_state(MODULE_STATE_READY);
	return 0;
}
