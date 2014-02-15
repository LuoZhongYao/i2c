#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    "i2c.h"
#include    "zdebug.h"


#define usage_if(b) ({\
        if((b)) {\
            fprintf(stderr,\
                "I2C Control Program\n"\
                "usage : i2c [device] r|w|o|s|d|t  [addr] [word]\n"\
                "\tdevice         Operating device (default is /dev/i2c-0)\n"\
                "\t-r addr        read  a word form address of the deive\n"\
                "\t-w addr word   write a word to address for the device\n"\
                "\t-o             open WLAN module form device\n"\
                "\t-s             stop WLAN module form device\n"\
                "\t-d             WLAN module to enter debug mode\n"\
                "\t-t             WLAN module to enter test mode\n"\
                "\t-h             show this message(default option)\n"\
                );\
            exit(1);}})

#define die_if(b,msg) ({\
        if(b){\
            fprintf(stderr,"msg : %s",msg);\
            fprintf(stderr,"sysmsg : %s",strerror(errno));\
            exit(1);\
        }\
    })
int wifi_power_on(void *i2c);
int wifi_power_off(void *i2c);
int wifi_enable_debug(void *i2c);
int wifi_enable_test_mode(void *i2c);

#define is_option(arg)  ((arg)[0] == '-' && (arg)[1] != 0 && (arg)[2] == 0)
#define arg_char(arg) (arg[1])

int main(int argc,char **argv){
    void *i2c;
    int rv = 0;
    int arg_index = 1;
    uint16_t addr;
    uint16_t wd;
    enum{
        DEVICE,
        OPTION,
        ADDRESS,
        WORD,
    };
    const char *args[] = {
        [DEVICE]  = "/dev/i2c-0",   // device 
        [OPTION]  = "-h",           // option
        [ADDRESS] = "0x3f",         // address
        [WORD]    = "0x0001",       // word
    };
    if(arg_index < argc && !is_option(argv[arg_index]))
        args[DEVICE] = argv[arg_index++];
    
    for(int i = OPTION;arg_index < argc && i < WORD;i++,arg_index++)
        args[i] = argv[arg_index];

    usage_if(arg_char(args[OPTION]) == 'h'); 

    die_if((NULL == (i2c = newI2c(args[DEVICE],0x14,TYPE_8BIT_ADDR))),"unable to open i2c device file");
    addr = strtol(args[ADDRESS],NULL,0);
    wd   = strtol(args[WORD],NULL,0);
    switch(arg_char(args[OPTION])){
        case 'r':
            wd = recvI2cWord(i2c,addr);
            printf("read address %02X value %04X\n",addr,wd);
            break;
        case 'w':
            printf("write address %02x value %04X\n",addr,wd);
            rv = sendI2cWord(i2c,addr,wd);
            break;
        case 'o':
            rv = wifi_power_on(i2c);
            break;
        case 's':
            rv = wifi_power_off(i2c);
            break;
        case 'd':
            rv = wifi_enable_debug(i2c);
            break;
        case 't':
            rv = wifi_enable_test_mode(i2c);
        default:
            rv = wifi_power_on(i2c);
            break;
    }
    delI2c(i2c);
    return rv;
}
