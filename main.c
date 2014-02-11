#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    "i2c.h"
#include    "zdebug.h"


#define usage_if(b) ({\
        if((b)) {\
            warning("I2C Control Program\n"\
                "usage : i2c device r/w/e [addr] [data]");\
            exit(1);}})

#define die_if(b,msg) ({\
        if(b){\
            warning("msg : %s",msg);\
            warning("sysmsg : %s",strerror(errno));\
            exit(1);\
        }\
    })

int main(int argc,char **argv){
    void *i2c;
    int addr;
    int rv = 0;
    usage_if(argc < 3 || argv[2][1] != '\0');
    die_if((NULL == (i2c = newI2c(argv[1],0x14,TYPE_8BIT_ADDR))),"unable to open i2c device file");
    if(argv[2][0] == 'w' || argv[2][0] == 'r')
        addr = strtol(argv[3],NULL,0);
    switch(argv[2][0]){
        case 'r':
            {
                uint16_t rd = 0;
                rd = recvI2cWord(i2c,addr);
                printf("read address %02X value %04X\n",addr,rd);
            }
            break;
        case 'w':
            {
                uint16_t wd = strtol(argv[4],NULL,0);
                printf("write address %02x value %04X\n",addr,wd);
                rv = sendI2cWord(i2c,addr,wd);
            }
            break;
        case 's':
            {
                int wifi_power_on(void *i2c);
                rv = wifi_power_on(i2c);
            }
            break;
        case 'o':
            {
                int wifi_power_off(void *i2c);
                rv = wifi_power_off(i2c);
            }
            break;
        default:
            usage_if(1);
            break;
    }
    delI2c(i2c);
    return rv;
}
