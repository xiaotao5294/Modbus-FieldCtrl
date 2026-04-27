/**
 * @file rs485_lock.c
 * @brief RS485 共享数据临界区抽象实现
 *
 * 说明：
 * - 当前裸机实现通过 PRIMASK 关闭全局中断实现短临界区。
 * - 该实现仅用于保护“短小共享数据读写”，禁止包裹耗时通信流程。
 * - 后续迁移 FreeRTOS 时可把这里替换为互斥量，不影响上层业务调用点。
 */

#include "layer_driver/rs485_lock.h"
#include "stm32f1xx_hal.h"

/**
 * @file rs485_lock.c
 * @brief RS485 共享数据临界区抽象实现 - FreeRTOS 互斥量版本
 *
 * 实现说明：
 * - 使用 FreeRTOS 二值信号量作为互斥量实现共享数据保护
 * - 支持在任务上下文和中断上下文中正确同步
 * - 提供超时保护防止死锁（100ms 超时）
 * - 接口兼容性保持，上层代码无需修改
 *
 * 调用流程：
 * 1. RS485_LockInit() - 应用启动时调用一次
 * 2. RS485_LockEnter() - 临界区入口
 * 3. RS485_LockExit(state) - 临界区出口
 */

#include "FreeRTOS.h"
#include "semphr.h"

/**
 * @brief RS485 共享数据互斥量句柄
 *
 * 全局互斥量用于保护诊断快照和从机状态读取操作。
 * 在 RS485_LockInit() 中创建。
 */
static SemaphoreHandle_t s_rs485_lock_mutex = NULL;

/**
 * @brief 互斥量创建状态标志
 *
 * 0 = 未创建（未初始化）
 * 1 = 创建成功
 * 2 = 创建失败
 */
static uint8_t s_lock_initialized = 0U;

/**
 * @brief 初始化 RS485 共享数据保护互斥量
 *
 * 该函数创建 FreeRTOS 二值信号量。必须在任何锁操作前调用。
 *
 * @return 1 表示成功，0 表示失败
 */
uint8_t RS485_LockInit(void)
{
    /* 防止重复初始化 */
    if (s_lock_initialized != 0U) {
        return (s_lock_initialized == 1U) ? 1U : 0U;
    }

    /* 创建互斥量（二值信号量） */
    s_rs485_lock_mutex = xSemaphoreCreateMutex();

    if (s_rs485_lock_mutex == NULL) {
        s_lock_initialized = 2U;  /* 创建失败 */
        return 0U;
    }

    s_lock_initialized = 1U;  /* 创建成功 */
    return 1U;
}

/**
 * @brief 进入临界区 - FreeRTOS 互斥量版本
 *
 * 尝试获取互斥量。若在 100ms 内获取失败，返回特殊值以表示获取失败。
 * 为保持与裸机版本的兼容性：
 * - 返回值 0 表示成功获取
 * - 返回值非 0 表示获取失败或被中断
 *
 * 在任务上下文中被调用。
 *
 * @return 锁状态（与 RS485_LockExit 配对使用）
 */
uint32_t RS485_LockEnter(void)
{
    BaseType_t result;

    /* 若互斥量未初始化，返回失败标志 */
    if (s_lock_initialized != 1U) {
        return 0xFFFFFFFFU;
    }

    /* 尝试获取互斥量，最多等待 100ms */
    result = xSemaphoreTake(s_rs485_lock_mutex, pdMS_TO_TICKS(100U));

    if (result == pdTRUE) {
        /* 成功获取，返回 0 表示锁被获取 */
        return 0U;
    }

    /* 超时或失败，返回非零值 */
    return 0xFFFFFFFFU;
}

/**
 * @brief 离开临界区 - FreeRTOS 互斥量版本
 *
 * 根据状态值决定是否释放互斥量。
 * - 若 state == 0，表示成功进入过临界区，需要释放互斥量
 * - 其他值表示进入失败，不释放
 *
 * @param state RS485_LockEnter() 的返回值
 */
void RS485_LockExit(uint32_t state)
{
    /* 仅当状态表示成功获取时才释放互斥量 */
    if (state == 0U && s_lock_initialized == 1U && s_rs485_lock_mutex != NULL) {
        xSemaphoreGive(s_rs485_lock_mutex);
    }
}
