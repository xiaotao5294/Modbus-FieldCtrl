/**
 * @file mpu6050_sensor.c
 * @brief MPU6050 传感器模块实现
 *
 * 此文件实现 MPU6050 采样逻辑，包含上电唤醒、周期轮询读取三轴加速度原始值，
 * 以及 I2C 通信成功/失败统计。
 *
 * 主要内容：
 * - MPU6050 初始化与唤醒
 * - 周期 I2C 采样与原始值更新
 * - 三轴数据与通信诊断计数读取接口
 *
 * @note 此模块依赖 HAL I2C 驱动，确保 I2C 外设已正确初始化。
 * @note 采样周期由 MPU6050_POLL_INTERVAL_MS 定义，避免 I2C 总线过载。
 */

#include "mpu6050_sensor.h"

#include "app_config.h"

// I2C 句柄由上层在初始化时注入，模块内部不直接创建外设实例。
// 注入后，方可进行 I2C 读写操作。
static I2C_HandleTypeDef *s_i2c = NULL;
// 最近一次采样的 X 轴原始加速度值（高字节在前拼接得到 int16）。
// 单位：MPU6050 内部 LSB，范围约 -32768 到 32767，对应重力加速度。
static int16_t s_accel_x_raw = 0;
// 最近一次采样的 Y 轴原始加速度值。
static int16_t s_accel_y_raw = 0;
// 最近一次采样的 Z 轴原始加速度值。
static int16_t s_accel_z_raw = 0;
// 轮询节拍时间戳，用于限制 I2C 访问频率，避免总线无意义高负载。
// 基于 HAL_GetTick()，单位毫秒。
static uint32_t s_poll_tick = 0U;
// I2C 读写成功计数，用于现场诊断 MPU 链路稳定性。
// 每次成功读取三轴数据后递增。
static uint32_t s_comm_ok_cnt = 0U;
// I2C 读写失败计数，用于定位接线或器件异常。
// 每次 HAL_I2C 操作失败后递增。
static uint32_t s_comm_fail_cnt = 0U;

// 写单字节寄存器的底层工具函数，所有配置寄存器写操作统一走此入口。
// 此函数封装 HAL_I2C_Mem_Write，确保一致的错误处理和超时控制。
// @param reg MPU6050 寄存器地址。
// @param value 要写入的字节值。
// @return 1 表示成功，0 表示失败（I2C 错误或句柄未初始化）。
static uint8_t mpu6050_write_byte(uint8_t reg, uint8_t value)
{
    // HAL 返回状态，用于区分成功与失败路径。
    HAL_StatusTypeDef status;

    // 句柄未注入时直接失败，避免对空句柄发起总线访问。
    if (s_i2c == NULL)
    {
        return 0U;
    }

    // 通过寄存器地址写入 1 字节配置值。
    // 使用 I2C_MEMADD_SIZE_8BIT 表示 8 位地址模式。
    // 超时时间由 MPU6050_I2C_TIMEOUT_MS 定义。
    status = HAL_I2C_Mem_Write(s_i2c,
                               MPU6050_I2C_ADDR,
                               reg,
                               I2C_MEMADD_SIZE_8BIT,
                               &value,
                               1U,
                               MPU6050_I2C_TIMEOUT_MS);
    // HAL 非 OK 统一判定为失败，由上层计入失败计数。
    if (status != HAL_OK)
    {
        return 0U;
    }

    return 1U;
}

// 初始化 MPU6050 传感器模块。
// 注入 I2C 句柄，清空缓存，唤醒 MPU6050 进入工作模式。
// @param hi2c 已初始化的 I2C 句柄指针。
// @note 调用后，MPU6050 即可响应 I2C 读取操作。
// @note 若唤醒失败，会递增失败计数，但不阻塞初始化流程。
void mpu6050_sensor_init(I2C_HandleTypeDef *hi2c)
{
    // 保存 I2C 句柄并清空缓存，确保上电后的初始状态可预测。
    s_i2c = hi2c;
    s_accel_x_raw = 0;
    s_accel_y_raw = 0;
    s_accel_z_raw = 0;
    s_poll_tick = HAL_GetTick();
    s_comm_ok_cnt = 0U;
    s_comm_fail_cnt = 0U;

    // 唤醒 MPU6050：清除睡眠位，后续即可读取加速度寄存器。
    // PWR_MGMT_1 寄存器的睡眠位为 0 时唤醒。
    if (mpu6050_write_byte(MPU6050_REG_PWR_MGMT_1, MPU6050_WAKEUP_VALUE) == 0U)
    {
        s_comm_fail_cnt++;
    }
}

