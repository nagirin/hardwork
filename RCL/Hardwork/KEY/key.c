#include ".\KEY\key.h" 

void Key_GPIO_Config(void)

{

    GPIO_InitTypeDef GPIO_InitStructure;

    KEY1_GPIO_CLK_ENABLE();

    KEY2_GPIO_CLK_ENABLE();


    GPIO_InitStructure.Pin = KEY1_PIN; 

    GPIO_InitStructure.Mode = GPIO_MODE_INPUT; 

    GPIO_InitStructure.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = KEY2_PIN |KEY3_PIN |KEY4_PIN; 

    HAL_GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);

 
}



uint8_t Key_Scan(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin)

{			

	if(HAL_GPIO_ReadPin(GPIOx,GPIO_Pin) == KEY_ON )  

	{	 

		/*µÈ´ý°´¼üÊÍ·Å */
		while(HAL_GPIO_ReadPin(GPIOx,GPIO_Pin) == KEY_ON);   
		return 	KEY_ON;	 
	}

	else
		return KEY_OFF;

}

/*********************************************END OF FILE**********************/

