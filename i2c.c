#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/ioctl.h>
#include    <unistd.h>
#include    <linux/i2c-dev.h>
#include    <linux/i2c.h>
#include    <linux/fs.h>
#include    <fcntl.h>
#include    <errno.h>
#include    "i2c.h"
#include    "zdebug.h"

#define SWAP(a,b)   ({ a ^= b;b ^= a;a ^= b;})
#define SWAP_WORD(x) (((x >> 8) & 0xff) | ((x << 8) & 0xff00))

/*! i2c ioctl !*/
static inline int32_t   i2c_smbus_access(int file,char read_write,uint8_t  command,
        size_t size,union i2c_smbus_data *data){
    struct i2c_smbus_ioctl_data args = {
        .read_write = read_write,
        .command    = command,
        .size       = size,
        .data       = data,
    };
    return ioctl(file,I2C_SMBUS,&args);
}

/*! write functions !*/

static inline int32_t i2c_smbus_write_quick(int file,uint8_t value){
    return i2c_smbus_access(file,value,0,I2C_SMBUS_QUICK,NULL);
}

static inline int32_t i2c_smbus_write_byte(int file,uint8_t value){
    return i2c_smbus_access(file,I2C_SMBUS_WRITE,value,I2C_SMBUS_BYTE,NULL);
}

static inline int32_t i2c_smbus_write_byte_data(int file,uint8_t command,uint8_t value){
    union i2c_smbus_data data = {
        .byte = value,
    };
    return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,I2C_SMBUS_BYTE_DATA,&data);
}

static inline int32_t i2c_smbus_write_word_data(int file,uint8_t command,uint16_t value){
    union i2c_smbus_data data = {
        .word = value,
    };
    return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,I2C_SMBUS_WORD_DATA,&data);
}

static inline int32_t i2c_smbus_write_block_data(int file,uint8_t command,uint8_t length,uint8_t *values){
    union i2c_smbus_data data;
    if(length > 32) length = 32;
    for (int i = 1;i <=length;i++)
        data.block[i] = values[i - 1];
    data.block[0] = length;
    return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,I2C_SMBUS_BLOCK_DATA,&data);
}

static inline int32_t i2c_smbus_process_call(int file,uint8_t command,uint16_t value){
    union i2c_smbus_data data = {
        .word = value,
    };
    if(i2c_smbus_access(file,I2C_SMBUS_WRITE,command,I2C_SMBUS_PROC_CALL,&data))
        return -1;
    return 0xFFFF & data.word;
}

static inline int32_t i2c_smbus_block_process_call(int file,uint8_t command,uint8_t length,uint8_t *values){
    union i2c_smbus_data data;
    if(length > 32) length = 32;
    for (int i = 1;i <= length;i++)
        data.block[i] = values[i - 1];
    data.block[0] = length;
    if(i2c_smbus_access(file,I2C_SMBUS_WRITE,command,I2C_SMBUS_BLOCK_PROC_CALL,&data))
            return -1;
    for( int i = 1;i <= data.block[0];i++)
        values[i-1] = data.block[i];
    return data.block[0];
}


/*! read functions !*/

static inline int32_t i2c_smbus_read_byte(int file){
    union i2c_smbus_data data;
    if(i2c_smbus_access(file,I2C_SMBUS_READ,0,I2C_SMBUS_BYTE,&data))
        return -1;
    return  0xFF & data.byte;
}

static inline int32_t i2c_smbus_read_word_data(int file,uint8_t command){
    union i2c_smbus_data data;
    if(i2c_smbus_access(file,I2C_SMBUS_READ,command,I2C_SMBUS_WORD_DATA,&data))
        return -1;
    return 0xFFFF & data.word;
}

static inline int32_t i2c_smbus_read_block_data(int file,uint8_t command,uint8_t *values){
    union i2c_smbus_data data;
    if(i2c_smbus_access(file,I2C_SMBUS_READ,command,I2C_SMBUS_BLOCK_DATA,&data))
        return -1;
    for(int i = 0;i <= data.block[0];i++)
        values[i-1] = data.block[i];
    return data.block[0];
}


#define CHECK_I2C_FUNC(var,label) ({\
        if(0 == (var & label)){\
            warning("%s,function is required.Program halted.",#label);\
            exit(1);\
        }})

typedef struct {
    int file;
    int addr;
    int type;
    const char *dev;
}I2c;

void *newI2c(const char *dev,int addr,int type){
    I2c *i2c;
    int fd,rv;
    unsigned func;

    i2c = malloc(sizeof(I2c));
    if(!i2c){
        goto malloc_fail;
    }

    fd = open(dev,O_RDWR);
    if(fd < 0){
        goto open_fail;
    }
    rv = ioctl(fd,I2C_FUNCS,&func);
    if(rv < 0){
        goto ioctl_fail;
    }

    CHECK_I2C_FUNC(func,I2C_FUNC_SMBUS_READ_BYTE);
    CHECK_I2C_FUNC(func,I2C_FUNC_SMBUS_WRITE_BYTE);
    CHECK_I2C_FUNC(func,I2C_FUNC_SMBUS_READ_BYTE_DATA);
    CHECK_I2C_FUNC(func,I2C_FUNC_SMBUS_WRITE_BYTE_DATA);
    CHECK_I2C_FUNC(func,I2C_FUNC_SMBUS_READ_WORD_DATA);
    CHECK_I2C_FUNC(func,I2C_FUNC_SMBUS_WRITE_WORD_DATA);
    if((rv = ioctl(fd,I2C_SLAVE,addr)) < 0){
        goto ioctl_fail;
    }
    *i2c = (I2c){
        .file = fd,
        .addr = addr,
        .type = type,
        .dev  = dev,
    };
    return i2c;
ioctl_fail:
    close(fd);
open_fail:
    free(i2c);
malloc_fail:
    warning("newI2c %s : %s",dev,strerror(errno));
    return NULL;
}

void delI2c(void *i2c){
    I2c *_ = i2c;
    if(_){
        close(_->file);
        free(i2c);
    }else{
        warning("Invalid i2c");
    }
}


int sendI2cWord(void *i2c,uint8_t addr,uint16_t value){
    int rv = -1;
    I2c *_ = i2c;
    if(!_){
        warning("Invalid i2c");
        goto leval;
    }
    value = SWAP_WORD(value);
    return i2c_smbus_write_word_data(_->file,addr,value);
leval:
    return rv;
}

uint16_t recvI2cWord(void *i2c,uint8_t addr){
    uint16_t rv = 0;
    I2c *_ = i2c;
    if(!_){
        warning("Invalid i2c");
        goto leval;
    }
    ioctl(_->file,BLKFLSBUF);
    rv = i2c_smbus_read_word_data(_->file,addr);
    rv = SWAP_WORD(rv);
leval:
    return rv;
}
