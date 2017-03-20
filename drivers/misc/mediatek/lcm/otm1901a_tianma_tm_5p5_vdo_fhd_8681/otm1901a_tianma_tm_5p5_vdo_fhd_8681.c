#ifndef BUILD_LK
#include <linux/string.h>
#endif


#include "lcm_drv.h"
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
    #include <platform/mt_i2c.h>
    #include <string.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (1080)
#define FRAME_HEIGHT                                        (1920)

#define REGFLAG_DELAY                                       0xFE
#define REGFLAG_END_OF_TABLE                                0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()

extern int tps65132_write_bytes(unsigned char addr, unsigned char value);

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 1, {0xFF}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static int transform[] = {
    255,253,252,250,249,247,246,244,243,241,
    240,238,237,236,234,233,231,230,228,227,
    225,224,222,221,219,218,217,215,214,212,
    211,209,208,206,205,203,202,200,199,198,
    196,195,193,192,190,189,187,186,184,183,
    181,180,179,177,176,174,173,171,170,168,
    167,165,164,163,161,160,158,157,155,154,
    152,151,149,148,146,145,144,142,141,139,
    138,136,135,133,132,130,129,127,126,125,
    124,123,122,121,120,119,118,117,116,115,
    114,113,112,111,109,107,105,103,101,99,
    97, 95, 93, 91, 89, 87, 85, 83, 81, 79,
    77, 75, 73, 71, 69, 67, 65, 63, 61, 59,
    57, 56, 55, 54, 53, 52, 51, 50, 49, 47,
    46, 45, 44, 43, 42, 41, 40, 39, 38, 36,
    35, 34, 33, 33, 33, 32, 32, 32, 32, 31, //105
    31, 31, 30, 30, 30, 30, 29, 29, 29, 28,
    28, 28, 27, 27, 27, 27, 26, 26, 26, 25,
    25, 25, 25, 24, 24, 24, 23, 23, 23, 23,
    22, 22, 22, 21, 21, 21, 21, 20, 20, 20,
    19, 19, 19, 19, 18, 18, 18, 17, 17, 17, //55
    17, 16, 16, 16, 15, 15, 15, 15, 14, 14,
    14, 13, 13, 13, 13, 12, 12, 12, 11, 11,
    11, 11, 10, 10, 10, 9, 9, 8, 8, 7,
    6, 5, 4, 4, 4, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 0
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
        memset(params, 0, sizeof(LCM_PARAMS));

        params->type   = LCM_TYPE_DSI;

        params->width  = FRAME_WIDTH;
        params->height = FRAME_HEIGHT;

        #if (LCM_DSI_CMD_MODE)
        params->dsi.mode   = CMD_MODE;
        #else
        params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
        #endif

        // DSI
        /* Command mode setting */
        //1 Three lane or Four lane
        params->dsi.LANE_NUM                = LCM_FOUR_LANE;
        //The following defined the fomat for data coming from LCD engine.
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        // Video mode setting
        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

        params->dsi.vertical_sync_active                = 2;// 3    2
        params->dsi.vertical_backporch                  = 8;// 20   1
        params->dsi.vertical_frontporch                 = 14; // 1  12
        params->dsi.vertical_active_line                = FRAME_HEIGHT;
//EMI Setting Modified By gongrui@yulong.com 7-01 Start
        params->dsi.horizontal_sync_active              = 20;// 10  2
        params->dsi.horizontal_backporch                = 32; //25;//100;
        params->dsi.horizontal_frontporch               = 31; //24;//86;
        params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

        params->physical_width = 68;
        params->physical_height = 121;

        params->dsi.PLL_CLOCK=  416; //410;
//EMI Setting Modified By gongrui@yulong.com 7-01 End
        params->dsi.ssc_disable = 1;

        //add TE signal for esd-check gongrui@yulong.com 5.11
        params->dsi.esd_check_enable=1;
        params->dsi.customization_esd_check_enable=0;
        /*relative with ssc
            //params->dsi.pll_div1=0;
            //params->dsi.pll_div2=1;
            //params->dsi.fbk_div=17;
        */

        /*begin add by liwei11 for esd detection and recovery

        params->dsi.esd_check_enable=1;
        params->dsi.customization_esd_check_enable=1;
        params->dsi.lcm_esd_check_table[0].cmd=0x0A;
        params->dsi.lcm_esd_check_table[0].count=1;
        params->dsi.lcm_esd_check_table[0].para_list[0]=0x9C;

        end add by liwei11 for esd detection and recovery*/


}