// 周期轮询 MPU6050，读取三轴加速度原始值。
// 仅在采样间隔到达时执行 I2C 读取，避免总线拥塞。
// @note 此函数应由定时任务调用，频率控制在 MPU6050_POLL_INTERVAL_MS。
// @note 读取失败时保留上次有效数据，仅递增失败计数。
void mpu6050_sensor_poll(void)
{
    // I2C 访问返回状态，失败时计数并退出。
    HAL_StatusTypeDef status;
    // 连续读取 ACCEL_X/Y/Z 共 6 字节缓冲区。
    uint8_t raw_buf[6] = {0U};
    // 当前系统节拍。
    uint32_t now;

    // 未初始化时不执行采样。
    if (s_i2c == NULL)
    {
        return;
    }

    // 轮询限频：未到采样周期直接返回，避免 I2C 访问过密。
    now = HAL_GetTick();
    if ((now - s_poll_tick) < MPU6050_POLL_INTERVAL_MS)
    {
        return;
    }
    // 到周期后立刻推进时间戳，保证采样周期固定。
    s_poll_tick = now;

    // 从 ACCEL_XOUT_H 连续读取 6 字节，得到三轴 16 位原始值。
    // MPU6050 寄存器为大端格式，高字节在前。
    status = HAL_I2C_Mem_Read(s_i2c,
                              MPU6050_I2C_ADDR,
                              MPU6050_REG_ACCEL_XOUT_H,
                              I2C_MEMADD_SIZE_8BIT,
                              raw_buf,
                              6U,
                              MPU6050_I2C_TIMEOUT_MS);
    // 读取失败时只更新失败计数，不覆盖上一次有效数据。
    if (status != HAL_OK)
    {
        s_comm_fail_cnt++;
        return;
    }

    // 按大端顺序拼接为 int16，保持与 MPU6050 寄存器定义一致。
    s_accel_x_raw = (int16_t)(((uint16_t)raw_buf[0] << 8U) | raw_buf[1]);
    s_accel_y_raw = (int16_t)(((uint16_t)raw_buf[2] << 8U) | raw_buf[3]);
    s_accel_z_raw = (int16_t)(((uint16_t)raw_buf[4] << 8U) | raw_buf[5]);
    // 成功读取后更新成功计数。
    s_comm_ok_cnt++;
}

// 获取最近一次采样的 X 轴加速度原始值。
// @return X 轴原始值，int16_t 类型，未初始化时返回 0。
int16_t mpu6050_sensor_get_accel_x_raw(void)
{
    return s_accel_x_raw;
}

// 获取最近一次采样的 Y 轴加速度原始值。
// @return Y 轴原始值，int16_t 类型，未初始化时返回 0。
int16_t mpu6050_sensor_get_accel_y_raw(void)
{
    return s_accel_y_raw;
}

// 获取最近一次采样的 Z 轴加速度原始值。
// @return Z 轴原始值，int16_t 类型，未初始化时返回 0。
int16_t mpu6050_sensor_get_accel_z_raw(void)
{
    return s_accel_z_raw;
}

// 获取 I2C 通信成功计数。
// @return 成功计数，uint32_t 类型，可用于诊断链路稳定性。
uint32_t mpu6050_sensor_get_comm_ok_cnt(void)
{
    return s_comm_ok_cnt;
}

// 获取 I2C 通信失败计数。
// @return 失败计数，uint32_t 类型，可用于定位硬件故障。
uint32_t mpu6050_sensor_get_comm_fail_cnt(void)
{
    return s_comm_fail_cnt;
}
