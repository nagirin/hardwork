#ifndef __KEY_H
#define	__KEY_H

#include "stm32f1xx.h"
#include "main.h"

/*******************************************************/
#define KEY1_PIN                  GPIO_PIN_8                 
#define KEY1_GPIO_PORT            GPIOA                      
#define KEY1_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()

#define KEY2_PIN                  GPIO_PIN_15                 
#define KEY2_GPIO_PORT            GPIOB                      
#define KEY2_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()

#define KEY3_PIN                  GPIO_PIN_14                 
#define KEY3_GPIO_PORT            GPIOB                      
#define KEY3_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()

#define KEY4_PIN                  GPIO_PIN_13                 
#define KEY4_GPIO_PORT            GPIOB                      
#define KEY4_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()
/*******************************************************/


#define KEY_ON	1
#define KEY_OFF	0

void Key_GPIO_Config(void);
uint8_t Key_Scan(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin);

#endif /* __LED_H */