static void init_lcm_registers(void)
{
    unsigned int data_array[10];

    //0xFF00 £¨0x19£¬0x01£¬0x01£©
    //0x19£¬0x01 enter Command 2 Mode
    //0x01 set SW_EXTC=1 enable write function of command 2 & parameter shift function
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x010119FF;
    dsi_set_cmdq(data_array, 2, 1);

    //0xFF80 (0x19,0x01)
    //enter in Orise Command 2 Mode
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000119FF;
    dsi_set_cmdq(data_array, 2, 1);
//EMI Setting begin gongrui@yulong.com Start 7-1
    //SAP
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05C42300;
    dsi_set_cmdq(data_array, 1, 1);
    //VGH
    data_array[0] = 0x92002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x80C52300;
    dsi_set_cmdq(data_array, 1, 1);
    //VGL
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x80C52300;
    dsi_set_cmdq(data_array, 1, 1);
    //150613
    //pump clkratio
    data_array[0] = 0x9B002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x442022C5;
    dsi_set_cmdq(data_array, 2, 1);

    //CKH EQ
    data_array[0] = 0xF7002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052902;
    data_array[1] = 0x001604C3;
    data_array[2] = 0x00000000;
    dsi_set_cmdq(data_array, 3, 1);

    //Slew-Rate Control
    data_array[0] = 0xF2002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x000080C1;
    dsi_set_cmdq(data_array, 2, 1);

    //Source Pre-charge
    data_array[0] = 0x81002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x06A52300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x82002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xF0C42300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x84002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x22C42300;
    dsi_set_cmdq(data_array, 1, 1);

    //Enable AVDDR/AVEER Regulator
    data_array[0] = 0xD0002300;
    dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x00052902;
    data_array[1] = 0x061106F5;
    data_array[2] = 0x00000011;
    dsi_set_cmdq(data_array, 3, 1);

    //VCL Clamp Level
    data_array[0] = 0x83002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x80C52300;
    dsi_set_cmdq(data_array, 1, 1);

    //AVDDR Regulator Output Level
    data_array[0] = 0x84002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9aC52300;
    dsi_set_cmdq(data_array, 1, 1);

    //AVEER Regulator Output Level
    data_array[0] = 0x85002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9aC52300;
    dsi_set_cmdq(data_array, 1, 1);
//EMI Setting End gongrui@yulong.com End 7-1

    //0x1C00 (0x33)
    //VMM=1  set video mode by register setting
    //VME=1  set driver IC mode as video mode
    //RAMZIP_OPT[2:0]=b011   no RAM compression, no display decompression
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00022902;
    data_array[1] = 0x0000331C;
    dsi_set_cmdq(data_array, 2, 1);

    //CE setting
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2902;
    data_array[1] = 0x000103D6;
    data_array[2] = 0xFD000000;
    data_array[3] = 0x06060300;
    data_array[4] = 0x00000002;
    dsi_set_cmdq(data_array, 5, 1);

    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2902;
    data_array[1] = 0x660000D6;
    data_array[2] = 0xCDB3CDB3;
    data_array[3] = 0xCDB3A6B3;
    data_array[4] = 0x000000B3;
    dsi_set_cmdq(data_array, 5, 1);

    data_array[0] = 0xC0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2902;
    data_array[1] = 0x890037D6;
    data_array[2] = 0x89778977;
    data_array[3] = 0x89776F77;
    data_array[4] = 0x00000077;
    dsi_set_cmdq(data_array, 5, 1);

    data_array[0] = 0xD0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2902;
    data_array[1] = 0x443C37D6;
    data_array[2] = 0x443C443C;
    data_array[3] = 0x443C373C;
    data_array[4] = 0x0000003C;
    dsi_set_cmdq(data_array, 5, 1);
    //CE end

    //EMI test start gongrui@yulong.com 7-01 Start
    //150616
    //gvdd ngvdd
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x002f2fD8;
    dsi_set_cmdq(data_array, 2, 1);
    //DG-off
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03D72300;
    dsi_set_cmdq(data_array, 1, 1);
    //VDD18&LVDSVDD
    data_array[0] = 0xC1002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x34C52300;
    //data_array[0] = 0x33C52300;
    dsi_set_cmdq(data_array, 1, 1);
    //MIPI bias
    data_array[0] = 0xB7002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x12B02300;
    dsi_set_cmdq(data_array, 1, 1);

    //ssc modify by tim 20150617
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84C12300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x98002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x74C12300;
    dsi_set_cmdq(data_array, 1, 1);

    //MIPI OSC Disable
    data_array[0] = 0x92002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xDFB02300;
    dsi_set_cmdq(data_array, 1, 1);

// Add Code by tim 150617 ext4
    //chop off, calibration on (GAMMA)
    data_array[0] = 0x92002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00E92300;
    dsi_set_cmdq(data_array, 1, 1);

    //MIPI Speed Up Disable
    data_array[0] = 0xB6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05B02300;
    dsi_set_cmdq(data_array, 1, 1);

    //MIPI Error RPT Disable
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00B02300;
    dsi_set_cmdq(data_array, 1, 1);

    //MIPI High Speed CLK div lowest setting
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xFFC12300;
    dsi_set_cmdq(data_array, 1, 1);
// Modify end by tim

    //CKH xCKH delay
    data_array[0] = 0xFB002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00C32300;
    dsi_set_cmdq(data_array, 1, 1);
    //R floating
    data_array[0] = 0xE6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77CB2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0xC6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77CB2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0xF6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77CB2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0xD6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77CB2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x93002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072902;
    data_array[1] = 0x262626CD;
    data_array[2] = 0x00262626;
    dsi_set_cmdq(data_array, 3, 1);

    //latech load data
    data_array[0] = 0x95002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9eC42300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01C42300;
    dsi_set_cmdq(data_array, 1, 1);
    //cache off
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x0000ffA6;
    dsi_set_cmdq(data_array, 2, 1);
    //EMI test start gongrui@yulong.com 7-01 End
    //0xFF00 (0xFF,0xFF,0xFF)
    //
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0xFFFFFFFF;
    dsi_set_cmdq(data_array, 2, 1);
    MDELAY(10);

    //disable CE function
    data_array[0] = 0x00812300;
    dsi_set_cmdq(data_array, 1, 1);
    //enable TE signal for esd-check gongrui@yulong.com 5.11

    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);

    //sleep out
    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    //display on
    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
}


