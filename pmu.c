#include    <stdio.h>
#include    <stdlib.h>
#include    <stdbool.h>
#include    <unistd.h>
#include    <string.h>
#include    "zdebug.h"
#include    "i2c.h"

#define msleep(n)   usleep((n) * 1000)

// configure
#define WLAN_USE_DCDC
#define WLAN_FOR_CTA
#define WLAN_USE_CRYSTAL


/*! I2C Device 地址 !*/
#define PMU_ADDR        0x14
/*! I2C Device内部寄存器地址 !*/
#define PAGE_REG        0x3f
#define CHIP_ID_REG     0x20
#define VERSION_REG     0x21

static struct {
    uint16_t chip_id;
    uint16_t wlan_version;
}pmu;

typedef struct {
    uint8_t     reg;
    uint16_t    value;
}__RF; 
static const __RF wifi_off_data[] = {
    { 0x3F, 0x0001 }, //page up
    { 0x31, 0x0B40 }, //power off wifi
    { 0x3F, 0x0000 }, //page down
};
static const __RF wifi_en_data[] = {
    //item:VerD_wf_on_2012_02_08
    {0x3f, 0x0001},
#ifdef WLAN_USE_DCDC     /*houzhen update Mar 15 2012 */
    {0x23, 0x8FA1},//20111001 higher AVDD voltage to improve EVM to 0x8f21 download current -1db 0x8fA1>>0x8bA1   
#else
    {0x23, 0x0FA1},
#endif
    {0x31, 0x0B40 }, //power off wifi
    //    {0x22, 0xD3C7},//for ver.c 20111109, txswitch
    {0x24, 0x80C8},//freq_osc_in[1:0]00  0x80C8 >> 0x80CB
    {0x27, 0x4925},//for ver.c20111109, txswitch
    //                {0x28, 0x80A1}, //BT_enable 
    {0x31, 0x8140},//enable wifi  
    {0x32, 0x0113},//set_ rdenout_ldooff_wf=0, rden4in_ldoon_wf=1						
    //                {0x39, 0x0004}, 	//uart switch to wf  
    {0x3F, 0x0000}, //page down
};
static const __RF    wifi_dc_cal_data[] = {
    {0x3f, 0x0000},
    {0x30, 0x0248},
    {0x30, 0x0249},
    //{wait 200ms, } here
};
static const __RF    wifi_dig_reset_data[] = {
    {0x3F,  0x0001},
    {0x31,  0x8D40},
    {0x31,  0x8F40},
    {0x31,  0x8b40},
    {0x3F,  0x0000},
};
static const __RF    wifi_32k_reset_data[] = {
    {0x3F,  0x0001},
    {0x33,  0x2504},
    {0x33,  0x0504},
    {0x3F,  0x0000},
};
static const __RF    wifi_rf_init_data_verE[] = {
    {0x3f, 0x0000},
    //{,,set_rf_swi},ch
    {0x06, 0x0101},
    {0x07, 0x0101},
    {0x08, 0x0101},
    {0x09, 0x3040},
    {0x0A, 0x002C},//aain_0
    {0x0D, 0x0507},
    {0x0E, 0x2300},
    {0x0F, 0x5689},//
    //{,,//set_RF  },
    {0x10, 0x0f78},//20110824
    {0x11, 0x0602},
    {0x13, 0x0652},//adc_tuning_bit[011]
    {0x14, 0x8886},
    {0x15, 0x0990},
    {0x16, 0x049f},
    {0x17, 0x0990},
    {0x18, 0x049F},
    {0x19, 0x3C01},
    {0x1C, 0x0934},
    {0x1D, 0xFF00},//for ver.D20120119for temperature 70 degree
    //{0x1F, 0x01F8},//for ver.c20111109
    //{0x1F, 0x0300},//for burst tx 不锁
    {0x20, 0x06E4},
    {0x21, 0x0ACF},//for ver.c20111109,dr dac reset,dr txflt reset
    {0x22, 0x24DC},
#ifdef WLAN_WLAN_CTA
    {0x23, 0x03FF},
#else
    {0x23, 0x0BFF},
#endif
    {0x24, 0x00FC},
    {0x26, 0x004F},//004F >> 005f premote pa 
    {0x27, 0x171D},///mdll*7
    {0x28, 0x031D},///mdll*7
    {0x2A, 0x2860},//et0x2849-8.5p  :yd 0x2861-7pf C1,C2=6.8p
    {0x2B, 0x0800},//bbpll,or ver.c20111116
    {0x32, 0x8a08},
    {0x33, 0x1D02},//liuyanan
    //{,,//agc_gain},
#if 1	
    {0x36, 0x02f4}, //00F8,//gain_7
    {0x37, 0x01f4}, //0074,//aain_6
    {0x38, 0x21d4}, //0014,//gain_5
    {0x39, 0x25d4}, //0414,//aain_4
    {0x3A, 0x2584}, //1804,//gain_3
    {0x3B, 0x2dc4}, //1C04,//aain_2
    {0x3C, 0x2d04}, //1C02,//gain_1
    {0x3D, 0x2c02}, //3C01,//gain_0
#else
    {0x36, 0x01f8}, //00F8,//gain_7
    {0x37, 0x01f4}, //0074,//aain_6
    {0x38, 0x21d4}, //0014,//gain_5
    {0x39, 0x2073}, //0414,//aain_4
    {0x3A, 0x2473}, //1804,//gain_3
    {0x3B, 0x2dc7}, //1C04,//aain_2
    {0x3C, 0x2d07}, //1C02,//gain_1
    {0x3D, 0x2c04}, //3C01,//gain_0
#endif	
    {0x33, 0x1502},//liuyanan
    //{,,SET_channe},_to_11
    {0x1B, 0x0001},//set_channel   
    {0x30, 0x024D},
    {0x29, 0xD468},
    {0x29, 0x1468},
    {0x30, 0x0249},
    {0x3f, 0x0000},
};
static const __RF    wifi_rf_init_data[] = {
    {0x3f, 0x0000},
    //{,,set_rf_swi},ch
    {0x06, 0x0101},
    {0x07, 0x0101},
    {0x08, 0x0101},
    {0x09, 0x3040},
    {0x0A, 0x002C},//aain_0
    {0x0D, 0x0507},
    {0x0E, 0x2300},//2012_02_20  
    {0x0F, 0x5689},//
    //{,,//set_RF  },
    {0x10, 0x0f78},//20110824
    {0x11, 0x0602},
    {0x13, 0x0652},//adc_tuning_bit[011]
    {0x14, 0x8886},
    {0x15, 0x0990},
    {0x16, 0x049f},
    {0x17, 0x0990},
    {0x18, 0x049F},
    {0x19, 0x3C01},//sdm_vbit[3:0]=1111
    {0x1C, 0x0934},
    {0x1D, 0xFF00},//for ver.D20120119for temperature 70 degree 0xCE00 >> 0xFF00
    {0x1F, 0x0300},//div2_band_48g_dr=1,div2_band_48g_reg[8:0]
    {0x20, 0x06E4},
    {0x21, 0x0ACF},//for ver.c20111109,dr dac reset,dr txflt reset
    {0x22, 0x24DC},
#ifdef WLAN_WLAN_CTA
    {0x23, 0x03FF},
#else
    {0x23, 0x0BFF},
#endif
    {0x24, 0x00FC},
    {0x26, 0x004F},//004F >> 005f premote pa 
    {0x27, 0x171D},///mdll*7
    {0x28, 0x031D},///mdll*7
    {0x2A, 0x2860},//et0x2849-8.5p  :yd 0x2861-7pf
    {0x2B, 0x0800},//bbpll,or ver.c20111116
    {0x32, 0x8a08},
    {0x33, 0x1D02},//liuyanan
    //{,,//agc_gain},
#if 1	
    {0x36, 0x02f4}, //00F8,//gain_7
    {0x37, 0x01f4}, //0074,//aain_6
    {0x38, 0x21d4}, //0014,//gain_5
    {0x39, 0x25d4}, //0414,//aain_4
    {0x3A, 0x2584}, //1804,//gain_3
    {0x3B, 0x2dc4}, //1C04,//aain_2
    {0x3C, 0x2d04}, //1C02,//gain_1
    {0x3D, 0x2c02}, //3C01,//gain_0
#else
    {0x36, 0x01f8}, //00F8,//gain_7
    {0x37, 0x01f4}, //0074,//aain_6
    {0x38, 0x21d4}, //0014,//gain_5
    {0x39, 0x2073}, //0414,//aain_4
    {0x3A, 0x2473}, //1804,//gain_3
    {0x3B, 0x2dc7}, //1C04,//aain_2
    {0x3C, 0x2d07}, //1C02,//gain_1
    {0x3D, 0x2c04}, //3C01,//gain_0
#endif	
    {0x33, 0x1502},//liuyanan
    //{,,SET_channe},_to_11
    {0x1B, 0x0001},//set_channel   
    {0x30, 0x024D},
    {0x29, 0xD468},
    {0x29, 0x1468},
    {0x30, 0x0249},
    {0x3f, 0x0000},
};
static const __RF    wifi_uart_debug_data[] = {
    {0x3F,0x0001},
    {0x28,0x80A1}, //BT_enable 
    {0x39,0x0004}, //uart switch to wf
    {0x3f,0x0000},
};
static const __RF    wifi_tm_en_data[] = {
    {0x3F,0x0001},
#ifdef WLAN_USE_DCDC     /*houzhen update Mar 15 2012 */
    {0x23, 0x8FA1},//20111001 higher AVDD voltage to improve EVM to 0x8f21 download current -1db 0x8fA1>>0x8bA1   
#else
    {0x23, 0x0FA1},
#endif
    {0x22,0xD3C7},//for ver.c 20111109, tx
    {0x24, 0x80C8},//freq_osc_in[1:0]00  0x80C8 >> 0x80CB 
    {0x27,0x4925},//for ver.c20111109, txs
    {0x28,0x80A1}, //BT_enable            
    {0x29,0x111F},                        
    {0x31,0x8140},                        
    {0x32,0x0113},//set_ rdenout_ldooff_wf
    {0x39,0x0004},//uart switch to wf
    {0x3f,0x0000},
};
static const __RF    wifi_tm_rf_init_data[] = {
    {0x3f, 0x0000},
    //set_rf_switch                                                  
    {0x06,0x0101},                                                     
    {0x07,0x0101},                                                     
    {0x08,0x0101},                                                     
    {0x09,0x3040},                                                     
    {0x0A,0x002C},//aain_0   
    {0x0D, 0x0507},                                          
    {0x0E,0x2300},//2012_02_20                                         
    {0x0F,0x5689},//                                                   
    //set_RF                                                            
    {0x10,0x0f78},//20110824                                             
    {0x11,0x0602},                                                     
    {0x13,0x0652},//adc_tuning_bit[011]                               
    {0x14,0x8886},                                                     
    {0x15,0x0990},                                                     
    {0x16,0x049f},                                                     
    {0x17,0x0990},                                                     
    {0x18,0x049F},                                                     
    {0x19,0x3C01},//sdm_vbit[3:0]=1111                                 
    {0x1C,0x0934},                                                     
    {0x1D,0xFF00},//for ver.D20120119for temperature 70 degree         
    {0x1F,0x0300},//div2_band_48g_dr=1,div2_band_48g_reg[8:0]1000000000
    {0x20,0x06E4},                                                     
    {0x21,0x0ACF},//for ver.c20111109,dr dac reset,dr txflt reset      
    {0x22,0x24DC},                                                     
#ifdef WLAN_WLAN_CTA
    {0x23, 0x03FF},
#else
    {0x23, 0x0BFF},
#endif
    {0x24,0x00FC},                                                     
    {0x26,0x004F},                                                     
    {0x27,0x171D},///mdll*7                                            
    {0x28,0x031D},///mdll*7                                            
    {0x2A,0x2860},                                                     
    {0x2B,0x0800},//bbpll,or ver.c20111116                             
    {0x32,0x8a08},                                                     
    {0x33,0x1D02},//liuyanan                                           
    //agc_gain                                                          
#if 1	
    {0x36, 0x02f4}, //00F8,//gain_7
    {0x37, 0x01f4}, //0074,//aain_6
    {0x38, 0x21d4}, //0014,//gain_5
    {0x39, 0x25d4}, //0414,//aain_4
    {0x3A, 0x2584}, //1804,//gain_3
    {0x3B, 0x2dc4}, //1C04,//aain_2
    {0x3C, 0x2d04}, //1C02,//gain_1
    {0x3D, 0x2c02}, //3C01,//gain_0
#else
    {0x36, 0x01f8}, //00F8,//gain_7
    {0x37, 0x01f4}, //0074,//aain_6
    {0x38, 0x21d4}, //0014,//gain_5
    {0x39, 0x2073}, //0414,//aain_4
    {0x3A, 0x2473}, //1804,//gain_3
    {0x3B, 0x2dc7}, //1C04,//aain_2
    {0x3C, 0x2d07}, //1C02,//gain_1
    {0x3D, 0x2c04}, //3C01,//gain_0
#endif	                                  
    {0x30,0x0248},                                                     
    {0x30,0x0249},                                                     
    //wait 200ms,                                                       
    {0x33,0x1502},//liuyanan                                           
    //SET_channel_to_11                                                 
    {0x1B,0x0001},//set_channel     
    {0x3f,0x0000},
};

