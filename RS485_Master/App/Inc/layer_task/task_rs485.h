/**
 * @file task_rs485.h
 * @brief RS485 主机任务接口与诊断结构声明
 *
 * 此文件声明 RS485 主机任务的对外接口，包括任务初始化与轮询入口、运行时
 * 诊断读取接口、从机缓存读取接口，以及手动写阈值请求接口。
 *
 * 主要内容：
 * - RS485 运行时诊断结构体声明
 * - RS485 主任务初始化与运行接口声明
 * - 从机状态/寄存器读取接口声明
 * - 手动写阈值请求接口声明
 */
#ifndef APP_LAYER_TASK_TASK_RS485_H
#define APP_LAYER_TASK_TASK_RS485_H

#include "stm32f1xx_hal.h"
#include "layer_task/app_config.h"

/**
 * @brief 初始化 RS485 主机任务
 */
uint8_t Task_RS485_Init(void);

/**
 * @brief 运行 RS485 主机任务
 */
void Task_RS485_Run(void);

/**
 * @brief 打印诊断信息到调试串口
 */
void Task_RS485_PrintDiag(void);

/**
 * @brief 读取指定从机的指定寄存器缓存值
 * @param slave_idx 从机索引
 * @param reg_idx 寄存器索引
 * @return 寄存器缓存值；索引非法时返回 0
 */
uint16_t Task_RS485_GetSlaveRegister(uint8_t slave_idx, uint8_t reg_idx);

/**
 * @brief 读取指定从机的在线状态
 * @param slave_idx 从机索引
 * @return 1=在线，0=离线
 */
uint8_t Task_RS485_GetSlaveOnline(uint8_t slave_idx);

/**
 * @brief 读取指定从机是否存在待写请求
 * @param slave_idx 从机索引
 * @return 1=有待写请求，0=没有
 */
uint8_t Task_RS485_GetManualWritePending(uint8_t slave_idx);

/**
 * @brief 记录一次待写控制值请求
 * @param slave_idx 从机索引
 * @param control_value 待写控制值（从机1=阈值，从机2=舵机角度）
 */
void Task_RS485_RequestManualWrite(uint8_t slave_idx, uint16_t control_value);

#endif
