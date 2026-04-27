/**
 * @file rs485_console.h
 * @brief RS485 控制台模块接口声明
 *
 * 此文件定义 RS485 控制台模块对外接口，用于在任务初始化阶段启动 USART2
 * 命令接收，并在主循环中处理已拼接完成的控制台命令。
 *
 * 主要内容：
 * - 控制台模块初始化入口
 * - 控制台命令处理入口
 */
#ifndef APP_LAYER_APP_RS485_CONSOLE_H
#define APP_LAYER_APP_RS485_CONSOLE_H

#include <stdint.h>

// 控制台模块负责 USART2 的逐字节接收、命令拼接和文本应答。
void Task_RS485_ConsoleInit(void);

// 主任务周期性调用这个函数，把已拼好的整行命令交给解析器。
void Task_RS485_ConsoleProcess(void);

#endif
