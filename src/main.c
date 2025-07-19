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

LOG_MODULE_REGISTER(MODULE);

int main(void)
{
	int ret;

	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Event Manager not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}

	module_set_state(MODULE_STATE_READY);
	return 0;
}
