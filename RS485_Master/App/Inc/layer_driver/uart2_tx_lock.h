/**
 * @file uart2_tx_lock.h
 * @brief USART2 发送路径互斥保护接口
 *
 * 该模块只负责保护 USART2 的串口发送路径，避免诊断任务和控制台应答
 * 在 FreeRTOS 下并发写同一个 UART 时出现字节交错、输出截断或假死。
 *
 * 使用流程：
 * 1. 在系统初始化阶段调用 Uart2_TxLockInit()
 * 2. 在任意 USART2 发送前调用 Uart2_TxLockEnter()
 * 3. 在发送完成后调用 Uart2_TxLockExit(state)
 */
#ifndef APP_LAYER_DRIVER_UART2_TX_LOCK_H
#define APP_LAYER_DRIVER_UART2_TX_LOCK_H

#include <stdint.h>

/**
 * @brief 初始化 USART2 发送互斥量
 * @return 1 表示成功，0 表示失败
 */
uint8_t Uart2_TxLockInit(void);

/**
 * @brief 进入 USART2 发送临界区
 * @return 0 表示成功持有锁，非 0 表示获取失败
 */
uint32_t Uart2_TxLockEnter(void);

/**
 * @brief 离开 USART2 发送临界区
 * @param state Uart2_TxLockEnter() 的返回值
 */
void Uart2_TxLockExit(uint32_t state);

#endif