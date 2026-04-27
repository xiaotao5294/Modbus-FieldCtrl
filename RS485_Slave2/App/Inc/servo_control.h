/**
 * @file servo_control.h
 * @brief SG90 舵机控制模块接口声明
 *
 * 此文件声明 SG90 舵机控制模块接口，负责角度合法性校验、角度到 PWM 脉宽
 * 映射，以及基于 TIM3 的占空比更新。
 *
 * 主要内容：
 * - 舵机控制初始化接口
 * - 舵机角度设置与读取接口
 * - 舵机角度合法性校验接口
 */

#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "tim.h"

void servo_control_init(TIM_HandleTypeDef *htim, uint32_t channel);
void servo_control_set_angle(uint16_t angle_deg);
uint16_t servo_control_get_angle(void);
bool servo_control_is_angle_valid(uint16_t angle_deg);

#endif
