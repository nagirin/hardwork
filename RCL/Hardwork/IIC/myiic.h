#ifndef _MYIIC_H
#define _MYIIC_H
#include "main.h"


enum
{
    I2C_1 = 0,
		I2C_2,
		I2C_3,
    I2C_MAX
};

typedef enum
{
    I2C_SPEED_10K = 55,
    I2C_SPEED_20K = 27,
    I2C_SPEED_50K = 10,
    I2C_SPEED_75K = 7,
    I2C_SPEED_100K = 5,
    I2C_SPEED_200K = 2,
} iic_speed_t;

typedef enum _err_
{
    LCR_OK       = 0,
    LCR_ERROR    = -1,
    LCR_TIMEOUT  = -2,
    LCR_FULL     = -3,
    LCR_EMPTY    = -4,
    LCR_NOMEM    = -5,
    LCR_NOSYS    = -6,
    LCR_NODEV    = -7,
    LCR_BUSY     = -8,
    LCR_IO       = -9
} lcr_err_t;

typedef struct
{
    int initialize;

    struct
    {
        GPIO_TypeDef    *gpio;
        uint16_t     pin;
    } scl, sda;

    uint32_t speed;
		char *name;
#if USE_OS
//    xx_mutex_t mutex;
#endif
} i2c_t;

#define PIN_I2C_SCL_L(ch)       	HAL_GPIO_WritePin(i2c[ch].scl.gpio, i2c[ch].scl.pin, GPIO_PIN_RESET)
#define PIN_I2C_SCL_H(ch)       	HAL_GPIO_WritePin  (i2c[ch].scl.gpio, i2c[ch].scl.pin, GPIO_PIN_SET)
#define PIN_I2C_SDA_L(ch)       	HAL_GPIO_WritePin(i2c[ch].sda.gpio, i2c[ch].sda.pin, GPIO_PIN_RESET)
#define PIN_I2C_SDA_H(ch)       	HAL_GPIO_WritePin  (i2c[ch].sda.gpio, i2c[ch].sda.pin, GPIO_PIN_SET)
#define PIN_I2C_SCL_READ(ch)      HAL_GPIO_ReadPin		(i2c[ch].scl.gpio, i2c[ch].scl.pin)
#define PIN_I2C_SDA_READ(ch)      HAL_GPIO_ReadPin		(i2c[ch].sda.gpio, i2c[ch].sda.pin)




//IIC所有操作函数

void delay_us(uint32_t nus);
void delay_ms(uint16_t nms);
lcr_err_t i2c_wait(uint8_t ch);
lcr_err_t i2c_start(uint8_t ch);
lcr_err_t i2c_stop(uint8_t ch);
void i2c_ack(uint8_t ch);
void i2c_nack(uint8_t ch);
lcr_err_t i2c_wack(uint8_t ch);
char i2c_read(uint8_t ch);
lcr_err_t i2c_read_ack(uint8_t ch, char *c, int ack);
void i2c_write(uint8_t ch, char c);
lcr_err_t i2c_write_wack(uint8_t ch, char c);
lcr_err_t i2c_init(uint8_t ch);
lcr_err_t i2c_close(uint8_t ch);
lcr_err_t i2c_detect(uint8_t ch, uint8_t device_addr);

#endif

