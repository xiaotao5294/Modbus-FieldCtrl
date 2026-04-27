/**
 * @file uart2_tx_lock.c
 * @brief USART2 发送路径互斥保护实现
 *
 * 该实现使用 FreeRTOS 互斥量串行化 USART2 的全部发送操作，保证诊断任务
 * 和控制台应答不会在同一 UART 上并发写入。这样可以避免两类典型问题：
 * - 文本输出被其他任务插入，变成不可读的乱码
 * - 短预算发送在竞争下提前超时，导致“看起来没有任何响应”
 */

#include "layer_driver/uart2_tx_lock.h"

#include "FreeRTOS.h"
#include "semphr.h"

/* USART2 发送互斥量只用于串口输出，不参与 RS485 业务数据保护。 */
static SemaphoreHandle_t s_uart2_tx_mutex = NULL;

/* 互斥量初始化状态：0=未初始化，1=成功，2=失败。 */
static uint8_t s_uart2_tx_lock_ready = 0U;

uint8_t Uart2_TxLockInit(void)
{
    if (s_uart2_tx_lock_ready != 0U) {
        return (s_uart2_tx_lock_ready == 1U) ? 1U : 0U;
    }

    s_uart2_tx_mutex = xSemaphoreCreateMutex();
    if (s_uart2_tx_mutex == NULL) {
        s_uart2_tx_lock_ready = 2U;
        return 0U;
    }

    s_uart2_tx_lock_ready = 1U;
    return 1U;
}

uint32_t Uart2_TxLockEnter(void)
{
    if (s_uart2_tx_lock_ready != 1U) {
        return 0xFFFFFFFFU;
    }

    if (xSemaphoreTake(s_uart2_tx_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
        return 0U;
    }

    return 0xFFFFFFFFU;
}

void Uart2_TxLockExit(uint32_t state)
{
    if ((state == 0U) && (s_uart2_tx_lock_ready == 1U) && (s_uart2_tx_mutex != NULL)) {
        xSemaphoreGive(s_uart2_tx_mutex);
    }
}