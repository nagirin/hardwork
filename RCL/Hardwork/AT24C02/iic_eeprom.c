#include ".\IIC\myiic.h"
#include "./AT24C02/iic_eeprom.h"
#include "main.h"
/*
    AT24C01  1kbit   128byte   页写一次最大8字节
    AT24C02  2kbit   256byte   页写一次最大8字节
    AT24C04  4kbit   512byte   页写一次最大16字节
    AT24C08  8kbit   1024byte  页写一次最大16字节
    AT24C16  16kbit  2048byte  页写一次最大16字节
    AT24C32  32kbit  4096byte  页写一次最大32字节
    AT24C64  64kbit  8192byte  页写一次最大32字节
    AT24C128 128kbit 16384byte 页写一次最大64字节
    AT24C256 256kbit 32768byte 页写一次最大64字节
    AT24C512 512kbit 65535byte 页写一次最大128字节
*/

enum
{
    TYPE_AT24C01 = 0,
    TYPE_AT24C02,
    TYPE_AT24C04,
    TYPE_AT24C08,
    TYPE_AT24C16,
    TYPE_AT24C32,
    TYPE_AT24C64,
    TYPE_AT24C128,
    TYPE_AT24C256,
    TYPE_AT24C512
};

const uint32_t _size[]      = {128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65535};
const uint32_t _page_size[] = {  8,   8,  16,   16,   16,   32,   32,    64,    64,   128};
const char _type[][8]          = {"24C01", "24C02", "24C04", "24C08", "24C16", "24C32", "24C64", "24C128", "24C256", "24C512"};



static i2c_eeprom_t i2c_device[I2C_EEPROM_MAX];

static uint32_t _read_bytes(uint8_t ch, uint32_t addr, void *buf, uint32_t len);
static uint32_t _write_bytes(uint8_t ch, uint32_t addr, const void *buf, uint32_t len);


static lcr_err_t _detect(uint8_t ch)
{
    lcr_err_t result = LCR_ERROR;
    uint8_t i2c = i2c_device[ch].i2c;

    if (i2c_detect(i2c, i2c_device[ch].addr) == LCR_OK)
    {
				printf("%s detect\r\n", i2c_device[ch].name);
        i2c_device[ch].type = TYPE_AT24C16;

        while (result != LCR_OK)
        {
            while (i2c_device[ch].type)
            {
                char old = 0;
                char byte[2] = { 0 };

                if (_read_bytes(ch, _size[i2c_device[ch].type] - 1, &old, 1) == 1)
                {
                    byte[0] = 0x5a;
                    _write_bytes(ch, _size[i2c_device[ch].type] - 1, &byte[0], 1);
                    _read_bytes(ch, _size[i2c_device[ch].type] - 1, &byte[0], 1);
                    _read_bytes(ch, _size[i2c_device[ch].type] / 2 - 1, &byte[1], 1);

                    //TASK_LOG(USE_DEBUG_ERROR, ("%s %02x, %02x, %02x\r\n", i2c_device[ch].name, old, byte[0], byte[1]));

                    if (byte[0] == 0x5a && byte[0] != byte[1])
                    {
                        byte[0] = 0x5b;
                        _write_bytes(ch, _size[i2c_device[ch].type] - 1, &byte[0], 1);
                        _read_bytes(ch, _size[i2c_device[ch].type] - 1, &byte[0], 1);
                        _read_bytes(ch, _size[i2c_device[ch].type] / 2 - 1, &byte[1], 1);

                        //TASK_LOG(USE_DEBUG_ERROR, ("%s %02x, %02x, %02x\r\n", i2c_device[ch].name, old, byte[0], byte[1]));

                        if (byte[0] == 0x5b && byte[0] != byte[1])
                        {
                            //TASK_LOG(USE_DEBUG_ERROR, ("%s detect %s\r\n", i2c_device[ch].name, _type[i2c_device[ch].type]));
														printf("%s detect %s\r\n", i2c_device[ch].name, _type[i2c_device[ch].type]);
                            _write_bytes(ch, _size[i2c_device[ch].type] - 1, &old, 1);

                            result = LCR_OK;

                            break;
                        }
                    }
                }

                i2c_device[ch].type--;

                delay_ms(10);
            }

            if (result != LCR_OK)
            {
                i2c_device[ch].type = TYPE_AT24C512;
            }
        }
    }
    else
    {
        //TASK_LOG(USE_DEBUG_ERROR, ("%s not present\r\n", i2c_device[ch].name));
			printf("%s not present\r\n", i2c_device[ch].name);
    }


    return result;
}

