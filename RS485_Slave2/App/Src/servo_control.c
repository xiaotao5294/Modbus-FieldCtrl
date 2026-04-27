/**
 * @file servo_control.c
 * @brief SG90 舵机控制模块实现
 *
 * 此文件实现 SG90 舵机控制逻辑，基于 TIM3 PWM 输出，将角度线性映射
 * 到目标脉宽，并在参数越界时执行钳位保护。
 *
 * 主要内容：
 * - 舵机角度与脉宽映射实现
 * - 舵机 PWM 输出更新实现
 * - 舵机状态缓存与接口实现
 *
 * @note 此模块依赖 HAL TIM 驱动，确保 TIM3 PWM 已正确配置。
 * @note 角度范围 0-180 度，对应脉宽 500-2500us，周期 20ms。
 * @note 角度越界时自动钳位到有效范围，避免舵机损坏。
 */

#include "servo_control.h"

#include "app_config.h"

// PWM 所属定时器句柄，由上层初始化后注入。
// 用于控制 PWM 输出。
static TIM_HandleTypeDef *s_servo_htim = NULL;
// PWM 输出通道，当前规划为 TIM3_CH1。
// 指定 PWM 通道号。
static uint32_t s_servo_channel = 0U;
// PWM 是否已经成功启动，用于区分“定时器句柄已注入”与“PWM 已可写入”两种状态。
// 该状态只在 HAL_TIM_PWM_Start 返回成功后置位，避免未启动时误写比较寄存器。
static uint8_t s_servo_pwm_ready = 0U;
// 当前舵机目标角度缓存，供 Modbus 读寄存器返回。
// 单位：度，范围 0-180。
static uint16_t s_servo_angle_deg = SERVO_ANGLE_DEFAULT;

// 角度到脉宽线性映射：0~180 度对应 500~2500us。
// 使用线性插值公式计算脉宽，避免查表节省内存。
// @param angle_deg 输入角度，uint16_t 类型。
// @return 对应的脉宽，uint16_t 类型，单位微秒。
static uint16_t servo_control_angle_to_pulse_us(uint16_t angle_deg)
{
    // 使用 32 位中间量避免乘法溢出。
    uint32_t pulse_us;

    // 线性插值公式：min + (range * angle / max_angle)。
    pulse_us = SERVO_PULSE_MIN_US +
               ((uint32_t)(SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US) * angle_deg) / SERVO_ANGLE_MAX;

    // 理论上不会超出 16 位，但仍做上限保护以增强鲁棒性。
    if (pulse_us > 0xFFFFU)
    {
        pulse_us = 0xFFFFU;
    }

    return (uint16_t)pulse_us;
}

// 校验角度是否在有效范围内。
// 用于输入验证，防止无效角度驱动舵机。
// @param angle_deg 待校验角度。
// @return true 表示有效，false 表示无效。
bool servo_control_is_angle_valid(uint16_t angle_deg)
{
    // 非法角度直接返回 false，供上层产生 Modbus 异常响应。
    if ((angle_deg < SERVO_ANGLE_MIN) || (angle_deg > SERVO_ANGLE_MAX))
    {
        return false;
    }

    return true;
}

// 初始化舵机控制模块。
// 注入定时器句柄和通道，启动 PWM 输出，定位到默认角度。
// 如果底层 PWM 启动失败，仍然保留软件角度缓存，但不允许后续写硬件比较值。
// @param htim 已初始化的 TIM 句柄指针。
// @param channel PWM 通道号。
void servo_control_init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    // 记录定时器句柄、通道和默认角度。
    s_servo_htim = htim;
    s_servo_channel = channel;
    s_servo_pwm_ready = 0U;
    s_servo_angle_deg = SERVO_ANGLE_DEFAULT;

    // 这里只检查句柄是否为空，不能用 channel == 0U 作为非法判断。
    // HAL 中 TIM_CHANNEL_1 的宏值本身就是 0，若在此处拦截会导致正常初始化直接返回。
    if (s_servo_htim == NULL)
    {
        return;
    }

    // 启动 PWM 输出，并把舵机定位到默认角度。
    // 只有当 HAL 明确返回成功时，才允许后续 set_angle 直接写比较寄存器。
    if (HAL_TIM_PWM_Start(s_servo_htim, s_servo_channel) == HAL_OK)
    {
        s_servo_pwm_ready = 1U;
    }

    // 无论 PWM 是否启动成功，都先写入一次目标比较值。
    // 若 PWM 未真正 ready，HAL 层不会马上输出，但缓存值会在后续成功后生效。
    __HAL_TIM_SET_COMPARE(s_servo_htim,
                          s_servo_channel,
                          servo_control_angle_to_pulse_us(s_servo_angle_deg));
}

// 设置舵机目标角度。
// 更新角度缓存，计算脉宽，驱动 PWM 输出。
// 角度越界时自动钳位。
// @param angle_deg 目标角度，uint16_t 类型。
void servo_control_set_angle(uint16_t angle_deg)
{
    // 目标脉宽（微秒）。
    uint16_t pulse_us;

    // 入参越界时执行钳位，避免异常值驱动舵机超出机械范围。
    if (servo_control_is_angle_valid(angle_deg) == false)
    {
        if (angle_deg > SERVO_ANGLE_MAX)
        {
            angle_deg = SERVO_ANGLE_MAX;
        }
        else
        {
            angle_deg = SERVO_ANGLE_MIN;
        }
    }

    // 先更新角度缓存，再计算脉宽。
    // 这样即使当前 PWM 尚未 ready，协议层读取角度时也能得到最新指令值。
    s_servo_angle_deg = angle_deg;
    pulse_us = servo_control_angle_to_pulse_us(s_servo_angle_deg);

    // TIM_CHANNEL_1 的宏值本身就是 0，因此这里只能用句柄和启动状态判断是否可写。
    // 未进入 ready 状态时仍然更新软件缓存，但不触发硬件寄存器写入。
    if ((s_servo_htim == NULL) || (s_servo_pwm_ready == 0U))
    {
        return;
    }

    // 更新比较寄存器，PWM 周期由 TIM3 配置固定为 20ms。
    // 这一步只负责改写脉宽，不重新启动定时器，也不修改 PWM 频率。
    __HAL_TIM_SET_COMPARE(s_servo_htim, s_servo_channel, pulse_us);
}

// 获取当前舵机角度。
// 返回缓存的角度值，用于 Modbus 读操作。
// @return 当前角度，uint16_t 类型。
uint16_t servo_control_get_angle(void)
{
    return s_servo_angle_deg;
}
