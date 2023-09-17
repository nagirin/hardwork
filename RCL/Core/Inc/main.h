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
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"	
#include "spi.h"	
#include "tim.h"	
#include "./LED/led.h"
#include "./KEY/key.h" 
#include "./IIC/myiic.h"
	
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
#define LED1_Pin GPIO_PIN_1
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_2
#define LED2_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_10
#define LED3_GPIO_Port GPIOB
#define LED4_Pin GPIO_PIN_11
#define LED4_GPIO_Port GPIOB
#define KEY4_Pin GPIO_PIN_13
#define KEY4_GPIO_Port GPIOB
#define KEY3_Pin GPIO_PIN_14
#define KEY3_GPIO_Port GPIOB
#define KEY2_Pin GPIO_PIN_15
#define KEY2_GPIO_Port GPIOB
#define KEY1_Pin GPIO_PIN_8
#define KEY1_GPIO_Port GPIOA
#define ESP_TX_Pin GPIO_PIN_9
#define ESP_TX_GPIO_Port GPIOA
#define ESP_RX_Pin GPIO_PIN_10
#define ESP_RX_GPIO_Port GPIOA
#define ESP_CS_Pin GPIO_PIN_11
#define ESP_CS_GPIO_Port GPIOA
#define ESP_RST_Pin GPIO_PIN_12
#define ESP_RST_GPIO_Port GPIOA
#define IIC1_SCL_Pin GPIO_PIN_6
#define IIC1_SCL_GPIO_Port GPIOB
#define IIC1_SDA_Pin GPIO_PIN_7
#define IIC1_SDA_GPIO_Port GPIOB
#define IIC2_SCL_Pin GPIO_PIN_8
#define IIC2_SCL_GPIO_Port GPIOB
#define IIC2_SDA_Pin GPIO_PIN_9
#define IIC2_SDA_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
