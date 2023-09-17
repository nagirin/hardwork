#include "./LED/led.h"


void LED_GPIO_Config(void)
{		
		
    GPIO_InitTypeDef  GPIO_InitStruct;
    LED_GPIO_CLK_ENABLE();
 															   
    GPIO_InitStruct.Pin = LED1_PIN | LED2_PIN |LED3_PIN |LED4_PIN;	
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  
    GPIO_InitStruct.Pull  = GPIO_PULLUP;   
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);	
															   
		LED_OFF;

}