static inline int readChip(void *i2c,uint16_t *chip_id,uint16_t *version){
    int rv;
    rv = sendI2cWord(i2c,PAGE_REG,1);
    if(rv){
        warning("select page (%d) failed",1);
        goto failed;
    }
    *chip_id = recvI2cWord(i2c,CHIP_ID_REG);
    *version = recvI2cWord(i2c,VERSION_REG);
failed:
    return rv;
}

static int wifi_setup_A2_power(void *i2c,bool enable){
    int         rv;
    uint16_t    temp_data = 0;
    uint16_t    chip_id = 0,rf_version = 0;
    rv = sendI2cWord(i2c,PAGE_REG,0x0001);
    if(rv){
        warning("select page 0x0001 failed");
        goto failed;
    }
    if(enable){
        temp_data = recvI2cWord(i2c,0x22);
        message("*** 0xA2 readback value enable : 0x%X",temp_data);
        temp_data |= 0x0200;        /*! enable reg4_page bit !*/
#ifdef  WLAN_USE_CRYSTAL
        temp_data &= ~(1 << 14);    /*! disable xen_out !*/
#endif
        rv = sendI2cWord(i2c,0x22,temp_data);
        if(rv){
            warning("reset value enable failed");
            goto failed;
        }
        rv = readChip(i2c,&chip_id,&rf_version);
        if(rv)
            warning("read chip id and rf version failed");
        else
            message("*** chip id : %04X,rf version : %04X",chip_id,rf_version);
        pmu.wlan_version = rf_version;
        pmu.chip_id      = chip_id;
    }else{
        temp_data = recvI2cWord(i2c,0x28);
        if(temp_data & 0x8000){
            warning("disenable wifi failed,bluetooth is on");
            rv = -1;
            goto failed;
        }
        temp_data = recvI2cWord(i2c,0x22);
        temp_data &= 0xfdff;
        rv = sendI2cWord(i2c,0x22,temp_data);
        if(rv){
            warning("disenable wifi failed,can't send command!");
            goto failed;
        }
        pmu.wlan_version = 0;
        pmu.chip_id      = 0;
    }
    rv = sendI2cWord(i2c,PAGE_REG,0x0000);
    if(rv){
        warning("can't select page");
    }
    message("*** wifi setup A2 power succeed!");
    return 0;
failed:
    warning("wifi setup A2 power failed!");
    return rv;
}

