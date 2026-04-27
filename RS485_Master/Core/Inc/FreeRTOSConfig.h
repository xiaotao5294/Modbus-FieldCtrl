/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS 内核配置文件
 *
 * 该文件为主机工程提供 FreeRTOS 内核参数、优先级映射、内存分配策略和
 * 调试 hook 开关。配置重点是与 STM32F103 HAL 共存，并保持 RS485 通信任务
 * 的确定性与可调试性。
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stddef.h>
#include "stm32f1xx.h"

/*--------------------------- Core kernel options ---------------------------*/
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      1000U
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ((uint16_t)128U)
#define configTOTAL_HEAP_SIZE                   ((size_t)(8U * 1024U))
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               8
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configUSE_TIMERS                        0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_TRACE_FACILITY                1
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/*----------------------- Memory allocation/trace macros --------------------*/
#define configAPPLICATION_ALLOCATED_HEAP        0
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/*--------------------------- Hook and debug options ------------------------*/
void App_FaultPanic(uint32_t code);
#define APP_FAULT_CODE_ASSERT                  0xA55E0001UL
#define configASSERT(x)                        if ((x) == 0) { App_FaultPanic(APP_FAULT_CODE_ASSERT); }
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                     1
#define INCLUDE_xTaskGetSchedulerState         1
#define INCLUDE_xTaskGetTickCount              1
#define INCLUDE_xTaskGetCurrentTaskHandle      1
#define INCLUDE_xTaskPrioritySet               1
#define INCLUDE_uxTaskPriorityGet              1
#define INCLUDE_vTaskSuspend                   1
#define INCLUDE_vTaskSuspendAll                1
#define INCLUDE_xTaskResumeAll                 1
#define INCLUDE_pcTaskGetName                  1
#define INCLUDE_xTaskGetHandle                 1
#define INCLUDE_eTaskGetState                 1

/*----------------------- Cortex-M priority configuration -------------------*/
#define configPRIO_BITS                         __NVIC_PRIO_BITS
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS) )

/*
 * FreeRTOS 端口异常处理函数直接绑定到 Cortex-M 异常向量名，
 * 避免在 stm32f1xx_it.c 中再包一层 C 调用导致异常返回现场被破坏。
 */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler

/*------------------------------ API coverage --------------------------------*/
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_uxTaskGetStackHighWaterMark2    1
#define INCLUDE_xTaskGetTickCount               1

/*--------------------------- HAL integration notes -------------------------*/
/*
 * HAL 和 FreeRTOS 共用 SysTick：
 * - HAL 仍使用 HAL_IncTick() 维护 uwTick。
 * - FreeRTOS 通过 xPortSysTickHandler() 推进内核节拍。
 * - SysTick 优先级保持最低，避免打断临界区。
 */

#endif /* FREERTOS_CONFIG_H */
