#pragma once
#include <stdint.h>

/* ---- 统一的 BLE 帧格式 ----
 * 16字节：CMD(1) + PAYLOAD(14) + 预留/CRC(1)
 */
#define BLE_FRAME_LEN 16

/* 命令集（先放时间，后续可加心率、IMU等） */
enum {
    CMD_SET_TIME = 0x01, /* BCD 年月日时分秒在 [1..6] */
    CMD_GET_TIME = 0x41, /* 获取时间（请求） */
    RSP_GET_TIME = 0x42, /* 获取时间（响应） */

    /* 预留：心率、六轴等
    CMD_HR_PUSH  = 0x10,
    CMD_IMU_PUSH = 0x20,
    */
};

/* ---- 自定义 128-bit UUID（和你现在一致） ----
 * Service: 3887880F-0763-4996-B232-A1DF2F945DE0
 * RX Char: 3887880F-0763-4996-B232-A1DF2F945DE1
 * TX Char: 3887880F-0763-4996-B232-A1DF2F945DE2
 */
#define BT_UUID_TSVC_VAL \
    BT_UUID_128_ENCODE(0x3887880f, 0x0763, 0x4996, 0xb232, 0xa1df2f945de0)
#define BT_UUID_TRX_VAL  \
    BT_UUID_128_ENCODE(0x3887880f, 0x0763, 0x4996, 0xb232, 0xa1df2f945de1)
#define BT_UUID_TTX_VAL  \
    BT_UUID_128_ENCODE(0x3887880f, 0x0763, 0x4996, 0xb232, 0xa1df2f945de2)
