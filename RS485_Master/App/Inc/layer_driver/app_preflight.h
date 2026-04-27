/**
 * @file app_preflight.h
 * @brief FreeRTOS 改造前预检与资源管控接口
 *
 * 该模块用于在裸机阶段提前建立“可迁移到 RTOS”的基础约束：
 * 1) 统一中断优先级策略，避免后续接入调度器时优先级冲突。
 * 2) 提供外设资源锁定能力，避免重复初始化和跨模块抢占同一外设。
 * 3) 保持接口轻量，便于后续替换为 FreeRTOS 互斥量/临界区实现。
 */
#ifndef APP_LAYER_DRIVER_APP_PREFLIGHT_H
#define APP_LAYER_DRIVER_APP_PREFLIGHT_H

#include <stdint.h>

typedef enum
{
    APP_RESOURCE_USART1 = 0,
    APP_RESOURCE_USART2,
    APP_RESOURCE_DMA,
    APP_RESOURCE_TIM,
    APP_RESOURCE_ADC,
    APP_RESOURCE_COUNT
} App_ResourceId_t;

void App_PreflightInit(void);
uint8_t App_PreflightLockResource(App_ResourceId_t id);
void App_PreflightUnlockResource(App_ResourceId_t id);
uint8_t App_PreflightIsResourceLocked(App_ResourceId_t id);

#endif
