#ifndef BACKLIGHT_CTRL_H_
#define BACKLIGHT_CTRL_H_

/**
 * @file backlight_ctrl.h
 * @brief 屏幕亮灭与PWM背光控制（带自动熄灭、亮度设置、可持久化）
 *
 * 依赖：
 *  - Devicetree: aliases { bl = &backlight_pwm; }，chosen: zephyr,display
 *  - Kconfig/prj.conf: CONFIG_GPIO, CONFIG_PWM, CONFIG_DISPLAY, CONFIG_LOG
 *    可选持久化：CONFIG_FLASH, CONFIG_NVS, CONFIG_SETTINGS, CONFIG_SETTINGS_NVS
 *
 * 说明：
 *  - 超时=0 表示“永不自动熄灭”
 *  - persist=true 时写入 settings（需启用 CONFIG_SETTINGS_NVS）
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化背光/屏幕控制模块 */
int      blctl_init(void);

/** 点亮屏幕并开启背光；若配置了超时，会自动熄灭 */
void     blctl_wake(void);

/** 立即熄灭背光并 blank 屏幕（取消自动熄灭计时） */
void     blctl_blank(void);

/** 当前是否处于“已亮屏”状态 */
bool     blctl_is_awake(void);

/** 设置自动熄灭时间（秒）；0=永不；persist=true 写入 settings */
int      blctl_set_timeout(uint32_t sec, bool persist);

/** 获取当前自动熄灭时间（秒） */
uint32_t blctl_get_timeout(void);

/** 设置亮度（0~100%）；persist=true 写入 settings；若已亮屏立即生效 */
int      blctl_set_brightness(uint8_t pct, bool persist);

/** 获取当前亮度（0~100%） */
uint8_t  blctl_get_brightness(void);

#ifdef __cplusplus
}
#endif

#endif /* BACKLIGHT_CTRL_H_ */
