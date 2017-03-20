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

#define FRAME_WIDTH                                         (720)
#define FRAME_HEIGHT                                        (1280)

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
    6, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 0
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
    //1 Three lane or Four lane
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Video mode setting
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active                = 2;
    params->dsi.vertical_backporch                  = 14;
    params->dsi.vertical_frontporch                 = 16;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active              = 6;
    params->dsi.horizontal_backporch                = 42;
    params->dsi.horizontal_frontporch               = 44;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->physical_width = 68;
    params->physical_height = 121;

    params->dsi.PLL_CLOCK=200;
    params->dsi.ssc_disable = 1;

    params->dsi.noncont_clock = 1;
    params->dsi.noncont_clock_period = 2;

    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;
}

static void init_lcm_registers(void)
{
    unsigned int data_array[33];

    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x018712ff;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x008712ff;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x92002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000230ff;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000a2902;
    data_array[1] = 0x006400c0;
    data_array[2] = 0x64001010;
    data_array[3] = 0x00001010;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072902;
    data_array[1] = 0x005c00c0;
    data_array[2] = 0x00040001;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0xb3002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x005500c0;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x81002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x55c12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x041005c4;
    data_array[2] = 0x11150502;
    data_array[3] = 0x02071005;
    data_array[4] = 0x00111505;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xb0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000c4;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x91002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x00d2a6c5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x00c7c7d8;
    dsi_set_cmdq(data_array, 2, 1);

    //VCOM
    //data_array[0] = 0x00002300;
    //dsi_set_cmdq(data_array, 1, 1);
    //data_array[0] = 0x6Cd92300;
    //dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0xb3002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9ac52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xbb002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8ac52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x82002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0ac42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xc6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03B02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40d02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000d1;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xb2002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000f5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xb6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000f5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000f5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xd2002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x001506f5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xb4002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xccc52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052902;
    data_array[1] = 0x021102f5;
    data_array[2] = 0x00000015;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x50c52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x66c52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000c2902;
    data_array[1] = 0x000000cb;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000cb;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xa0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000cb;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xb0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000cb;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xc0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x050505cb;
    data_array[2] = 0x05050505;
    data_array[3] = 0x05000505;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xd0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000cb;
    data_array[2] = 0x00000500;
    data_array[3] = 0x05050505;
    data_array[4] = 0x05050505;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xe0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x050005cb;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000005;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xf0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000c2902;
    data_array[1] = 0xffffffcb;
    data_array[2] = 0xffffffff;
    data_array[3] = 0xffffffff;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x0a2a29cc;
    data_array[2] = 0x12100e0c;
    data_array[3] = 0x08000614;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000cc;
    data_array[2] = 0x00000200;
    data_array[3] = 0x0b092a29;
    data_array[4] = 0x13110f0d;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xa0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x070005cc;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000001;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xb0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x132a29cc;
    data_array[2] = 0x0b0d0f11;
    data_array[3] = 0x07000109;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xc0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000cc;
    data_array[2] = 0x00000500;
    data_array[3] = 0x12142a29;
    data_array[4] = 0x0a0c0e10;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xd0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x080002cc;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000006;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000d2902;
    data_array[1] = 0x100589ce;
    data_array[2] = 0x00100588;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x10fc54ce;
    data_array[2] = 0x5510fd54;
    data_array[3] = 0x01551000;
    data_array[4] = 0x00000010;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xa0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x040758ce;
    data_array[2] = 0x001000fc;
    data_array[3] = 0xfd040658;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xb0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x040558ce;
    data_array[2] = 0x001000fe;
    data_array[3] = 0xff040458;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xc0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x050358ce;
    data_array[2] = 0x00100000;
    data_array[3] = 0x01050258;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xd0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x050158ce;
    data_array[2] = 0x00100002;
    data_array[3] = 0x03050058;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x050050cf;
    data_array[2] = 0x00100004;
    data_array[3] = 0x05050150;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x050250cf;
    data_array[2] = 0x00100006;
    data_array[3] = 0x07050350;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xa0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x000000cf;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xb0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000f2902;
    data_array[1] = 0x000000cf;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xc0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000c2902;
    data_array[1] = 0x203939cf;
    data_array[2] = 0x01000020;
    data_array[3] = 0x00002001;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0xb5002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072902;
    data_array[1] = 0xff950bc5;
    data_array[2] = 0x00ff950b;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152902;
    data_array[1] = 0x42382DE1;
    data_array[2] = 0x67665A4E;
    data_array[3] = 0x72927B8D;
    data_array[4] = 0x4C4F725F;
    data_array[5] = 0x03113140;
    data_array[6] = 0x00000000;
    dsi_set_cmdq(data_array, 7, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152902;
    data_array[1] = 0x42392DE2;
    data_array[2] = 0x67675B4E;
    data_array[3] = 0x72937C8E;
    data_array[4] = 0x4C4F725F;
    data_array[5] = 0x02113040;
    data_array[6] = 0x00000000;
    dsi_set_cmdq(data_array, 7, 1);

    //timeout disable
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02C12300;
    dsi_set_cmdq(data_array, 1, 1);

    //open TE
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00112300;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00292300;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0xffffffff;
    dsi_set_cmdq(data_array, 2, 1);

    //open TE
    //data_array[0] = 0x00352300;
    //dsi_set_cmdq(data_array, 1, 1);

    /*
    data_array[0] = 0x00110500;      //Sleep Out
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;     //Display On
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
    */
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
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
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
    push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,0);
    MDELAY(10);
}


static void lcm_resume(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
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
#define LCM_COMPARE_ID_OF_INNO 0x50
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
    MDELAY(10);
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
        printf("lk debug: OF_INNO id = 0x%x\n", id);
    #endif

    if(LCM_COMPARE_ID_OF_INNO == id)
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
LCM_DRIVER otm1287a_dsi_of_inno_lcm_drv =
{
    .name           = "otm1287a_dsi_of_inno",
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
