#ifndef __LED_H
#define	__LED_H

#include "stm32f1xx.h"

/************************************************************/

#define LED_GPIO_CLK_ENABLE()   	__HAL_RCC_GPIOB_CLK_ENABLE()
#define LED_GPIO_PORT            	GPIOB
#define LED1_PIN                  GPIO_PIN_1               
#define LED2_PIN                  GPIO_PIN_2
#define LED3_PIN                  GPIO_PIN_10
#define LED4_PIN                  GPIO_PIN_11

/************************************************************/



#define ON  GPIO_PIN_RESET
#define OFF GPIO_PIN_SET

#define LED1(a)					HAL_GPIO_WritePin(LED_GPIO_PORT,LED1_PIN,a)
#define LED2(a)					HAL_GPIO_WritePin(LED_GPIO_PORT,LED2_PIN,a)
#define LED3(a)					HAL_GPIO_WritePin(LED_GPIO_PORT,LED3_PIN,a)
#define LED4(a)					HAL_GPIO_WritePin(LED_GPIO_PORT,LED4_PIN,a)


#define LED_OFF	\
				LED1(OFF);\
				LED2(OFF);\
				LED3(OFF);\
				LED4(OFF)
					

void LED_GPIO_Config(void);

#endif /* __LED_H */
