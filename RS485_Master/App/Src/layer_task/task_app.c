/**
 * @file task_app.c
 * @brief FreeRTOS 应用任务编排实现
 *
 * 该文件负责把 RS485 通信状态机、诊断输出和后续应用任务组织成明确的
 * FreeRTOS 任务集合。当前实现保持功能行为与裸机阶段一致，同时把周期调度
 * 和任务生命周期收口到一个清晰位置。
 */

#include "layer_task/task_app.h"   // 任务编排层对外接口声明。
#include "layer_task/task_rs485.h"  // RS485 主机任务入口和诊断接口。
#include "layer_task/app_config.h"  // 任务周期、优先级和栈深等配置。
#include "layer_app/rs485_console.h"  // USART2 控制台初始化入口。

#include "FreeRTOS.h"  // FreeRTOS 基础类型和宏定义。
#include "task.h"  // FreeRTOS 任务创建、延时和句柄类型。
#include "layer_driver/rs485_lock.h"  // RS485 共享数据互斥量。
#include "layer_driver/uart2_tx_lock.h"  // USART2 发送互斥量。

/* 任务句柄仅用于生命周期管理和后续扩展，不向业务层暴露。 */
static TaskHandle_t s_rs485_task_handle = 0;  // RS485 通信任务句柄。
static TaskHandle_t s_diag_task_handle = 0;  // 诊断输出任务句柄。

/*
 * @brief RS485 通信任务入口。
 *
 * 该任务只负责周期性推进 RS485 状态机，不承担底层协议细节，也不直接
 * 操作硬件寄存器。所有通信节奏通过 vTaskDelayUntil 保持稳定。
 */
static void task_app_rs485_entry(void *argument)
{
    TickType_t last_wake_tick;  // 上一次唤醒节拍，用于固定周期运行。
    uint8_t console_inited = 0U;  // 控制台初始化标志，仅执行一次。

    (void)argument;  // 当前任务不使用外部参数，显式丢弃避免告警。
    last_wake_tick = xTaskGetTickCount();  // 记录首次基准节拍，作为周期起点。

    for (;;)
    {
        /*
         * 把 USART2 接收中断挂起放到任务上下文中执行，
         * 避免在调度器启动前就触发外设中断干扰首任务切换。
         * 这里只做一次性初始化，后续循环只推进 RS485 状态机本体。
         */
        if (console_inited == 0U)  // 第一次进入任务时完成控制台初始化。
        {
            Task_RS485_ConsoleInit();  // 初始化 USART2 控制台接收路径。
            console_inited = 1U;  // 标记初始化完成，后续循环不再重复执行。
        }

        /*
         * 主循环不直接访问寄存器，只调用任务层状态机入口。
         * 这样通信节奏、超时、重试和离线恢复都收口在一个地方维护。
         */
        Task_RS485_Run();  // 推进 RS485 主机状态机一步。
        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(RS485_TASK_PERIOD_MS));  // 固定任务周期。
    }
}

/*
 * @brief 诊断输出任务入口。
 *
 * 该任务只负责低频打印当前总线健康状态，避免诊断文本占用通信任务的
 * 执行窗口。输出节拍与通信节拍分离后，运行行为更容易观察和验证。
 */
static void task_app_diag_entry(void *argument)
{
    TickType_t last_wake_tick;  // 上一次唤醒节拍，用于低频诊断输出。

    (void)argument;  // 当前任务不使用外部参数，显式丢弃避免告警。
    last_wake_tick = xTaskGetTickCount();  // 记录诊断任务自己的周期基准。

    for (;;)
    {
        /* 诊断任务只负责低频输出，不参与任何通信状态推进。 */
        Task_RS485_PrintDiag();  // 打印当前 RS485 健康状态和寄存器快照。
        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(RS485_DIAG_PERIOD_MS));  // 固定诊断节拍。
    }
}

uint8_t Task_App_Init(void)
{
    uint8_t init_ok;  // RS485 子模块初始化结果缓存。

    /*
     * 初始化 RS485 共享数据互斥量。
     * 必须在任何任务创建前调用，确保 RS485_LockEnter/Exit 可用。
     * 如果这里失败，说明底层 FreeRTOS 资源尚未准备好，后续任务没有运行基础。
     */
    if (RS485_LockInit() == 0U)  // 创建 RS485 共享资源锁失败直接退出。
    {
        /* 互斥量创建失败，系统无法继续 */
        return 0U;
    }

    /* USART2 发送路径也需要独立互斥量，避免诊断输出和控制台应答互相穿插。 */
    if (Uart2_TxLockInit() == 0U)  // 创建 USART2 发送锁失败直接退出。
    {
        return 0U;
    }

    /*
     * RS485 任务层自身的初始化由其模块负责，此处只做编排入口收口。
     * 这样任务编排层只关心“能不能启动”，不关心协议细节和寄存器布局。
     */
    init_ok = Task_RS485_Init();  // 交由任务层完成协议缓存和运行态初始化。
    if (init_ok == 0U)  // 下层初始化失败时直接向上返回失败。
    {
        return 0U;
    }

    return 1U;  // 所有初始化均成功，允许创建任务。
}

uint8_t Task_App_CreateTasks(void)
{
    BaseType_t result;  // FreeRTOS 任务创建返回值。

    /* 先创建通信任务，确保总线状态机优先获得运行机会。 */
    result = xTaskCreate(task_app_rs485_entry,
                         "RS485",
                         RS485_TASK_STACK_WORDS,
                         0,
                         RS485_TASK_PRIORITY_RS485,
                         &s_rs485_task_handle);  // 创建 RS485 主任务并保存句柄。
    if (result != pdPASS)  // 创建失败时不继续创建后续任务。
    {
        s_rs485_task_handle = 0;  // 清空句柄，避免保留无效任务标识。
        return 0U;  // 启动失败向上返回。
    }

    /*
     * 再创建诊断任务；若失败则回收前一个任务，保持启动过程可恢复。
     * 这样不会留下“只起了一半”的 FreeRTOS 运行态，排障时更可控。
     */
    result = xTaskCreate(task_app_diag_entry,
                         "DIAG",
                         RS485_DIAG_TASK_STACK_WORDS,
                         0,
                         RS485_TASK_PRIORITY_DIAG,
                         &s_diag_task_handle);  // 创建诊断任务并保存句柄。
    if (result != pdPASS)  // 如果诊断任务创建失败，回收已创建的通信任务。
    {
        if (s_rs485_task_handle != 0)  // 只在通信任务句柄有效时执行删除。
        {
            vTaskDelete(s_rs485_task_handle);  // 删除已创建的通信任务，避免半初始化运行态。
            s_rs485_task_handle = 0;  // 删除后清空句柄。
        }

        s_diag_task_handle = 0;  // 诊断任务未创建成功，句柄保持为空。
        return 0U;  // 创建任务过程失败。
    }

    return 1U;  // 两个任务均创建成功。
}
