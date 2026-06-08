/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

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
#define led_Pin GPIO_PIN_13
#define led_GPIO_Port GPIOC
#define Motor1_Pin GPIO_PIN_0
#define Motor1_GPIO_Port GPIOA
#define Motor2_Pin GPIO_PIN_1
#define Motor2_GPIO_Port GPIOA
#define BUZZ1_Pin GPIO_PIN_4
#define BUZZ1_GPIO_Port GPIOA
#define LED2R_Pin GPIO_PIN_5
#define LED2R_GPIO_Port GPIOA
#define BUZZ3_Pin GPIO_PIN_1
#define BUZZ3_GPIO_Port GPIOB
#define BUZZ4_Pin GPIO_PIN_2
#define BUZZ4_GPIO_Port GPIOB
#define LED2G_Pin GPIO_PIN_11
#define LED2G_GPIO_Port GPIOA
#define LED2B_Pin GPIO_PIN_12
#define LED2B_GPIO_Port GPIOA
#define BUZZ2_Pin GPIO_PIN_15
#define BUZZ2_GPIO_Port GPIOA
#define BUZZ5_Pin GPIO_PIN_4
#define BUZZ5_GPIO_Port GPIOB
#define LED4R_Pin GPIO_PIN_5
#define LED4R_GPIO_Port GPIOB
#define LED4G_Pin GPIO_PIN_8
#define LED4G_GPIO_Port GPIOB
#define LED4B_Pin GPIO_PIN_9
#define LED4B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