static int check_rf_id(void *i2c){
    int rv = 0;
    uint16_t rf_id;
    rv = sendI2cWord(i2c,PAGE_REG,0x00);
    if(rv){
        warning("select page failed!");
        goto failed;
    }
    rf_id = recvI2cWord(i2c,0);
    message("*** rf id : %04X",rf_id);
    if(rf_id != 0x5420){
        rv = -1;
        goto failed;
    }
    return 0;
failed:
    return rv;
}

static int wifi_enable(void *i2c){
    int rv;
    for(unsigned int i = 0;i < sizeof(wifi_en_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_en_data[i].reg,wifi_en_data[i].value);
        if(rv){
            warning("send wifi enable data failed,data index : %d",i);
            goto failed;
        }
        if(wifi_en_data[i].reg == 0x31) msleep(10);
    }
    rv = wifi_setup_A2_power(i2c,true);
    msleep(8);
    if(rv)
        goto failed;
    rv = check_rf_id(i2c);
    if(rv)
        warning("check rf id failed!");
    message("*** wifi enable succeed!");
    return 0;
failed:
    warning("*** wifi enable failed!");
    return rv;
}

static int wifi_rf_init(void *i2c){
    int rv;
    int count;
    const __RF *data = NULL;
    switch(pmu.wlan_version & 0x1f){
        case 4 ... 5:
            data = wifi_rf_init_data;
            count = sizeof(wifi_rf_init_data) / sizeof(__RF);
            break;
        case 7:
            data = wifi_rf_init_data_verE;
            count = sizeof(wifi_rf_init_data_verE) / sizeof(__RF);
            break;
        default:
            rv = -1;
            warning("unkonw version of this %04X chip",pmu.chip_id);
            goto failed;
    }
    for(int i = 0; i < count;i++){
        rv = sendI2cWord(i2c,data[i].reg,data[i].value);
        if(rv){
            warning("send wifi init data fialed!index : %d",i);
            goto failed;
        }
    }
    msleep(25);
    message("*** wifi initialization succceed!");
    return 0;
failed:
    warning("*** wifi initialization failed!");
    return rv;
}

