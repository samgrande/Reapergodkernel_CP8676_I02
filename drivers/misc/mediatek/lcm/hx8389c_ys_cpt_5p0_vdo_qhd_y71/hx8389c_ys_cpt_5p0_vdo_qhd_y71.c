#ifndef BUILD_LK
#include <linux/string.h>
#endif


#include "lcm_drv.h"
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (540)
#define FRAME_HEIGHT                                        (960)

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
#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()


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
    114,113,112,111,110,109,108,107,106,105,
    104,103,102,101,100, 99, 98, 97, 96, 95,
    94, 93, 92, 91, 90, 89, 88, 87, 86, 85,
    84, 83, 82, 81, 80, 79, 78, 77, 76, 75,
    74, 73, 72, 71, 70, 69, 68, 68, 67, 67,
    66, 66, 65, 64, 63, 62, 61, 60, 59, 58, //105
    56, 54, 52, 50, 49, 48, 47, 46, 45, 45,
    44, 44, 43, 43, 42, 42, 41, 41, 40, 40,
    39, 39, 38, 38, 37, 36, 35, 35, 34, 33,
    32, 32, 31, 30, 29, 29, 28, 27, 27, 26,
    26, 25, 25, 24, 24, 23, 23, 22, 22, 21,
    21, 20, 20, 19, 19, 18, 18, 17, 17, 16,
    16, 15, 15, 14, 14, 13, 13, 12, 12, 11,
    11, 10, 10, 9, 9, 8, 8, 7, 7, 6,
    6, 5, 4, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 0
};
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd)
        {
            case REGFLAG_DELAY :
            {
                MDELAY(table[i].count);
                break;
            }
            case REGFLAG_END_OF_TABLE :
            {
                break;
            }
            default:
            {
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            }
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
    params->dsi.LANE_NUM                = LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Video mode setting
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active                = 2;
    params->dsi.vertical_backporch                  = 14;
    params->dsi.vertical_frontporch                 = 18;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active              = 4;
    params->dsi.horizontal_backporch                = 48;
    params->dsi.horizontal_frontporch               = 52;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->dsi.PLL_CLOCK=230;
    params->dsi.ssc_disable = 1;

    params->dsi.noncont_clock = 1;
    params->dsi.noncont_clock_period = 2;

    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;
    //params->dsi.lcm_esd_check_table[0].cmd          = 0x09;
    //params->dsi.lcm_esd_check_table[0].count        = 1;
    //params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
}


