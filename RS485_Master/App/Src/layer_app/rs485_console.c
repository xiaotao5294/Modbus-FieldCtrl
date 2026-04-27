/**
 * @file rs485_console.c
 * @brief RS485 控制台模块实现
 *
 * 此文件实现 USART2 控制台命令链路，包括逐字节接收、整行命令拼接、命令解析
 * 与文本应答。模块采用“中断接收 + 主循环处理”结构，避免在中断中执行耗时逻辑。
 *
 * 主要内容：
 * - 串口命令字节拼接与输入过滤
 * - 控制台命令解析与应答输出
 * - USART2 接收完成回调与下一字节重挂起
 */
#include "layer_app/rs485_console.h"

#include "layer_task/task_rs485.h"
#include "layer_task/app_config.h"
#include "layer_driver/uart2_tx_lock.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>

// 控制台输出仍然使用 USART2，因此这里保留一个明确的句柄别名，避免误用业务串口。
#define RS485_DIAG_UART_HANDLE          huart2

// 命令缓存需要在模块内部独立维护，这样不会和 RS485 主状态机的收发缓冲混在一起。
static char s_console_cmd_buf[RS485_CONSOLE_CMD_MAX_LEN + 1U] = {0};
static uint8_t s_console_cmd_len = 0U;
static uint8_t s_console_rx_byte = 0U;
static volatile uint8_t s_console_line_ready = 0U;
static volatile uint8_t s_console_rx_ring[RS485_CONSOLE_RX_RING_SIZE] = {0U};
static volatile uint8_t s_console_rx_head = 0U;
static volatile uint8_t s_console_rx_tail = 0U;
static volatile uint8_t s_console_rx_overflow = 0U;

// 中断里只做字节搬运：写入环形缓冲，避免在 ISR 里做命令拼接与解析。
static void rs485_console_rx_ring_push_isr(uint8_t byte)
{
    uint8_t next_head;

    next_head = (uint8_t)((s_console_rx_head + 1U) % RS485_CONSOLE_RX_RING_SIZE);
    if (next_head == s_console_rx_tail) {
        s_console_rx_overflow = 1U;
        return;
    }

    s_console_rx_ring[s_console_rx_head] = byte;
    s_console_rx_head = next_head;
}

// 任务上下文弹出字节，用于后续拼接完整命令行。
static uint8_t rs485_console_rx_ring_pop(uint8_t *out_byte)
{
    if (out_byte == 0) {
        return 0U;
    }

    if (s_console_rx_tail == s_console_rx_head) {
        return 0U;
    }

    *out_byte = s_console_rx_ring[s_console_rx_tail];
    s_console_rx_tail = (uint8_t)((s_console_rx_tail + 1U) % RS485_CONSOLE_RX_RING_SIZE);

    return 1U;
}

// 解析前先检查范围，避免把越界值推进写请求队列。
static uint8_t rs485_is_threshold_valid(uint16_t threshold)
{
    if ((threshold < MODBUS_THRESHOLD_MIN) || (threshold > MODBUS_THRESHOLD_MAX)) {
        return 0U;
    }

    return 1U;
}

// 舵机角度写请求要在控制台入口先做边界检查，避免非法值进入主状态机。
static uint8_t rs485_is_servo_angle_valid(uint16_t angle)
{
    if ((angle < MODBUS_SERVO_ANGLE_MIN) || (angle > MODBUS_SERVO_ANGLE_MAX)) {
        return 0U;
    }

    return 1U;
}

// 这里手写十进制解析，避免引入额外标准库依赖并保留越界检查。
static uint8_t rs485_parse_u16(const char *text, uint16_t *out_value)
{
    uint32_t acc = 0U;
    uint16_t i;

    if ((text == 0) || (out_value == 0) || (text[0] == '\0')) {
        return 0U;
    }

    for (i = 0U; text[i] != '\0'; i++) {
        uint8_t ch = (uint8_t)text[i];

        if ((ch < (uint8_t)'0') || (ch > (uint8_t)'9')) {
            return 0U;
        }

        acc = (acc * 10U) + (uint32_t)(ch - (uint8_t)'0');
        if (acc > 0xFFFFU) {
            return 0U;
        }
    }

    *out_value = (uint16_t)acc;
    return 1U;
}