static int wifi_dc_cal(void *i2c){
    int rv;
    for(unsigned int i = 0;i < sizeof(wifi_dc_cal_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_dc_cal_data[i].reg,wifi_dc_cal_data[i].value);
        if(rv){
            warning("send wifi dc cal data failed!index : %d",i);
            goto failed;
        }
    }
    msleep(70);
    message("*** wifi rf dc cal succceed!");
    return 0;
failed:
    warning("wifi rf dc cal failed");
    return rv;
}

static int wifi_dig_reset(void *i2c){
    int rv;
    for(unsigned int i = 0;i < sizeof(wifi_dig_reset_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_dig_reset_data[i].reg,wifi_dig_reset_data[i].value);
        if(rv){
            warning("wifi send dig reset data failed,index : %d",i);
            goto failed;
        }
    }
    msleep(28);
    message("*** wifi dig reset succceed!");
    return 0;
failed:
    warning("wifi dig reset failed!");
    return rv;
}

int wifi_power_off(void *i2c){
    int rv;
    uint16_t temp;
    rv = wifi_setup_A2_power(i2c,0);
    if(rv){
        warning("wifi power off failed!");
        goto failed;
    }
    rv = sendI2cWord(i2c,PAGE_REG,0x0001);
    if(rv){
        warning("wifi power off,select page 1 failed!");
        goto failed;
    }
    temp = recvI2cWord(i2c,0x28);   //poll bt status
    if(temp & 0x8000){
        rv = sendI2cWord(i2c,PAGE_REG,0x0000);
        if(rv){
            warning("wifi power off,select page 0 failed!");
            goto failed;
        }
        rv = sendI2cWord(i2c,0x0f,0x2223); //set antenna for bt
        if(rv){
            warning("wifi power off,set antenna for bt!");
            goto failed;
        }
    }
    for(unsigned int i = 0;i < sizeof(wifi_off_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_off_data[i].reg,wifi_off_data[i].value);
        if(rv){
            warning("wifi power off,send off data failed!");
            goto failed;
        }
    }
    message("wifi power off succceed!");
    return 0;
failed:
    warning("wifi power off failed!");
    return rv;
}