static void lcm_init_power(void)
{

    lcm_util.set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_PWR_EN,1);
    MDELAY(100);
}

static void lcm_init(void)
{
    int ret=0;
    unsigned char cmd[2];
    unsigned char data[2];

    cmd[0]=0x00;
    data[0]=0x0F;

    cmd[1]=0x01;
    data[1]=0x0F;

#ifdef BUILD_LK
    ret=TPS65132_write_byte(cmd[0],data[0]);
    if(ret)
    printf("[LK]nt35595----tps6132----cmd=%0x--i2c write +5.5V error----\n",cmd[0]);
    else
    printf("[LK]nt35595----tps6132----cmd=%0x--i2c write +5.5V success----\n",cmd[0]);

    ret=TPS65132_write_byte(cmd[1],data[1]);
    if(ret)
    printf("[LK]nt35595----tps6132----cmd=%0x--i2c write -5.5V error----\n",cmd[1]);
    else
    printf("[LK]nt35595----tps6132----cmd=%0x--i2c write -5.5V success----\n",cmd[1]);
#else
    ret=tps65132_write_bytes(cmd[0],data[0]);
    if(ret<0)
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write +5.5v error-----\n",cmd[0]);
    else
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write +5.5V success-----\n",cmd[0]);

    ret=tps65132_write_bytes(cmd[1],data[1]);
    if(ret<0)
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write -5.5v error-----\n",cmd[1]);
    else
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write -5.5V success-----\n",cmd[1]);
#endif
    MDELAY(20);

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    init_lcm_registers();
}