static uint32_t _read_bytes(uint8_t ch, uint32_t addr, void *buf, uint32_t len)
{
    uint8_t i2c = i2c_device[ch].i2c;

    uint32_t i = 0;
    uint8_t addr_h = 0;

    if (i2c_start(i2c) == LCR_OK)
    {
        if (i2c_device[ch].type < TYPE_AT24C32)
        {
            addr_h = (addr >> 8) << 1;
        }

        if (i2c_write_wack(i2c, i2c_device[ch].addr | addr_h) == LCR_OK)                                          // 器件地址，写
        {
            if ((i2c_device[ch].type < TYPE_AT24C32) ? 1 : (i2c_write_wack(i2c, addr >> 8) == LCR_OK))      // 高地址
            {
                if (i2c_write_wack(i2c, addr & 0xff) == LCR_OK)                                                   // 低地址
                {
                    i2c_start(i2c);

                    if (i2c_write_wack(i2c, i2c_device[ch].addr | addr_h + 1) == LCR_OK)                          // 器件地址，读
                    {
                        for (i = 0; i < len; i++)
                        {
                            i2c_read_ack(i2c, (char *)buf + i, (int)(i < len - 1));
                        }
                    }
                    else
                    {
                        //TASK_LOG(USE_DEBUG_ERROR, ("%s read error : read data\r\n", i2c_device[ch].name));
												printf("%s read error : read data\r\n", i2c_device[ch].name);
                    }
                }
                else
                {
                    //TASK_LOG(USE_DEBUG_ERROR, ("%s read error : low address\r\n", i2c_device[ch].name));
										printf("%s read error : low address\r\n", i2c_device[ch].name);
                }
            }
            else
            {
                //TASK_LOG(USE_DEBUG_ERROR, ("%s read error : high address\r\n", i2c_device[ch].name));
								printf("%s read error : high address\r\n", i2c_device[ch].name);
            }
        }
        else
        {
            //TASK_LOG(USE_DEBUG_ERROR, ("%s read error : device address %02x\r\n", i2c_device[ch].name, i2c_device[ch].addr | addr_h));
						printf("%s read error : device address %02x\r\n", i2c_device[ch].name, i2c_device[ch].addr | addr_h);
        }

        i2c_stop(i2c);
    }

    return i;
}
static uint32_t _write_bytes(uint8_t ch, uint32_t addr, const void *buf, uint32_t len)
{
    uint8_t i2c = i2c_device[ch].i2c;

    uint32_t i = 0;
    uint8_t addr_h = 0;

    //i2c_lock(i2c);

    if (i2c_start(i2c) == LCR_OK)
    {
        if (i2c_device[ch].type < TYPE_AT24C32)
        {
            addr_h = (addr >> 8) << 1;
        }

        if (i2c_write_wack(i2c, i2c_device[ch].addr | addr_h) == LCR_OK)                                          // 器件地址，写
        {
            if ((i2c_device[ch].type < TYPE_AT24C32) ? 1 : (i2c_write_wack(i2c, addr >> 8) == LCR_OK))      // 高地址
            {
                if (i2c_write_wack(i2c, addr & 0xff) == LCR_OK)                                                   // 低地址
                {
                    for (i = 0; i < len; i++)
                    {
                        if (i != 0 && ((addr + i) % _page_size[i2c_device[ch].type]) == 0)
                        {
                            //DEBUG(" 0x%04x %d", addr+i, i);

                            break; // 页写时遇到页尾(下页页首)则此次写入操作结束
                        }

                        if (i2c_write_wack(i2c, *((char *)buf + i)) != LCR_OK)
                        {
                            //TASK_LOG(USE_DEBUG_ERROR, ("%s write error : write data\r\n", i2c_device[ch].name));
														printf("%s write error : write data\r\n", i2c_device[ch].name);
                            break; // 写入错误则此次写入操作结束
                        }
                    }
                }
                else
                {
                    //TASK_LOG(USE_DEBUG_ERROR, ("%s write error : low address\r\n", i2c_device[ch].name));
										printf("%s write error : low address\r\n", i2c_device[ch].name);
                }
            }
            else
            {
                //TASK_LOG(USE_DEBUG_ERROR, ("%s write error : high address\r\n", i2c_device[ch].name));
								printf("%s write error : high address\r\n", i2c_device[ch].name);
            }
        }
        else
        {
            //TASK_LOG(USE_DEBUG_ERROR, ("%s write error : device address %02x\r\n", i2c_device[ch].name, i2c_device[ch].addr | addr_h));
						printf("%s write error : device address %02x\r\n", i2c_device[ch].name, i2c_device[ch].addr | addr_h);
        }

        i2c_stop(i2c);


        delay_ms(10);       // 等待写入完成 10ms

    }

    return i;
}

