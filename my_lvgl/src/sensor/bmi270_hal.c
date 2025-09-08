#include "bmi270_hal.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(steps_hal, LOG_LEVEL_INF);

#include "bmi2.h"
#include "bmi270.h"
#include "common.h"

/* 覆盖/按需修改：左/右手；ACC 工作点 */
#ifndef STEPS_WRIST_ARM
#define STEPS_WRIST_ARM           BMI2_ARM_LEFT      /* 右手：BMI2_ARM_RIGHT */
#endif
#ifndef STEPS_ACC_ODR
#define STEPS_ACC_ODR             BMI2_ACC_ODR_50HZ  /* 抬腕不够灵敏可改 BMI2_ACC_ODR_100HZ */
#endif
#ifndef STEPS_ACC_BWP
#define STEPS_ACC_BWP             BMI2_ACC_NORMAL_AVG4 /* 仍偏敏可试 BMI2_ACC_CIC_AVG8 / BMI2_ACC_RES_AVG16 */
#endif
#ifndef STEPS_ACC_RANGE
#define STEPS_ACC_RANGE           BMI2_ACC_RANGE_4G
#endif
#ifndef STEPS_ACC_FILTER_PERF
#define STEPS_ACC_FILTER_PERF     BMI2_PERF_OPT_MODE
#endif

/* 从 DT 取 I2C 和 INT1 所在节点（bmi270@69） */
#define BMI270_NODE DT_NODELABEL(bmi270)
#if !DT_NODE_HAS_STATUS(BMI270_NODE, okay)
#error "BMI270 node not found or not okay in devicetree"
#endif
static const struct i2c_dt_spec s_i2c = I2C_DT_SPEC_GET(BMI270_NODE);

/* 全局 BMI2 设备对象（本 HAL 内部使用） */
static struct bmi2_dev s_bmi270_dev;

/* --- BMI2 适配：延时 + I2C 读写 --- */
static void bmi2_delay_us(uint32_t period, void *intf_ptr)
{
    ARG_UNUSED(intf_ptr);
    k_busy_wait(period);
}

static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    const struct i2c_dt_spec *bus = (const struct i2c_dt_spec *)intf_ptr;
    int ret = i2c_burst_read_dt(bus, reg_addr, reg_data, length);
    return (ret == 0) ? BMI2_OK : BMI2_E_COM_FAIL;
}

static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    const struct i2c_dt_spec *bus = (const struct i2c_dt_spec *)intf_ptr;
    int ret = i2c_burst_write_dt(bus, reg_addr, reg_data, length);
    return (ret == 0) ? BMI2_OK : BMI2_E_COM_FAIL;
}

/* 配置 INT1：高电平、推挽、输出使能、非锁存；并把给定特性映射到 INT1 */
static int map_feature_to_int1(uint8_t feature_type)
{
    int8_t rslt;
    struct bmi2_int_pin_config pin = {0};
    struct bmi2_sens_int_config map = { .type = feature_type, .hw_int_pin = BMI2_INT1 };

    rslt = bmi2_get_int_pin_config(&pin, &s_bmi270_dev);
    if (rslt != BMI2_OK) return rslt;

    pin.pin_type                 = BMI2_INT1;
    pin.pin_cfg[0].lvl           = BMI2_INT_ACTIVE_HIGH;
    pin.pin_cfg[0].od            = BMI2_INT_PUSH_PULL;
    pin.pin_cfg[0].output_en     = BMI2_INT_OUTPUT_ENABLE;
    pin.pin_cfg[0].input_en      = BMI2_INT_INPUT_DISABLE;
    pin.int_latch                = BMI2_INT_NON_LATCH;

    rslt = bmi2_set_int_pin_config(&pin, &s_bmi270_dev);
    if (rslt != BMI2_OK) return rslt;

    return bmi270_map_feat_int(&map, 1, &s_bmi270_dev);
}

int bmi270_steps_init(void)
{
    if (!device_is_ready(s_i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    memset(&s_bmi270_dev, 0, sizeof(s_bmi270_dev));
    s_bmi270_dev.intf      = BMI2_I2C_INTF;
    s_bmi270_dev.read      = bmi2_i2c_read;
    s_bmi270_dev.write     = bmi2_i2c_write;
    s_bmi270_dev.delay_us  = bmi2_delay_us;
    s_bmi270_dev.intf_ptr  = (void *)&s_i2c;

    /* 1) 初始化 BMI270（加载配置固件等） */
    if (bmi270_init(&s_bmi270_dev) != BMI2_OK) {
        return -EIO;
    }

    /* 2) 调整 ACC 工作点（更稳） */
    {
        struct bmi2_sens_config acc = { .type = BMI2_ACCEL };
        if (bmi2_get_sensor_config(&acc, 1, &s_bmi270_dev) == BMI2_OK) {
            acc.cfg.acc.odr         = STEPS_ACC_ODR;
            acc.cfg.acc.bwp         = STEPS_ACC_BWP;
            acc.cfg.acc.filter_perf = STEPS_ACC_FILTER_PERF;
            acc.cfg.acc.range       = STEPS_ACC_RANGE;
            (void)bmi2_set_sensor_config(&acc, 1, &s_bmi270_dev);
        }
    }

    /* 3) 使能 ACC + Step Detector，并映射到 INT1 */
    {
        uint8_t sens[] = { BMI2_ACCEL, BMI2_STEP_DETECTOR };
        (void)bmi270_sensor_enable(sens, 2, &s_bmi270_dev);
        if (map_feature_to_int1(BMI2_STEP_DETECTOR) != BMI2_OK) return -EIO;
    }

    /* 4) 使能 Wrist Gesture，并映射到 INT1；设置左/右手 */
    {
        uint8_t sens[] = { BMI2_ACCEL, BMI2_WRIST_GESTURE };
        (void)bmi270_sensor_enable(sens, 2, &s_bmi270_dev);

        struct bmi2_sens_config cfg = { .type = BMI2_WRIST_GESTURE };
        if (bmi270_get_sensor_config(&cfg, 1, &s_bmi270_dev) == BMI2_OK) {
            cfg.cfg.wrist_gest.wearable_arm = STEPS_WRIST_ARM;
            (void)bmi270_set_sensor_config(&cfg, 1, &s_bmi270_dev);
        }

        if (map_feature_to_int1(BMI2_WRIST_GESTURE) != BMI2_OK) return -EIO;
    }

    LOG_INF("BMI270 ready: StepDetector + WristGesture on INT1");
    return 0;
}

/* 轻重试 + 静音：避免偶发 E_COM_FAIL 刷屏 */
int bmi270_steps_get_int_status(uint16_t *int_status)
{
    int8_t rslt;
    for (int i = 0; i < 3; ++i) {
        rslt = bmi2_get_int_status(int_status, &s_bmi270_dev);
        if (rslt == BMI2_OK) return 0;
        if (rslt == BMI2_E_COM_FAIL) { k_busy_wait(200); continue; }
        break;
    }
    return -EIO;
}

int bmi270_read_wrist_gesture(uint8_t *gesture)
{
    struct bmi2_feat_sensor_data d = { .type = BMI2_WRIST_GESTURE };
    if (bmi270_get_feature_data(&d, 1, &s_bmi270_dev) != BMI2_OK) return -EIO;
    *gesture = d.sens_data.wrist_gesture_output;
    return 0;
}
