/**
 * @file task_app.h
 * @brief FreeRTOS 应用任务编排接口
 *
 * 该模块负责在 FreeRTOS 启动前创建应用任务，并将 RS485 通信任务与诊断
 * 任务的生命周期收口到一个清晰的任务编排入口。
 */
#ifndef APP_LAYER_TASK_TASK_APP_H
#define APP_LAYER_TASK_TASK_APP_H

#include <stdint.h>

uint8_t Task_App_Init(void);
uint8_t Task_App_CreateTasks(void);

#endif
