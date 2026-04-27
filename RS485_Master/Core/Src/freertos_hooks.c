/**
 * @file freertos_hooks.c
 * @brief FreeRTOS 运行时钩子实现
 *
 * 该文件集中处理 FreeRTOS 的内存分配失败和栈溢出回调，保证当系统资源
 * 不足时进入明确、可观测、可恢复的错误路径，而不是静默失效。
 */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

#define APP_FAULT_CODE_ASSERT        0xA55E0001UL
#define APP_FAULT_CODE_MALLOC_FAIL   0xA55E0002UL
#define APP_FAULT_CODE_STACK_OVER    0xA55E0003UL

/* 统一故障码：调试器可直接读取，快速判断停机来源。 */
volatile uint32_t s_app_fault_code = 0U;

void App_FaultPanic(uint32_t code)
{
    s_app_fault_code = code;

    /* LED 置位，给现场一个明确故障信号。 */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

/*
 * @brief 内存分配失败钩子。
 *
 * 当 FreeRTOS 堆空间不足时，系统必须立即进入故障态，防止后续任务继续在
 * 不完整资源环境下运行。
 */
void vApplicationMallocFailedHook(void)
{
    App_FaultPanic(APP_FAULT_CODE_MALLOC_FAIL);
}

/*
 * @brief 栈溢出钩子。
 *
 * 该钩子用于捕捉任务栈越界，避免栈损坏继续扩散到其他任务或内核数据结构。
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    App_FaultPanic(APP_FAULT_CODE_STACK_OVER);
}