// 统一的串口应答输出入口，所有控制台命令都通过它回显，避免分散处理超时策略。
static void rs485_console_send_text(const char *text)
{
    uint16_t length;
    uint16_t sent = 0U;
    uint32_t start_tick;
    uint32_t lock_state;

    if (text == 0) {
        return;
    }

    length = (uint16_t)strnlen(text, 0xFFFFU);
    if (length == 0U) {
        return;
    }

    /* 先串行化 USART2 发送路径，再执行轮询发送，避免与诊断任务交错写入。 */
    lock_state = Uart2_TxLockEnter();
    if (lock_state != 0U) {
        return;
    }

    /* 发送路径采用有限预算轮询 TXE，避免阻塞主状态机。 */
    start_tick = HAL_GetTick();
    while (sent < length) {
        if (__HAL_UART_GET_FLAG(&RS485_DIAG_UART_HANDLE, UART_FLAG_TXE) != RESET) {
            RS485_DIAG_UART_HANDLE.Instance->DR = (uint16_t)((uint8_t)text[sent]);
            sent++;
            start_tick = HAL_GetTick();
            continue;
        }

        if ((HAL_GetTick() - start_tick) > RS485_DIAG_TX_BUDGET_MS) {
            break;
        }
    }

    Uart2_TxLockExit(lock_state);
}

// 只把字符拼成完整一行，不在中断路径里做命令解析，这样可以把耗时工作留到任务上下文。
static void rs485_on_console_rx_byte(uint8_t byte)
{
    if (s_console_line_ready != 0U) {
        return;
    }

    if ((byte == (uint8_t)'\r') || (byte == (uint8_t)'\n')) {
        if (s_console_cmd_len > 0U) {
            s_console_cmd_buf[s_console_cmd_len] = '\0';
            s_console_line_ready = 1U;
        }

        return;
    }

    if ((byte == 0x08U) || (byte == 0x7FU)) {
        if (s_console_cmd_len > 0U) {
            s_console_cmd_len--;
        }

        return;
    }

    if ((byte < 0x20U) || (byte > 0x7EU)) {
        return;
    }

    if (s_console_cmd_len >= RS485_CONSOLE_CMD_MAX_LEN) {
        s_console_cmd_len = 0U;
        return;
    }

    s_console_cmd_buf[s_console_cmd_len] = (char)byte;
    s_console_cmd_len++;
}

// 控制台命令只允许改写已经通过任务层校验过的目标值，这样不会绕过主循环的状态管理。
static void rs485_handle_console_command(const char *cmd)
{
    const char *actual_cmd = cmd;
    char rsp[128];
    uint16_t threshold;
    uint16_t servo_angle;
    int16_t s2_ax;
    int16_t s2_ay;
    int16_t s2_az;
    uint16_t s2_servo;

    if ((cmd == 0) || (cmd[0] == '\0')) {
        return;
    }

    // 支持 "S1 " / "S2 " 前缀，便于调试时显式标注目标从机。
    if ((strncmp(cmd, "S1 ", 3U) == 0) || (strncmp(cmd, "S2 ", 3U) == 0)) {
        actual_cmd = &cmd[3];
    }

    if (strcmp(actual_cmd, "THR?") == 0) {
        (void)snprintf(rsp,
                       sizeof(rsp),
                       "S1 THR=%u PEND=%u RANGE:%u-%u\r\n",
                       (unsigned int)Task_RS485_GetSlaveRegister(0U, 1U),
                       (unsigned int)Task_RS485_GetManualWritePending(0U),
                       (unsigned int)MODBUS_THRESHOLD_MIN,
                       (unsigned int)MODBUS_THRESHOLD_MAX);
        rs485_console_send_text(rsp);
        return;
    }

    if (strcmp(actual_cmd, "STATUS?") == 0) {
        // 读取从机2寄存器快照，按 int16 解释加速度原始值。
        s2_ax = (int16_t)Task_RS485_GetSlaveRegister(1U, 0U);
        s2_ay = (int16_t)Task_RS485_GetSlaveRegister(1U, 1U);
        s2_az = (int16_t)Task_RS485_GetSlaveRegister(1U, 2U);
        s2_servo = Task_RS485_GetSlaveRegister(1U, 3U);

        (void)snprintf(rsp,
                       sizeof(rsp),
                       "S1 ON=%u ADC=%u TH=%u LUX=%u RELAY=%u PEND=%u | S2 ON=%u AX=%d AY=%d AZ=%d SERVO=%u\r\n",
                       (unsigned int)Task_RS485_GetSlaveOnline(0U),
                       (unsigned int)Task_RS485_GetSlaveRegister(0U, 0U),
                       (unsigned int)Task_RS485_GetSlaveRegister(0U, 1U),
                       (unsigned int)Task_RS485_GetSlaveRegister(0U, 2U),
                       (unsigned int)Task_RS485_GetSlaveRegister(0U, 3U),
                       (unsigned int)Task_RS485_GetManualWritePending(0U),
                       (unsigned int)Task_RS485_GetSlaveOnline(1U),
                       (int)s2_ax,
                       (int)s2_ay,
                       (int)s2_az,
                       (unsigned int)s2_servo);
        rs485_console_send_text(rsp);
        return;
    }

    if (strcmp(actual_cmd, "HELP") == 0) {
        rs485_console_send_text("CMD: THR? | THR=<0..3300> | SERVO=<0..180> | STATUS? | HELP\r\n");
        return;
    }

    if (strncmp(actual_cmd, "SERVO=", 6U) == 0) {
        if (rs485_parse_u16(&actual_cmd[6], &servo_angle) == 0U) {
            rs485_console_send_text("ERR:INVALID_NUMBER\r\n");
            return;
        }

        if (rs485_is_servo_angle_valid(servo_angle) == 0U) {
            (void)snprintf(rsp,
                           sizeof(rsp),
                           "ERR:ANGLE_OUT_OF_RANGE(%u-%u)\r\n",
                           (unsigned int)MODBUS_SERVO_ANGLE_MIN,
                           (unsigned int)MODBUS_SERVO_ANGLE_MAX);
            rs485_console_send_text(rsp);
            return;
        }

        // 从机索引 1 对应从机2，实际发送仍由主状态机统一调度执行。
        Task_RS485_RequestManualWrite(1U, servo_angle);
        (void)snprintf(rsp, sizeof(rsp), "OK:QUEUED S2 SERVO=%u\r\n", (unsigned int)servo_angle);
        rs485_console_send_text(rsp);
        return;
    }

    if (strncmp(actual_cmd, "THR=", 4U) == 0) {
        if (rs485_parse_u16(&actual_cmd[4], &threshold) == 0U) {
            rs485_console_send_text("ERR:INVALID_NUMBER\r\n");
            return;
        }

        if (rs485_is_threshold_valid(threshold) == 0U) {
            (void)snprintf(rsp,
                           sizeof(rsp),
                           "ERR:OUT_OF_RANGE(%u-%u)\r\n",
                           (unsigned int)MODBUS_THRESHOLD_MIN,
                           (unsigned int)MODBUS_THRESHOLD_MAX);
            rs485_console_send_text(rsp);
            return;
        }

        Task_RS485_RequestManualWrite(0U, threshold);
        (void)snprintf(rsp, sizeof(rsp), "OK:QUEUED S1 THR=%u\r\n", (unsigned int)threshold);
        rs485_console_send_text(rsp);
        return;
    }

    rs485_console_send_text("ERR:UNKNOWN_CMD\r\n");
}

