/**
 * @file rs485_lock.h
 * @brief RS485 共享数据临界区抽象
 *
 * 该抽象通过 FreeRTOS 二值信号量（互斥量）保护共享数据。
 * 接口设计保持向下兼容，使得上层业务代码无需修改。
 *
 * 使用流程：
 * 1. 在应用初始化时调用 RS485_LockInit() 创建互斥量
 * 2. 在临界区入口调用 RS485_LockEnter()
 * 3. 在临界区出口调用 RS485_LockExit(state)
 */
#ifndef APP_LAYER_DRIVER_RS485_LOCK_H
#define APP_LAYER_DRIVER_RS485_LOCK_H

#include <stdint.h>

/**
 * @brief 初始化 RS485 共享数据保护互斥量
 *
 * 该函数在系统启动早期调用（在调度器启动前），创建 FreeRTOS 二值信号量。
 * 必须在任何 RS485_LockEnter/Exit 调用之前被调用一次。
 *
 * @return 1 表示成功，0 表示初始化失败（互斥量创建失败）
 */
uint8_t RS485_LockInit(void);

/**
 * @brief 进入临界区
 *
 * 在 FreeRTOS 环境中：尝试获取互斥量，最多等待 100ms。
 * 返回值用于记录之前的锁状态（用于嵌套调用恢复）。
 * 在无法获取互斥量时返回特殊值，调用者应检查返回值。
 *
 * @return 锁的前一个状态值，传入 RS485_LockExit 时使用
 */
uint32_t RS485_LockEnter(void);

/**
 * @brief 离开临界区
 *
 * @param state RS485_LockEnter() 的返回值，用于恢复之前的锁状态
 */
void RS485_LockExit(uint32_t state);

#endif
