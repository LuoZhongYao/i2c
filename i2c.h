#ifndef __I2C_H__
#define __I2C_H__
#include    <stdint.h>
#define TYPE_8BIT_ADDR  1
#define TYPE_16BIT_ADDR 2

void        *newI2c(const char * dev,int addr,int type);
void        delI2c(void *i2c);
int         sendI2cWord(void *i2c,uint8_t addr,uint16_t value);
uint16_t    recvI2cWord(void *i2c,uint8_t addr);

#endif
