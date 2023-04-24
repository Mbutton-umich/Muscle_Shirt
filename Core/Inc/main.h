/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOA
#define EMG_ADC_IN_A3__Pin GPIO_PIN_4
#define EMG_ADC_IN_A3__GPIO_Port GPIOA
#define Strain_ADC_IN_A4__Pin GPIO_PIN_5
#define Strain_ADC_IN_A4__GPIO_Port GPIOA
#define MUX_A1_A5__Pin GPIO_PIN_6
#define MUX_A1_A5__GPIO_Port GPIOA
#define MUX_EN_D3__Pin GPIO_PIN_0
#define MUX_EN_D3__GPIO_Port GPIOB
#define ADC_IN_D6__Pin GPIO_PIN_1
#define ADC_IN_D6__GPIO_Port GPIOB
#define LED_PWM_D9__Pin GPIO_PIN_8
#define LED_PWM_D9__GPIO_Port GPIOA
#define Screen_SCL_D1__Pin GPIO_PIN_9
#define Screen_SCL_D1__GPIO_Port GPIOA
#define Screen_SDA_D0__Pin GPIO_PIN_10
#define Screen_SDA_D0__GPIO_Port GPIOA
#define Button_D2__Pin GPIO_PIN_12
#define Button_D2__GPIO_Port GPIOA
#define Button_D2__EXTI_IRQn EXTI15_10_IRQn
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_15
#define VCP_RX_GPIO_Port GPIOA
#define LD3_Pin GPIO_PIN_3
#define LD3_GPIO_Port GPIOB
#define MUX_A2_D11__Pin GPIO_PIN_5
#define MUX_A2_D11__GPIO_Port GPIOB
#define MUX_A0_D4__Pin GPIO_PIN_7
#define MUX_A0_D4__GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
