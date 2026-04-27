/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "i2c.h"

// I2C1 句柄由 HAL 驱动使用，供 MPU6050 模块绑定通信总线。
I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void)
{
  // 选择 I2C1 外设实例。
  hi2c1.Instance = I2C1;
  // 400kHz Fast Mode，满足 MPU6050 周期读取带宽需求。
  hi2c1.Init.ClockSpeed = 400000;
  // 标准占空比模式，兼容常见 I2C 设备。
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  // 从机地址字段在主机模式下不参与寻址，置 0。
  hi2c1.Init.OwnAddress1 = 0;
  // 使用 7 位设备地址模式。
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  // 关闭双地址模式，避免无效地址匹配。
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  // 关闭广播应答，防止被总线广播帧误触发。
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  // 允许时钟拉伸，兼容慢速器件应答。
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  // HAL 初始化失败时进入统一错误处理。
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  // GPIO 初始化结构体。
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  // 仅处理 I2C1 外设的底层资源初始化。
  if(i2cHandle->Instance==I2C1)
  {
    // 先开 GPIOB 时钟，保证后续 PB6/PB7 配置可写。
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    // I2C 线使用复用开漏输出，外部上拉保证总线空闲高电平。
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 最后开启 I2C1 外设时钟。
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
  // 仅处理 I2C1 的反初始化流程。
  if(i2cHandle->Instance==I2C1)
  {
    // 关闭外设时钟，降低功耗并防止误访问。
    __HAL_RCC_I2C1_CLK_DISABLE();

    // 释放 PB6/PB7 引脚占用。
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);
  }
}
