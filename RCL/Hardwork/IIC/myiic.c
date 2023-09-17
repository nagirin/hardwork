#include "./IIC/myiic.h"
#include "usart.h"
#include "main.h"
static i2c_t i2c[I2C_MAX];


//不进入systick 中断
void delay_us(uint32_t nus)
{
	 uint32_t temp;
	 SysTick->LOAD = 3*nus;
	 SysTick->VAL=0X00;				//清空计数器
	 SysTick->CTRL=0X01;			//使能，减到0是无动作，外部时钟源
 do
	{
			temp=SysTick->CTRL;	//读取当前计数值
	}while((temp&0x01)&&(!(temp&(1<<16))));//等待时间到达
   SysTick->CTRL=0x00; 		//关闭计数器
   SysTick->VAL =0X00; 		//清空计数器
}
void delay_ms(uint16_t nms)
{
	 uint32_t temp;
	 SysTick->LOAD = 3000*nms;
	 SysTick->VAL=0X00;
	 SysTick->CTRL=0X01;
	 do
	 {
			temp=SysTick->CTRL;
	 }while((temp&0x01)&&(!(temp&(1<<16))));
    SysTick->CTRL=0x00; 
    SysTick->VAL =0X00; 
}

lcr_err_t i2c_wait(uint8_t ch)
{
    uint32_t timeout = 200000 / i2c[ch].speed; // 200ms

    PIN_I2C_SCL_H(ch);

    while (timeout--)
    {
        if (PIN_I2C_SCL_READ(ch))
        {
            return LCR_OK;
        }

        delay_us(2);
    }

//    TASK_LOG(USE_DEBUG_I2C || USE_DEBUG_ERROR, ("%s wait timeout\r\n", i2c[ch].name));

    return LCR_ERROR;
}

lcr_err_t i2c_start(uint8_t ch)
{
    lcr_err_t result = LCR_ERROR;

//    TASK_LOG(USE_DEBUG_I2C, ("%s start\r\n", i2c[ch].name));
		printf("%s start\r\n", i2c[ch].name);
    PIN_I2C_SCL_L(ch);
    delay_us(4);

    PIN_I2C_SDA_H(ch);
    delay_us(4);

    if (i2c_wait(ch) == LCR_OK)
    {
        // SCL为高时，SDA由高转低
        PIN_I2C_SCL_H(ch);
        delay_us(4);

        PIN_I2C_SDA_L(ch);
        delay_us(4);

        PIN_I2C_SCL_L(ch);
        delay_us(10);
        result = LCR_OK;
    }

    return result;
}

lcr_err_t i2c_stop(uint8_t ch)
{
    lcr_err_t result = LCR_ERROR;

//    TASK_LOG(USE_DEBUG_I2C, ("%s stop\r\n", i2c[ch].name));
		printf("%s stop\r\n", i2c[ch].name);
    PIN_I2C_SCL_L(ch);
    delay_us(4);

    PIN_I2C_SDA_L(ch);
    delay_us(4);

    if (i2c_wait(ch) == LCR_OK)
    {
        // SCL为高时，SDA由低转高
        PIN_I2C_SCL_H(ch);
        delay_us(4);

        PIN_I2C_SDA_H(ch);
        delay_us(10);
        result = LCR_OK;
    }
    return result;
}

void i2c_ack(uint8_t ch)
{

//    TASK_LOG(USE_DEBUG_I2C, ("%s ack\r\n", i2c[ch].name));
		printf("%s ack\r\n", i2c[ch].name);
    PIN_I2C_SDA_L(ch);
    delay_us(2);

    PIN_I2C_SCL_H(ch);
    delay_us(2);

    PIN_I2C_SCL_L(ch);
    delay_us(2);
}

void i2c_nack(uint8_t ch)
{

//    TASK_LOG(USE_DEBUG_I2C, ("%s nack\r\n", i2c[ch].name));
		printf("%s nack\r\n", i2c[ch].name);
    PIN_I2C_SDA_H(ch);
    delay_us(2);

    PIN_I2C_SCL_H(ch);
    delay_us(2);

    PIN_I2C_SCL_L(ch);
    delay_us(2);
}

lcr_err_t i2c_wack(uint8_t ch)
{
    uint32_t timeout = 100000 / i2c[ch].speed; // 100ms


    PIN_I2C_SDA_H(ch);
    delay_us(1);

    while (timeout--)
    {
        if (PIN_I2C_SDA_READ(ch) == 0)
        {
            PIN_I2C_SCL_H(ch);
            delay_us(1);

            PIN_I2C_SCL_L(ch);
            delay_us(1);

//            TASK_LOG(USE_DEBUG_I2C, ("%s ack ok\r\n", i2c[ch].name));
						printf("%s ack ok\r\n", i2c[ch].name);
            return LCR_OK;
        }

        delay_us(2);

    }

//    TASK_LOG(USE_DEBUG_I2C || USE_DEBUG_ERROR, ("%s ack timeout\r\n", i2c[ch].name));
		printf("%s ack timeout\r\n", i2c[ch].name);
    i2c_stop(ch);

    return LCR_ERROR;
}

