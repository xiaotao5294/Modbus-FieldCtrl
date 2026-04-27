/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"

/* FreeRTOS 仅在中断层用于接管内核异常和节拍，不在这里引入业务逻辑。 */
#include "FreeRTOS.h"
#include "task.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart2;  // 主机 USART2 句柄，用于把 IRQ 转给 HAL�?

extern void xPortSysTickHandler(void);

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* HardFault 诊断信息：发生故障时保留关键寄存器，便于调试器直接查看。 */
volatile uint32_t s_hardfault_r0 = 0U;
volatile uint32_t s_hardfault_r1 = 0U;
volatile uint32_t s_hardfault_r2 = 0U;
volatile uint32_t s_hardfault_r3 = 0U;
volatile uint32_t s_hardfault_r12 = 0U;
volatile uint32_t s_hardfault_lr = 0U;
volatile uint32_t s_hardfault_pc = 0U;
volatile uint32_t s_hardfault_psr = 0U;
volatile uint32_t s_hardfault_cfsr = 0U;
volatile uint32_t s_hardfault_hfsr = 0U;
volatile uint32_t s_hardfault_mmar = 0U;
volatile uint32_t s_hardfault_bfar = 0U;
volatile uint32_t s_hardfault_code = 0U;
volatile uint32_t s_hardfault_vtor = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void __attribute__((used)) HardFault_Capture(uint32_t *stacked_frame);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
__attribute__((naked)) void HardFault_Handler(void)
{
  __asm volatile(
    "tst lr, #4        \n"
    "ite eq            \n"
    "mrseq r0, msp     \n"
    "mrsne r0, psp     \n"
    "b HardFault_Capture \n"
  );
}

/*
 * @brief 捕获 HardFault 上下文并停机。
 *
 * 该函数不尝试恢复运行，而是把异常入口时的栈帧和系统故障寄存器
 * 保存到全局变量，方便在调试器中直接读取故障发生位置。
 */
static void __attribute__((used)) HardFault_Capture(uint32_t *stacked_frame)
{
  s_hardfault_r0 = stacked_frame[0];
  s_hardfault_r1 = stacked_frame[1];
  s_hardfault_r2 = stacked_frame[2];
  s_hardfault_r3 = stacked_frame[3];
  s_hardfault_r12 = stacked_frame[4];
  s_hardfault_lr = stacked_frame[5];
  s_hardfault_pc = stacked_frame[6];
  s_hardfault_psr = stacked_frame[7];

  s_hardfault_cfsr = SCB->CFSR;
  s_hardfault_hfsr = SCB->HFSR;
  s_hardfault_mmar = SCB->MMFAR;
  s_hardfault_bfar = SCB->BFAR;
  s_hardfault_vtor = SCB->VTOR;
  s_hardfault_code = 0xA55E00F1UL;

  /* 亮起板载 LED，给现场一个明确的硬件故障标记。 */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  __disable_irq();
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* 只有在调度器运行后才把节拍交给 FreeRTOS，避免启动前误触发内核状态。 */
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    xPortSysTickHandler();
  }
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */  // 预留给用户在进入 IRQ 时插入额外处理�?

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);  // 交给 HAL 完成接收完成和错误事件分发�?
  /* USER CODE BEGIN USART2_IRQn 1 */  // 预留给用户在退�?IRQ 前插入额外处理�?

  /* USER CODE END USART2_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

