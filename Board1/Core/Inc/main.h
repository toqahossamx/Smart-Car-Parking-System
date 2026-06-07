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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define led_Pin GPIO_PIN_13
#define led_GPIO_Port GPIOC
#define USLed1_Pin GPIO_PIN_0
#define USLed1_GPIO_Port GPIOA
#define USLed2_Pin GPIO_PIN_1
#define USLed2_GPIO_Port GPIOA
#define RFIDLed_Pin GPIO_PIN_4
#define RFIDLed_GPIO_Port GPIOA
#define IRLed5_Pin GPIO_PIN_0
#define IRLed5_GPIO_Port GPIOB
#define Trig2_Pin GPIO_PIN_2
#define Trig2_GPIO_Port GPIOB
#define Trig1_Pin GPIO_PIN_10
#define Trig1_GPIO_Port GPIOB
#define Gas_Pin GPIO_PIN_12
#define Gas_GPIO_Port GPIOB
#define ExitLed_Pin GPIO_PIN_13
#define ExitLed_GPIO_Port GPIOB
#define GasLed_Pin GPIO_PIN_14
#define GasLed_GPIO_Port GPIOB
#define ExitButton_Pin GPIO_PIN_15
#define ExitButton_GPIO_Port GPIOB
#define Echo1_Pin GPIO_PIN_8
#define Echo1_GPIO_Port GPIOA
#define Echo2_Pin GPIO_PIN_9
#define Echo2_GPIO_Port GPIOA
#define Echo3_Pin GPIO_PIN_10
#define Echo3_GPIO_Port GPIOA
#define Echo4_Pin GPIO_PIN_11
#define Echo4_GPIO_Port GPIOA
#define USLed3_Pin GPIO_PIN_12
#define USLed3_GPIO_Port GPIOA
#define USLed4_Pin GPIO_PIN_15
#define USLed4_GPIO_Port GPIOA
#define Trig3_Pin GPIO_PIN_3
#define Trig3_GPIO_Port GPIOB
#define Trig4_Pin GPIO_PIN_4
#define Trig4_GPIO_Port GPIOB
#define IR1_Pin GPIO_PIN_5
#define IR1_GPIO_Port GPIOB
#define IR2_Pin GPIO_PIN_6
#define IR2_GPIO_Port GPIOB
#define IR3_Pin GPIO_PIN_7
#define IR3_GPIO_Port GPIOB
#define IR4_Pin GPIO_PIN_8
#define IR4_GPIO_Port GPIOB
#define IR5_Pin GPIO_PIN_9
#define IR5_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