static void init_lcm_registers(void)
{
    unsigned int data_array[33];

    data_array[0] = 0x00043902;
    data_array[1] = 0x8983FFB9;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00153902;
    data_array[1] = 0x10107FB1;
    data_array[2] = 0x105032F2;
    data_array[3] = 0x208058F2;
    data_array[4] = 0xAAAAF820;
    data_array[5] = 0x308000A0;
    data_array[6] = 0x00000000;
    dsi_set_cmdq(data_array, 7, 1);
    data_array[0] = 0x000b3902;
    data_array[1] = 0x055080B2;
    data_array[2] = 0x11384007;
    data_array[3] = 0x00095D64;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x000c3902;
    data_array[1] = 0x707070B4;
    data_array[2] = 0x10000070;
    data_array[3] = 0xB0761076;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x00243902;
    data_array[1] = 0x000000D3;
    data_array[2] = 0x00080000;
    data_array[3] = 0x00001032;
    data_array[4] = 0x03C60300;
    data_array[5] = 0x000000C6;
    data_array[6] = 0x04333500;
    data_array[7] = 0x00003704;
    data_array[8] = 0x00080500;
    data_array[9] = 0x01000A00;
    dsi_set_cmdq(data_array, 10, 1);
    data_array[0] = 0x00273902;
    data_array[1] = 0x181818D5;
    data_array[2] = 0x18191918;
    data_array[3] = 0x24212018;
    data_array[4] = 0x18181825;
    data_array[5] = 0x04010018;
    data_array[6] = 0x06030205;
    data_array[7] = 0x18181807;
    data_array[8] = 0x18181818;
    data_array[9] = 0x18181818;
    data_array[10] = 0x00181818;
    dsi_set_cmdq(data_array, 11, 1);
    data_array[0] = 0x00273902;
    data_array[1] = 0x181818D6;
    data_array[2] = 0x19181818;
    data_array[3] = 0x21242519;
    data_array[4] = 0x18181820;
    data_array[5] = 0x03060718;
    data_array[6] = 0x01040502;
    data_array[7] = 0x18181800;
    data_array[8] = 0x18181818;
    data_array[9] = 0x18181818;
    data_array[10] = 0x00181818;
    dsi_set_cmdq(data_array, 11, 1);
    data_array[0] = 0x002b3902;
    data_array[1] = 0x0F0A00E0;
    data_array[2] = 0x1F3F3B35;
    data_array[3] = 0x0C0A0543;
    data_array[4] = 0x13110F17;
    data_array[5] = 0x10061312;
    data_array[6] = 0x0A001813;
    data_array[7] = 0x3F3B350E;
    data_array[8] = 0x0A05421F;
    data_array[9] = 0x120E170C;
    data_array[10] = 0x07121214;
    data_array[11] = 0x00171211;
    dsi_set_cmdq(data_array, 12, 1);
    data_array[0] = 0x00043902;
    data_array[1] = 0x00A2A2B6;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x33D21500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00053902;
    data_array[1] = 0x008000C7;
    data_array[2] = 0x000000C0;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0x02CC1500;
    dsi_set_cmdq(data_array, 1, 1);

    //TE
    data_array[0] = 0x00351500;
    dsi_set_cmdq(data_array, 1, 1);

    //PWM
    data_array[0] = 0x00043902;
    data_array[1] = 0x0E001FC9;
    dsi_set_cmdq(data_array, 2, 1);

    //CABC
    data_array[0] = 0x00511500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x24531500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x11551500;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00110500;      //Sleep Out
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(150);
    data_array[0] = 0x00290500;     //Display On
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
}


static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 200, {}},

    // Display ON
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_in_setting[] = {
    // Display off sequence
    {0x28, 20, {0x00}},

    // Sleep Mode On
    {0x10, 100, {0x00}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_init(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);
    init_lcm_registers();
}



static void lcm_suspend(void)
{
    unsigned int data_array[33];
    data_array[0] = 0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(50);
    data_array[0] = 0x00100500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
    //push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN, GPIO_OUT_ZERO);
    MDELAY(10);
}


static void lcm_resume(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,1);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);
    init_lcm_registers();
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    data_array[3]= 0x00053902;
    data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[5]= (y1_LSB);
    data_array[6]= 0x002c3909;

    dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
    //unsigned int default_level = 0;//145;
    unsigned int mapped_level = 0;

    //for LGE backlight IC mapping table
    if(level > 255)
            level = 255;

    if(level >0)
            //mapped_level = default_level+(level)*(255-default_level)/(255);
            mapped_level = (unsigned char)transform[255-level];
    else
            mapped_level=0;

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define LCM_COMPARE_ID_YS 0x20
static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[5];
    unsigned int array[16];

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);

    MDELAY(50);

    array[0] = 0x00013700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xDA, buffer, 1);
    id = buffer[0];
    #ifdef BUILD_LK
        printf("lk debug: YS id = 0x%x\n", id);
    #endif

    if(LCM_COMPARE_ID_YS == id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
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
LCM_DRIVER hx8389c_dsi_ys_cpt_lcm_drv =
{
    .name           = "hx8389c_ys_cpt_dsi_vdo",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
    .set_backlight  = lcm_setbacklight,
    .vddi_power_on  = lcm_vddi_power_on,
    .vddi_power_off  = lcm_vddi_power_off,
    .vddi_power_init = lcm_vddi_power_init,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update
#endif
};
