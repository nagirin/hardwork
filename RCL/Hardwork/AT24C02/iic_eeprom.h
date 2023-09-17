#ifndef _IIC_EEPROM__
#define _IIC_EEPROM__

#include "main.h"
enum
{
    I2C_EEPROM_1 = 0,
    I2C_EEPROM_2,
    I2C_EEPROM_MAX
};

typedef struct
{
    int initialize;
    int detect;

    uint8_t i2c;
    uint8_t addr;
    uint8_t type;

    char *name;
} i2c_eeprom_t;

lcr_err_t i2c_eeprom_init(uint8_t ch);
lcr_err_t i2c_eeprom_close(uint8_t ch);
uint32_t i2c_eeprom_read(uint8_t ch, uint32_t addr, void *buf, uint32_t len);
uint32_t i2c_eeprom_write(uint8_t ch, uint32_t addr, const void *buf, uint32_t len);
lcr_err_t i2c_eeprom_clear(uint8_t ch);

#endif
