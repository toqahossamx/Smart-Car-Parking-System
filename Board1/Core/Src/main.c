/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rc522.h"
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t status;
uint8_t str[16];
uint8_t sNum[5];
//char global_rx_buffer[256] = {0}; //sol
//uint8_t rx_idx = 0; //sol
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */






uint32_t IC_Val1[4] = {0};
uint32_t IC_Val2[4] = {0};
uint32_t Difference[4] = {0};
uint8_t Is_First_Captured[4] = {0};
float Distance[4] = {0};

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < us);
}

void HCSR04_Read_Sensor(uint8_t idx)
{
    uint16_t trig_pin;
    uint32_t channel_it;
    uint32_t timer_channel;

    switch(idx)
    {
        case 0: trig_pin = Trig1_Pin; channel_it = TIM_IT_CC1; timer_channel = TIM_CHANNEL_1; break;
        case 1: trig_pin = Trig2_Pin; channel_it = TIM_IT_CC2; timer_channel = TIM_CHANNEL_2; break;
        case 2: trig_pin = Trig3_Pin; channel_it = TIM_IT_CC3; timer_channel = TIM_CHANNEL_3; break;
        case 3: trig_pin = Trig4_Pin; channel_it = TIM_IT_CC4; timer_channel = TIM_CHANNEL_4; break;
        default: return;
    }

    Is_First_Captured[idx] = 0;
    __HAL_TIM_SET_CAPTUREPOLARITY(&htim1, timer_channel, TIM_INPUTCHANNELPOLARITY_RISING);

    __HAL_TIM_CLEAR_FLAG(&htim1, channel_it);

    HAL_GPIO_WritePin(GPIOB, trig_pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(GPIOB, trig_pin, GPIO_PIN_RESET);

    __HAL_TIM_ENABLE_IT(&htim1, channel_it);
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM1) return;

    uint8_t idx;
    uint32_t channel;

    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) { idx = 0; channel = TIM_CHANNEL_1; }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) { idx = 1; channel = TIM_CHANNEL_2; }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) { idx = 2; channel = TIM_CHANNEL_3; }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) { idx = 3; channel = TIM_CHANNEL_4; }
    else return;

    if (Is_First_Captured[idx] == 0)
    {
        IC_Val1[idx] = HAL_TIM_ReadCapturedValue(htim, channel);
        Is_First_Captured[idx] = 1;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else
    {
        IC_Val2[idx] = HAL_TIM_ReadCapturedValue(htim, channel);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_INPUTCHANNELPOLARITY_RISING);

        if (IC_Val2[idx] > IC_Val1[idx])
            Difference[idx] = IC_Val2[idx] - IC_Val1[idx];
        else
            Difference[idx] = (htim->Instance->ARR - IC_Val1[idx]) + IC_Val2[idx];

        Distance[idx] = Difference[idx] * 0.034 / 2;
        Is_First_Captured[idx] = 0;
    }

}



void Send_Sensor_Update(char type, uint8_t index, uint8_t state) {
    char msg[10];
    sprintf(msg, "%c,%d,%d\n", type, index, state);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
}






