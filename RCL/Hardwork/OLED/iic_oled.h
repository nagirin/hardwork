#ifndef __IIC_OLED_H__
#define	__IIC_OLED_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include ".\IIC\myiic.h"
//#include ".\OLED/codetab.h"

enum
{
    I2C_OLED_1 = 0,
    I2C_OLED_MAX
};

typedef struct
{
    int initialize;
    int detect;

    uint8_t i2c;
    uint8_t addr;
    uint8_t type;

    char *name;
} i2c_oled_t;


extern unsigned char iic_F16x16[];
extern const unsigned char iic_F6x8[][6];
extern const unsigned char iic_F8X16[];
extern unsigned char iic_BMP1[]; 


lcr_err_t i2c_oled_init(uint8_t ch);
void i2c_oled_setPos(uint8_t x, uint8_t y);
void i2c_oled_fill(uint8_t fill_Data);
void i2c_oled_cls(void);
void i2c_oled_wakeup(void);
void i2c_oled_off(void);
void i2c_oled_showstr(uint8_t x, uint8_t y, uint8_t ch[], uint8_t TextSize);
void i2c_oled_showCN(uint8_t x, uint8_t y, uint8_t N);
void i2c_oled_drawBMP(uint8_t x0,uint8_t y0,uint8_t x1,uint8_t y1,uint8_t BMP[]);

#endif