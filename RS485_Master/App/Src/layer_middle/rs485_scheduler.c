/**
 * @file rs485_scheduler.c
 * @brief RS485 轮询调度与在线离线状态实现
 *
 * 此文件实现主机轮询调度策略，包括 Tick 回绕安全超时判断、从机通信成功/
 * 失败状态更新、离线重试判定，以及轮询索引推进逻辑。
 *
 * 主要内容：
 * - 超时比较实现
 * - 在线离线状态维护实现
 * - 轮询目标选择与索引推进实现
 */
#include "layer_middle/rs485_scheduler.h"

// 使用有符号差值比较处理 HAL Tick 回绕，保证 32 位计数溢出后仍能正确判断超时。
uint8_t RS485_SchedIsTimeout(uint32_t now, uint32_t deadline)
{
    return ((int32_t)(now - deadline) >= 0) ? 1U : 0U;
}

// 成功路径统一清除连续失败并设置在线，避免多个调用点重复维护同样状态。
void RS485_SchedMarkSlaveSuccess(uint8_t slave_idx,
                                 uint8_t slave_count,
                                 uint8_t *slave_online,
                                 uint8_t *slave_fail_streak)
{
    if ((slave_online == 0) || (slave_fail_streak == 0)) {
        return;
    }

    if (slave_idx >= slave_count) {
        return;
    }

    slave_online[slave_idx] = 1U;
    slave_fail_streak[slave_idx] = 0U;
}

// 失败路径统一处理离线门限和重试时间，确保离线恢复策略在一个模块内保持一致。
void RS485_SchedMarkSlaveFailure(uint8_t slave_idx,
                                 uint8_t slave_count,
                                 uint8_t *slave_online,
                                 uint8_t *slave_fail_streak,
                                 uint32_t *slave_retry_tick,
                                 uint32_t now,
                                 uint8_t offline_fail_threshold,
                                 uint32_t retry_interval_ms)
{
    if ((slave_online == 0) || (slave_fail_streak == 0) || (slave_retry_tick == 0)) {
        return;
    }

    if (slave_idx >= slave_count) {
        return;
    }

    if (slave_fail_streak[slave_idx] < 0xFFU) {
        slave_fail_streak[slave_idx]++;
    }

    if (slave_fail_streak[slave_idx] >= offline_fail_threshold) {
        slave_online[slave_idx] = 0U;
        slave_retry_tick[slave_idx] = now + retry_interval_ms;
    }
}

// 这里按 offset 轮询，先挑在线从机，再挑重试到期的离线从机，保持原有优先级不变。
uint8_t RS485_SchedSelectNextSlave(uint32_t now,
                                   uint8_t poll_index,
                                   uint8_t slave_count,
                                   const uint8_t *slave_online,
                                   const uint32_t *slave_retry_tick,
                                   uint8_t *out_idx)
{
    uint8_t offset;

    if ((slave_online == 0) || (slave_retry_tick == 0) || (out_idx == 0) || (slave_count == 0U)) {
        return 0U;
    }

    for (offset = 0U; offset < slave_count; offset++) {
        uint8_t idx = (uint8_t)((poll_index + offset) % slave_count);

        if (slave_online[idx] != 0U) {
            *out_idx = idx;
            return 1U;
        }

        if (RS485_SchedIsTimeout(now, slave_retry_tick[idx]) != 0U) {
            *out_idx = idx;
            return 1U;
        }
    }

    return 0U;
}

// 轮询推进统一收口到一个函数，避免主任务中散落多处索引计算。
uint8_t RS485_SchedAdvancePollIndex(uint8_t active_slave_idx,
                                    uint8_t active_probe_only,
                                    uint8_t slave_count)
{
    if ((slave_count == 0U) || (active_slave_idx >= slave_count)) {
        return 0U;
    }

    // 离线探测和在线轮询都前进到下一个从机，保留原先调度行为。
    if (active_probe_only != 0U) {
        return (uint8_t)((active_slave_idx + 1U) % slave_count);
    }

    return (uint8_t)((active_slave_idx + 1U) % slave_count);
}