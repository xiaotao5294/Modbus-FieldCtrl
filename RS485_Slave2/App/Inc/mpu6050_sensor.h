/**
 * @file mpu6050_sensor.h
 * @brief MPU6050 传感器模块接口声明
 *
 * 此文件声明 MPU6050 传感器模块接口，负责 I2C 初始化、周期采样、原始加速度
 * 数据缓存，以及通信成功失败计数。
 *
 * 主要内容：
 * - 传感器初始化与轮询接口
 * - 三轴加速度读取接口
 * - 传感器链路诊断计数读取接口
 */

#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <stdint.h>

#include "i2c.h"

void mpu6050_sensor_init(I2C_HandleTypeDef *hi2c);
void mpu6050_sensor_poll(void);

int16_t mpu6050_sensor_get_accel_x_raw(void);
int16_t mpu6050_sensor_get_accel_y_raw(void);
int16_t mpu6050_sensor_get_accel_z_raw(void);

uint32_t mpu6050_sensor_get_comm_ok_cnt(void);
uint32_t mpu6050_sensor_get_comm_fail_cnt(void);

#endif