void Task_RS485_ConsoleInit(void)
{
    // 这里把控制台状态完全清空，避免上电残留字节被当成上一条命令继续处理。
    s_console_cmd_len = 0U;
    s_console_rx_byte = 0U;
    s_console_line_ready = 0U;
    s_console_rx_head = 0U;
    s_console_rx_tail = 0U;
    s_console_rx_overflow = 0U;
    memset(s_console_cmd_buf, 0, sizeof(s_console_cmd_buf));

    // 只挂起 1 字节接收，这样 USART2 中断负责逐字节喂给命令拼接器。
    (void)HAL_UART_Receive_IT(&RS485_DIAG_UART_HANDLE, &s_console_rx_byte, 1U);
}

void Task_RS485_ConsoleProcess(void)
{
    uint8_t byte_count = 0U;
    uint8_t rx_byte = 0U;

    // 逐轮限额消费中断缓冲，避免控制台处理抢占主通信状态机执行时间。
    while ((byte_count < RS485_CONSOLE_POLL_BUDGET) && (rs485_console_rx_ring_pop(&rx_byte) != 0U)) {
        rs485_on_console_rx_byte(rx_byte);
        byte_count++;
    }

    // 缓冲溢出只做一次文本提示，避免重复刷屏干扰调试观察。
    if (s_console_rx_overflow != 0U) {
        s_console_rx_overflow = 0U;
        rs485_console_send_text("WARN:RX_RING_OVERFLOW\r\n");
    }

    if (s_console_line_ready == 0U) {
        return;
    }

    // 这里先清标志再解析，避免如果命令处理过程中又来新字节，状态被重复消费。
    s_console_line_ready = 0U;
    rs485_handle_console_command(s_console_cmd_buf);
    s_console_cmd_len = 0U;
    memset(s_console_cmd_buf, 0, sizeof(s_console_cmd_buf));
}

// HAL 的接收完成回调仍然在这个模块内处理，这样串口输入链路和命令解析逻辑可以放在同一文件里维护。
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((huart == 0) || (huart->Instance != USART2)) {
        return;
    }

    rs485_console_rx_ring_push_isr(s_console_rx_byte);
    (void)HAL_UART_Receive_IT(&RS485_DIAG_UART_HANDLE, &s_console_rx_byte, 1U);
}