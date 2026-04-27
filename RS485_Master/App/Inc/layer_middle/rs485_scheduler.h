/**
 * @file rs485_scheduler.h
 * @brief RS485 轮询调度与在线离线状态接口声明
 *
 * 此文件声明主机轮询调度相关接口，覆盖超时判断、从机在线离线状态维护、
 * 下一个从机选择与轮询索引推进，供任务状态机按统一策略调用。
 *
 * 主要内容：
 * - Tick 超时判断接口
 * - 成功/失败状态维护接口
 * - 轮询选择与索引推进接口
 */
#ifndef APP_LAYER_MIDDLE_RS485_SCHEDULER_H
#define APP_LAYER_MIDDLE_RS485_SCHEDULER_H

#include <stdint.h>

// 为了避免状态机模块依赖业务文件，这里定义一个通用的无效索引常量。
#define RS485_SCHED_INVALID_INDEX       0xFFU

// 统一的 Tick 回绕安全超时判断，供轮询调度与状态机复用。
uint8_t RS485_SchedIsTimeout(uint32_t now, uint32_t deadline);

// 通信成功后更新在线和失败计数，避免主任务重复写同样逻辑。
void RS485_SchedMarkSlaveSuccess(uint8_t slave_idx,
                                 uint8_t slave_count,
                                 uint8_t *slave_online,
                                 uint8_t *slave_fail_streak);

// 通信失败后更新连续失败和离线重试时刻，保证离线判定逻辑在单点维护。
void RS485_SchedMarkSlaveFailure(uint8_t slave_idx,
                                 uint8_t slave_count,
                                 uint8_t *slave_online,
                                 uint8_t *slave_fail_streak,
                                 uint32_t *slave_retry_tick,
                                 uint32_t now,
                                 uint8_t offline_fail_threshold,
                                 uint32_t retry_interval_ms);

// 从当前轮询索引出发，优先在线从机，其次离线到期从机。
uint8_t RS485_SchedSelectNextSlave(uint32_t now,
                                   uint8_t poll_index,
                                   uint8_t slave_count,
                                   const uint8_t *slave_online,
                                   const uint32_t *slave_retry_tick,
                                   uint8_t *out_idx);

// 事务结束后推进下一个轮询索引，避免主文件散落索引切换规则。
uint8_t RS485_SchedAdvancePollIndex(uint8_t active_slave_idx,
                                    uint8_t active_probe_only,
                                    uint8_t slave_count);

#endif
