/**
 * @file app_preflight.c
 * @brief FreeRTOS 改造前预检与资源管控实现
 *
 * 关键职责：
 * - 统一中断优先级分组与关键中断优先级，避免后续引入 RTOS 时出现抢占关系失控。
 * - 提供轻量资源锁位图，保证外设初始化和占用关系可检测、可拒绝。
 */

#include "layer_driver/app_preflight.h"
#include "stm32f1xx_hal.h"

/* 资源位图：bit=1 表示资源已被锁定。 */
static volatile uint32_t s_resource_bitmap = 0U;

/* 预检初始化只允许执行一次，避免重复写 NVIC 配置。 */
static uint8_t s_preflight_inited = 0U;

/* 进入临界区：使用 PRIMASK 保存中断使能状态，保证位图改写原子性。 */
static uint32_t app_preflight_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();

    return primask;
}

/* 退出临界区：按进入前状态恢复全局中断。 */
static void app_preflight_exit_critical(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

/* 统一配置关键中断优先级：采用 Group4（4bit 抢占优先级，0bit 子优先级）。 */
static void app_preflight_configure_irq_priorities(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* 为后续 FreeRTOS 预留：系统节拍和 PendSV 保持最低优先级。 */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15U, 0U);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15U, 0U);
    /* SVC 必须保持最高优先级，避免首任务启动上下文恢复过程被中断打断。 */
    HAL_NVIC_SetPriority(SVCall_IRQn, 0U, 0U);

    /* 当前项目关键外设中断统一放在可管理等级，减少抢占抖动。 */
    HAL_NVIC_SetPriority(USART1_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(USART2_IRQn, 6U, 0U);
}

/* 预检初始化接口：在 main.c 的系统时钟配置前调用，完成中断优先级分组和关键中断优先级设置。 */
void App_PreflightInit(void)
{
    uint32_t primask;

    primask = app_preflight_enter_critical();         // 进入临界区，保护位图操作和初始化过程。

    if (s_preflight_inited == 0U)
    {
        app_preflight_configure_irq_priorities();
        s_preflight_inited = 1U;
    }

    app_preflight_exit_critical(primask);            // 退出临界区，恢复中断状态。
}

/* 资源锁接口：提供锁定、解锁和查询功能，基于位图实现，适合少量资源的静态管理。 */
uint8_t App_PreflightLockResource(App_ResourceId_t id)
{
    uint32_t primask;
    uint32_t mask;

    if (id >= APP_RESOURCE_COUNT)
    {
        return 0U;
    }

    mask = (1UL << (uint32_t)id);
    primask = app_preflight_enter_critical();

    if ((s_resource_bitmap & mask) != 0U)
    {
        app_preflight_exit_critical(primask);
        return 0U;
    }

    s_resource_bitmap |= mask;
    app_preflight_exit_critical(primask);

    return 1U;
}

void App_PreflightUnlockResource(App_ResourceId_t id)
{
    uint32_t primask;
    uint32_t mask;

    if (id >= APP_RESOURCE_COUNT)
    {
        return;
    }

    mask = (1UL << (uint32_t)id);
    primask = app_preflight_enter_critical();
    s_resource_bitmap &= (~mask);
    app_preflight_exit_critical(primask);
}

uint8_t App_PreflightIsResourceLocked(App_ResourceId_t id)
{
    uint32_t mask;

    if (id >= APP_RESOURCE_COUNT)
    {
        return 0U;
    }

    mask = (1UL << (uint32_t)id);

    return ((s_resource_bitmap & mask) != 0U) ? 1U : 0U;
}