static int wifi_tm_enable(void *i2c){
    int rv;
    msleep(5);
    for(unsigned int i = 0;i < sizeof(wifi_tm_en_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_tm_en_data[i].reg,wifi_tm_en_data[i].value);
        if(rv){
            warning("send data to i2c failed!index : %d!",i);
            goto failed;
        }
    }
    msleep(28);
    message("*** WLAN enable test mode succceed!");
    return 0;
failed:
    warning("WLAN enable test mode failed!");
    return rv;
}


static int wifi_tm_rf_init(void *i2c){
    int rv;
    for(unsigned int i = 0;i < sizeof(wifi_tm_rf_init_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_tm_rf_init_data[i].reg,wifi_tm_rf_init_data[i].value);
        if(rv)
            goto failed;
    }
    message("*** WLAN test mode rd init succceed!");
    return 0;
failed:
    warning("WLAN test mode rf init failed!error code : %d!",rv);
    return rv;
}

int wifi_power_on(void *i2c){
    int rv;
    int (*const wifi_power[])(void *i2c) = {
        wifi_enable,
        wifi_rf_init,
        wifi_dc_cal,
        wifi_dig_reset,
    };
    for(unsigned int i = 0;i < sizeof(wifi_power) / sizeof(wifi_power[0]);i++){
        rv = wifi_power[i](i2c);
        if(rv) goto failed;
    }
    return 0;
failed:
    return rv;
}

int wifi_enable_debug(void *i2c){
    int rv;
    for(unsigned int i = 0;i < sizeof(wifi_uart_debug_data) / sizeof(__RF);i++){
        rv = sendI2cWord(i2c,wifi_uart_debug_data[i].reg,wifi_uart_debug_data[i].value);
        if(rv) goto failed;
    }
    return 0;
failed:
    warning("wifi can't enable debug!send i2c data failed! error code :%d!",rv);
    return rv;
}

int wifi_enable_test_mode(void *i2c){
    int rv;
    int (*tm_func[])(void *i2c) = {
        wifi_tm_enable,
        wifi_tm_rf_init,
    };
    for(unsigned int i = 0;i < sizeof(tm_func) / sizeof(tm_func[0]);i++){
        rv = tm_func[i](i2c);
        if(rv)
            goto failed;
    }
    return rv;
failed:
    warning("WLAN enable test mode fialed;!");
    return rv;
}