char i2c_read(uint8_t ch)
{
    char i, c = 0;

    PIN_I2C_SDA_H(ch);

    for (i = 0; i < 8; i++)
    {
        c = (c << 1) | (PIN_I2C_SDA_READ(ch));

        PIN_I2C_SCL_H(ch);
        delay_us(2);

        PIN_I2C_SCL_L(ch);
        delay_us(2);
    }

//    TASK_LOG(USE_DEBUG_I2C, ("%s read :  0x%02X\r\n", i2c[ch].name, c));
		printf("%s read :  0x%02X\r\n", i2c[ch].name, c);
    return c;
}

lcr_err_t i2c_read_ack(uint8_t ch, char *c, int ack)
{
    lcr_err_t result = LCR_ERROR;

    PIN_I2C_SDA_H(ch);

    if (i2c_wait(ch) == LCR_OK)
    {
        *c = i2c_read(ch);

        if (ack == 1)
        {
            i2c_ack(ch);
        }
        else
        {
            i2c_nack(ch);
        }

        result = LCR_OK;
    }
    else
    {
        i2c_stop(ch);
    }

    return result;
}
void i2c_write(uint8_t ch, char c)
{
    char i;

//    TASK_LOG(USE_DEBUG_I2C, ("%s write : 0x%02X\r\n", i2c[ch].name, c));

    for (i = 0x80; i > 0; i >>= 1)
    {
        if (c & i)
        {
            PIN_I2C_SDA_H(ch);
        }
        else
        {
            PIN_I2C_SDA_L(ch);
        }

        delay_us(2);

        PIN_I2C_SCL_H(ch);
        delay_us(2);

        PIN_I2C_SCL_L(ch);
        delay_us(2);
    }
}
lcr_err_t i2c_write_wack(uint8_t ch, char c)
{
    lcr_err_t result = LCR_ERROR;

    i2c_write(ch, c);

    if (i2c_wack(ch) == LCR_OK)
    {
        result = LCR_OK;
    }

    return result;
}

lcr_err_t i2c_init(uint8_t ch)
{
    GPIO_InitTypeDef GPIO_Initure;

    uint32_t cnt, cnt_scl, cnt_sda;

    if (i2c[ch].initialize == 1)
    {
        return LCR_OK;
    }

    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;     //快速

    if (ch == I2C_1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
			
				GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7;
				HAL_GPIO_Init(GPIOB,&GPIO_Initure);

        i2c[ch].scl.gpio = GPIOB;
        i2c[ch].scl.pin  = GPIO_PIN_6;
        i2c[ch].sda.gpio = GPIOB;
        i2c[ch].sda.pin  = GPIO_PIN_7;

        i2c[ch].name = "i2c_1";
        i2c[ch].speed = I2C_SPEED_100K;
    }
    else if (ch == I2C_2)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

				GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_9;
				HAL_GPIO_Init(GPIOB,&GPIO_Initure);

        i2c[ch].scl.gpio = GPIOB;
        i2c[ch].scl.pin  = GPIO_PIN_8;
        i2c[ch].sda.gpio = GPIOB;
        i2c[ch].sda.pin  = GPIO_PIN_9;

        i2c[ch].name = "i2c_2";
        i2c[ch].speed = I2C_SPEED_100K;
    }
//    else if (ch == I2C_3)
//    {
//        __HAL_RCC_GPIOE_CLK_ENABLE();

//				GPIO_Initure.Pin=GPIO_PIN_7|GPIO_PIN_8;
//				HAL_GPIO_Init(GPIOE,&GPIO_Initure);

//        i2c[ch].scl.gpio = GPIOE;
//        i2c[ch].scl.pin  = GPIO_PIN_8;
//        i2c[ch].sda.gpio = GPIOE;
//        i2c[ch].sda.pin  = GPIO_PIN_7;

//        i2c[ch].name = "i2c_3";
//        i2c[ch].speed = I2C_SPEED_100K;
//    }
    else
    {
        return LCR_NODEV;
    }

    PIN_I2C_SCL_H(ch);
    PIN_I2C_SDA_H(ch);

    cnt_scl = 0;
    cnt_sda = 0;

    for (cnt = 0; cnt < 10; cnt++)
    {
        cnt_scl += PIN_I2C_SCL_READ(ch);
        cnt_sda += PIN_I2C_SDA_READ(ch);

        delay_us(10);
    }

    if (cnt_scl != 10 || cnt_sda != 10)
    {
//        TASK_LOG(USE_DEBUG_INIT, ("initialize %s error\r\n", i2c[ch].name));
				printf("initialize %s error\r\n", i2c[ch].name);
        return LCR_ERROR;
    }

#if USE_OS
    mutex_init(&i2c[ch].mutex, i2c[ch].name);
#endif

    i2c[ch].initialize = 1;

//    TASK_LOG(USE_DEBUG_INIT, ("initialize %s\r\n", i2c[ch].name));
		printf("initialize %s\r\n", i2c[ch].name);

    return LCR_OK;
}

lcr_err_t i2c_close(uint8_t ch)
{

    if (i2c[ch].initialize == 1)
    {
#if USE_OS
        mutex_detach(&i2c[ch].mutex);
#endif

        i2c[ch].initialize = 0;
    }

    return LCR_OK;
}

lcr_err_t i2c_detect(uint8_t ch, uint8_t device_addr)
{
    lcr_err_t result = LCR_ERROR;

    if (i2c_start(ch) == LCR_OK)
    {
        if (i2c_write_wack(ch, device_addr) == LCR_OK)
        {
            result = LCR_OK;
        }

        i2c_stop(ch);
    }

    return result;
}
