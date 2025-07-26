#include <caf/sensor_manager.h>

/* This configuration file is included only once from sensor_manager module and holds
 * information about the sampled sensors.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} sensor_manager_def_include_once;

static const struct caf_sampled_channel bmi270_chan[] = {
    {
        .chan = SENSOR_CHAN_ACCEL_XYZ,
        .data_cnt = 3,
    },
};



static const struct caf_sampled_channel bme280_chans[] = {
    {
        .chan = SENSOR_CHAN_AMBIENT_TEMP,
        .data_cnt = 1,
    },
    {
        .chan = SENSOR_CHAN_HUMIDITY,
        .data_cnt = 1,
    },
    {
        .chan = SENSOR_CHAN_PRESS,
        .data_cnt = 1,
    },
};

static const struct sm_sensor_config sensor_configs[] = {
    {
        .dev = DEVICE_DT_GET(DT_NODELABEL(bmi270)),
        .chans = bmi270_chan,
        .chan_cnt = ARRAY_SIZE(bmi270_chan),
        .sampling_period_ms = 200,
        .active_events_limit = 3,
        .event_descr = "bmi270_accel",
    },
    {
        .dev = DEVICE_DT_GET(DT_NODELABEL(bme280)),
        .chans = bme280_chans,
        .chan_cnt = ARRAY_SIZE(bme280_chans),
        .sampling_period_ms = 1000,
        .active_events_limit = 3,
        .event_descr = "bme280_env",
    }
};