/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_4);

  MFRC522_Init();

 // ESP_ConnectToServer();



  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(2000);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  HAL_Delay(100);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */




    int flagM1 = 0;
    int flagM2 = 0;
    int flagG = 0;
    int flagU[] = {0,0,0,0};
    int flagI[] = {0,0,0,0,0};




    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


      HAL_Delay(1000);



	    status = MFRC522_Request(PICC_REQIDL, str);

	    if (status == MI_OK)
	    {
	       status = MFRC522_Anticoll(str);

	        if (status == MI_OK)
	        {
	            HAL_GPIO_WritePin(GPIOA, RFIDLed_Pin, GPIO_PIN_SET);
	            if (flagM1 == 0){
	            Send_Sensor_Update('M', 1, 1); flagM1 = 1;}
	            HAL_Delay(5000);

	            char idString[16];
	            sprintf(idString, "R,%02X%02X%02X%02X\n", str[0], str[1], str[2], str[3]);
	            HAL_UART_Transmit(&huart2, (uint8_t*)idString, strlen(idString), 100);

	            HAL_Delay(6000);

	            HAL_GPIO_WritePin(GPIOA, RFIDLed_Pin, GPIO_PIN_RESET);
	            if(flagM1 == 1){
	            Send_Sensor_Update('M', 1, 0); flagM1 = 0;}
	            HAL_Delay(5000);
	        }
	    }





	    if (HAL_GPIO_ReadPin(GPIOB, Gas_Pin) == GPIO_PIN_RESET)
	          {
	              HAL_GPIO_WritePin(GPIOB, GasLed_Pin, GPIO_PIN_SET);

	              if (flagG == 0){
	              Send_Sensor_Update('G', 0, 1); flagG = 1;
	              HAL_Delay(4000);}
	          }
	          else
	          {
	              HAL_GPIO_WritePin(GPIOB, GasLed_Pin, GPIO_PIN_RESET);
	              if (flagG == 1){
	              Send_Sensor_Update('G', 0, 0); flagG = 0;
	              HAL_Delay(4000);}
	          }


	  for (uint8_t i = 0; i < 4; i++)
		 	 	      {
		 	 	          Is_First_Captured[i] = 0;

		 	 	          uint32_t ch = (i==0)?TIM_CHANNEL_1 : (i==1)?TIM_CHANNEL_2 : (i==2)?TIM_CHANNEL_3 : TIM_CHANNEL_4;
		 	 	          __HAL_TIM_SET_CAPTUREPOLARITY(&htim1, ch, TIM_INPUTCHANNELPOLARITY_RISING);

		 	 	          HCSR04_Read_Sensor(i);
		 	 	          HAL_Delay(50);

		 	 	          uint16_t led_pin;
		 	 	          switch(i)
		 	 	          {
		 	 	              case 0: led_pin = USLed1_Pin; break;
		 	 	              case 1: led_pin = USLed2_Pin; break;
		 	 	              case 2: led_pin = USLed3_Pin; break;
		 	 	              case 3: led_pin = USLed4_Pin; break;
		 	 	              default: continue;
		 	 	          }

		 	 	          if (Distance[i] > 0.1 && Distance[i] < 5.0){
		 	 	              HAL_GPIO_WritePin(GPIOA, led_pin, GPIO_PIN_SET);
		 	 	              if (flagU[i] == 0){
		 	 	              Send_Sensor_Update('U', i+1, 1); HAL_Delay(4000); flagU[i] = 1;}
		 	 	          }
		 	 	          else{
		 	 	              HAL_GPIO_WritePin(GPIOA, led_pin, GPIO_PIN_RESET);
		 	 	              if (flagU[i] == 1){
		 	 	              Send_Sensor_Update('U', i+1, 0); HAL_Delay(4000); flagU[i] = 0;}
		 	 	          }
		 	 	      }

		 	 		  if (HAL_GPIO_ReadPin(ExitButton_GPIO_Port, ExitButton_Pin) == GPIO_PIN_RESET)
		 	 		  {
		 	 		      HAL_GPIO_WritePin(ExitLed_GPIO_Port, ExitLed_Pin, GPIO_PIN_SET);
		 	 		      if (flagM2 == 0){
		 	 		      Send_Sensor_Update('M', 2, 1); HAL_Delay(4000); flagM2 = 1;}
		 	 		  }
		 	 	      else
		 	 		  {
		 	 		      HAL_GPIO_WritePin(ExitLed_GPIO_Port, ExitLed_Pin, GPIO_PIN_RESET);
		 	 		      if (flagM2 == 1){
		 	 		      Send_Sensor_Update('M', 2, 0); HAL_Delay(4000); flagM2 = 0;}
		 	 		  }


		 	 	      if (HAL_GPIO_ReadPin(IR1_GPIO_Port, IR1_Pin) == GPIO_PIN_RESET) {
		 	 	    	  if (flagI[0] == 1){
		 	 	          Send_Sensor_Update('I', 1, 1); HAL_Delay(4000); flagI[0] = 0;}
		 	 	      } else {
		 	 	    	if (flagI[0] == 0){
		 	 	    	Send_Sensor_Update('I', 1, 0); HAL_Delay(4000); flagI[0] = 1;}
		 	 	      }
		 	 	      if (HAL_GPIO_ReadPin(IR2_GPIO_Port, IR2_Pin) == GPIO_PIN_RESET) {
		 	 	    	if (flagI[1] == 1){
		 	 	        Send_Sensor_Update('I', 2, 1); HAL_Delay(4000); flagI[1] = 0;}
		 	 	      } else {
		 	 	    	if (flagI[1] == 0){
		 	 	        Send_Sensor_Update('I', 2, 0); HAL_Delay(4000); flagI[1] = 1;}
		 	 	      }

		 	 	      if (HAL_GPIO_ReadPin(IR3_GPIO_Port, IR3_Pin) == GPIO_PIN_RESET) {
		 	 	    	if (flagI[2] == 1){
		 	 	        Send_Sensor_Update('I', 3, 1); HAL_Delay(4000); flagI[2] = 0;}
		 	 	      } else {
		 	 	    	if (flagI[2] == 0){
		 	 	        Send_Sensor_Update('I', 3, 0); HAL_Delay(4000); flagI[2] = 1;}
		 	 	      }
		 	 	      if (HAL_GPIO_ReadPin(IR4_GPIO_Port, IR4_Pin) == GPIO_PIN_RESET) {
		 	 	    	if (flagI[3] == 1){
		 	 	        Send_Sensor_Update('I', 4, 1); HAL_Delay(4000); flagI[3] = 0;}
		 	 	      } else {
		 	 	    	if (flagI[3] == 0){
		 	 	         Send_Sensor_Update('I', 4, 0); HAL_Delay(4000); flagI[3] = 1;}
		 	 	      }
		 	 	      if (HAL_GPIO_ReadPin(IR5_GPIO_Port, IR5_Pin) == GPIO_PIN_RESET) {
		 	 	          HAL_GPIO_WritePin(IRLed5_GPIO_Port, IRLed5_Pin, GPIO_PIN_RESET);
		 	 	        if (flagI[4] == 1){
		 	 	        Send_Sensor_Update('I', 5, 1); HAL_Delay(4000); flagI[4] = 0;}
		 	 	      } else {
		 	 	         HAL_GPIO_WritePin(IRLed5_GPIO_Port, IRLed5_Pin, GPIO_PIN_SET);
		 	 	       if (flagI[4] == 0){
		 	 	        Send_Sensor_Update('I', 5, 0); HAL_Delay(4000); flagI[4] = 1;}
		 	 	      }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 15;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, USLed1_Pin|USLed2_Pin|RFIDLed_Pin|USLed3_Pin
                          |USLed4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, IRLed5_Pin|Trig2_Pin|Trig1_Pin|ExitLed_Pin
                          |GasLed_Pin|Trig3_Pin|Trig4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : led_Pin */
  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USLed1_Pin USLed2_Pin RFIDLed_Pin USLed3_Pin
                           USLed4_Pin */
  GPIO_InitStruct.Pin = USLed1_Pin|USLed2_Pin|RFIDLed_Pin|USLed3_Pin
                          |USLed4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : IRLed5_Pin Trig2_Pin Trig1_Pin ExitLed_Pin
                           GasLed_Pin Trig3_Pin Trig4_Pin */
  GPIO_InitStruct.Pin = IRLed5_Pin|Trig2_Pin|Trig1_Pin|ExitLed_Pin
                          |GasLed_Pin|Trig3_Pin|Trig4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Gas_Pin */
  GPIO_InitStruct.Pin = Gas_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Gas_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ExitButton_Pin IR1_Pin IR2_Pin IR3_Pin
                           IR4_Pin IR5_Pin */
  GPIO_InitStruct.Pin = ExitButton_Pin|IR1_Pin|IR2_Pin|IR3_Pin
                          |IR4_Pin|IR5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
