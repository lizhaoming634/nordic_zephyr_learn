#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/* 初始化蓝牙、注册连接回调、开始广播（包含设备名与服务UUID） */
int ble_comm_init(void);

/* 手动重启广播（断开后会自动重启，必要时可手动调用） */
int ble_comm_advertise_restart(void);

#ifdef __cplusplus
}
#endif
