#ifndef KEY_EBTN_H_
#define KEY_EBTN_H_

/**
 * @file key_ebtn.h
 * @brief easy_button 线程轮询版按键处理的配置与对外接口
 *
 * 使用说明：
 *  - 这些宏都有默认值；你可以在 CMake/编译命令用 -D 覆盖，
 *    或在包含本头文件之前 #define 它们来自定义参数。
 *  - 重要关系：KEY_KEEPALIVE_MS 必须明显大于 KEY_CLICK_MAX_MS，
 *    否则短按也可能触发 KEEPALIVE（像“长按”）。
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 用户可调参数（均可 -D 覆盖） ================= */

/* 去抖（按下/松开独立设置） */
#ifndef KEY_DEBOUNCE_PRESS_MS
#define KEY_DEBOUNCE_PRESS_MS     30
#endif
#ifndef KEY_DEBOUNCE_RELEASE_MS
#define KEY_DEBOUNCE_RELEASE_MS   30
#endif

/* “点击”判定的按住时间窗口 */
#ifndef KEY_CLICK_MIN_MS
#define KEY_CLICK_MIN_MS          30
#endif
#ifndef KEY_CLICK_MAX_MS
#define KEY_CLICK_MAX_MS         280   /* < KEEPALIVE 更稳 */
#endif

/* 多击（双击）窗口上限；单击会等该窗口关上才上报 */
#ifndef KEY_MULTI_GAP_MS
#define KEY_MULTI_GAP_MS         300
#endif

/* 长按 KEEPALIVE 周期（第一帧 keepalive 视为长按成立） */
#ifndef KEY_KEEPALIVE_MS
#define KEY_KEEPALIVE_MS         800   /* > CLICK_MAX_MS */
#endif

/* 允许的最大连击数（2=支持到双击并在第二次点击后立刻结算） */
#ifndef KEY_MAX_CONSEC
#define KEY_MAX_CONSEC            2
#endif

/* 轮询周期（线程每隔多少毫秒调用一次 ebtn_process） */
#ifndef KEY_POLL_MS
#define KEY_POLL_MS               5    /* 5–10ms 常用 */
#endif

/* 线程配置（可按需调小/调高栈或优先级） */
#ifndef KEY_THREAD_STACK
#define KEY_THREAD_STACK        1024
#endif
#ifndef KEY_THREAD_PRIO
#define KEY_THREAD_PRIO           10   /* 数字越大优先级越低 */
#endif

/* ================= 对外接口 ================= */

/**
 * @brief 初始化并启动按键处理线程（无需中断）
 * 依赖：
 *  - Devicetree: alias sw0 指向你的按键
 *  - easy_button 源码：ebtn.c / ebtn.h / bit_array.h
 *  - 你的背光模块：backlight_ctrl.h（提供 blctl_is_awake()/blctl_wake()）
 */
int key_ebtn_thread_init(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_EBTN_H_ */
