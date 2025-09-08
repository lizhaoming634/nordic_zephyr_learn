/* time_bus.h */
#ifndef TIME_BUS_H
#define TIME_BUS_H

#include <zephyr/zbus/zbus.h>



struct time_model {
    int64_t epoch;
};

/* 声明时间更新信道（具体定义在 time_bus.c） */
ZBUS_CHAN_DECLARE(time_chan);

#endif /* TIME_BUS_H */