static void lcm_suspend(void)
{
    unsigned int data_array[16];

    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(80);

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    lcm_util.set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_PWR_EN,0);
    MDELAY(30);  //recommand 10ms (+50ms)  300ms

}


static void lcm_resume_power(void)
{
#ifndef BUILD_LK
    int ret=0;
    unsigned char cmd[2];
    unsigned char data[2];


    lcm_init_power();
    MDELAY(20);
    printk("[KERNEL]nt35595----tps6132---lcm power en-----\n");

// modified tianma IC for AVDD/AVEE to 5.5V gongrui@yulong.com begin
    cmd[0]=0x00;
    data[0]=0x0F;

    cmd[1]=0x01;
    data[1]=0x0F;
// modified tianma IC for AVDD/AVEE to 5.5V gongrui@yulong.com end

    ret=tps65132_write_bytes(cmd[0],data[0]);
    if(ret<0)
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write +5.5v error-----\n",cmd[0]);
    else
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write +5.5V success-----\n",cmd[0]);

    ret=tps65132_write_bytes(cmd[1],data[1]);
    if(ret<0)
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write -5.5v error-----\n",cmd[1]);
    else
    printk("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write -5.5V success-----\n",cmd[1]);

    MDELAY(20);
#endif
}

static void lcm_resume(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    init_lcm_registers();
}

static void lcm_setbacklight(unsigned int level)
{
    unsigned int default_level = 0;//145;
    unsigned int mapped_level = 0;

    //for LGE backlight IC mapping table
    if(level > 255)
            level = 255;

    if(level >0)
            mapped_level = (unsigned char)transform[255-level];
    else
            mapped_level=0;

    /*  maybe set the min brightness to 180=0xb4
    if (mapped_level < 180)
    {
        mapped_level = 180;
    }*/

    #ifdef BUILD_LK
        printf("%s, LK backlight = 0x%x\n", __func__, mapped_level);
    #else
        printk("%s, kernel backlight = 0x%x\n", __func__, mapped_level);
    #endif

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}


static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[5];
    unsigned int array[16];

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(20);

    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);

    array[0] = 0x00013700;// read id return 5 byte
    dsi_set_cmdq(array, 1, 1);
    MDELAY(20);
    read_reg_v2(0xda, buffer, 1);
    id = buffer[0]; //we only need ID
    #ifdef BUILD_LK
        printf("%s, LK LCD IC id = 0x%x\n", __func__, id);
    #else
        printk("%s, kernel LK LCD IC id = 0x%x\n", __func__, id);
    #endif
    if(id==0x30)
        return 1;
    else
        return 0;
}

static void lcm_vddi_power_on(void)
{
#ifdef BUILD_LK
    printf("lk  lcm_vddi_power_on\n");
#else
    printk("kernel lcm_vddi_power_on\n");
#endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ONE);
    MDELAY(10);
}
static void lcm_vddi_power_off(void)
{
#ifdef BUILD_LK
    printf("lk  lcm_vddi_power_off\n");
#else
    printk("kernel lcm_vddi_power_off\n");
#endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ZERO);
    MDELAY(10);
}
static void lcm_vddi_power_init(void)
{
#ifdef BUILD_LK
    printf("lk  lcm_vddi_power_init\n");
#else
    printk("kernel lcm_vddi_power_init\n");
#endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ONE);
}
LCM_DRIVER otm1901a_dsi_vdo_tianma_tm_lcm_drv =
{
    .name           = "otm1901A_tianma_tm",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id    = lcm_compare_id,
    .init_power    = lcm_init_power,
    .resume_power  = lcm_resume_power,
    //.esd_check      =lcm_esd_check,
    //.esd_recover    =lcm_esd_recover,
    .set_backlight  = lcm_setbacklight,
    .vddi_power_on  = lcm_vddi_power_on,
    .vddi_power_off  = lcm_vddi_power_off,
    .vddi_power_init = lcm_vddi_power_init,
#if (LCM_DSI_CMD_MODE)
    //.set_pwm        = lcm_setpwm,
    //.get_pwm        = lcm_getpwm,
    .update         = lcm_update
#endif
};
