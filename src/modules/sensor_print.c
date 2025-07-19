#include <caf/events/sensor_event.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_print);

static bool event_handler(const struct app_event_header *aeh)
{
    if (is_sensor_event(aeh)) {
        const struct sensor_event *event = cast_sensor_event(aeh);
        size_t data_cnt = sensor_event_get_data_cnt(event);
        struct sensor_value *data = sensor_event_get_data_ptr(event);

        LOG_INF("Sensor: %s, data count: %d", event->descr, data_cnt);
        for (size_t i = 0; i < data_cnt; i++) {
            LOG_INF("  value[%d]: %d.%06d", i, data[i].val1, data[i].val2);
        }
        return false;
    }
    return false;
}

APP_EVENT_LISTENER(sensor_print, event_handler);
APP_EVENT_SUBSCRIBE(sensor_print, sensor_event);