#pragma once
#include <stdint.h>



/* 初始化 BMI270：
 * - 绑定 I2C（来自 DT 的 node-label: bmi270）
 * - bmi270_init()
 * - 调整 ACC 工作点（更稳：ODR 50Hz/100Hz、窄带宽、±4g）
 * - 启用 Step Detector + Wrist Gesture 并映射到 INT1（高电平有效）
 */
int bmi270_steps_init(void);

/* 读取 BMI270 的“特性中断状态位” */
int bmi270_steps_get_int_status(uint16_t *int_status);

/* 读取 Wrist Gesture 的手势输出（如 pivot_up=2）。
 * 返回 0 表示成功，*gesture 为手势编码。
 */
int bmi270_read_wrist_gesture(uint8_t *gesture);