/* public fuction */
lcr_err_t i2c_eeprom_init(uint8_t ch)
{
    if (i2c_device[ch].initialize == 1)
    {
        return LCR_OK;
    }

    if (ch == I2C_EEPROM_1)
    {
        i2c_device[ch].i2c = I2C_1;
        i2c_device[ch].addr = 0xa0;
        i2c_device[ch].type = TYPE_AT24C256;
        i2c_device[ch].name = "eeprom_1";
    }
    else if (ch == I2C_EEPROM_2)
    {
        i2c_device[ch].i2c = I2C_2;
        i2c_device[ch].addr = 0xa0;
        i2c_device[ch].type = TYPE_AT24C02;
        i2c_device[ch].name = "eeprom_2";
    }
    else
    {
        return LCR_NODEV;
    }

    if (i2c_init(i2c_device[ch].i2c) == LCR_OK)
    {
        delay_ms(100);
        i2c_device[ch].detect = (_detect(ch) == LCR_OK);

        if (i2c_device[ch].detect != 1)
        {
            delay_ms(100);
            i2c_device[ch].detect = (_detect(ch) == LCR_OK);
        }

        if (i2c_device[ch].detect == 1)
        {
            i2c_device[ch].initialize = 1;

            //TASK_LOG(USE_DEBUG_INIT, ("initialize %s\r\n", i2c_device[ch].name));
						printf("initialize %s\r\n", i2c_device[ch].name);
            return LCR_OK;
        }
    }

    return LCR_ERROR;
}


lcr_err_t i2c_eeprom_close(uint8_t ch)
{


    if (i2c_device[ch].initialize == 1)
    {
        i2c_device[ch].initialize = 0;
        i2c_device[ch].detect = 0;
    }
	
    return LCR_OK;
}


uint32_t i2c_eeprom_read(uint8_t ch, uint32_t addr, void *buf, uint32_t len)
{
    if (i2c_device[ch].detect != 1)
    {
        return 0;
    }

    _read_bytes(ch, addr, buf, len);

    return len;
}
uint32_t i2c_eeprom_write(uint8_t ch, uint32_t addr, const void *buf, uint32_t len)
{
    char *p = (char *)buf;
    uint32_t byte_remain = len;
    uint32_t byte_write = 0;
    uint32_t addr_offset = 0;

    if (i2c_device[ch].detect != 1)
    {
        return 0;
    }

    do
    {
        byte_write = _write_bytes(ch, addr + addr_offset, p + addr_offset, byte_remain);

        byte_remain -= byte_write;
        addr_offset += byte_write;

        if (byte_write == 0)
        {
            break;
        }
    }
    while (byte_remain > 0);

    return (len - byte_remain);
}

lcr_err_t i2c_eeprom_clear(uint8_t ch)
{
    uint16_t addr;
    uint16_t len;
    uint8_t buf[8];

    if (i2c_device[ch].detect != 1)
    {
        return LCR_ERROR;
    }

    memset(buf, 0xff, sizeof(buf));

    addr = 0x0000;
    len = sizeof(buf);

    do
    {
        if (i2c_eeprom_write(ch, addr, buf, len) == len)
        {
            addr += len;
        }
        else
        {
            return LCR_ERROR;
        }
    }
    while (addr < _size[i2c_device[ch].type]);

    return LCR_OK;
}
