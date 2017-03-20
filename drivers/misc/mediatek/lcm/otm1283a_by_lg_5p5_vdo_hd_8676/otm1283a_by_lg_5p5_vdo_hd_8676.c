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

    params->dsi.vertical_sync_active                = 2;
    params->dsi.vertical_backporch                  = 14;
    params->dsi.vertical_frontporch                 = 16;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active              = 6;
    params->dsi.horizontal_backporch                = 145;
    params->dsi.horizontal_frontporch               = 145;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->physical_width = 68;
    params->physical_height = 121;

    params->dsi.PLL_CLOCK= 240;//2015.06.23 lijianbin@yulong.com modify for emi test
    params->dsi.ssc_disable = 1;
    params->dsi.cont_clock = TRUE;
    params->dsi.noncont_clock = TRUE;
    params->dsi.noncont_clock_period = 2;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
    params->dsi.lcm_esd_check_table[0].count        = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
	
}

static void init_lcm_registers(void)
{
    unsigned int data_array[16];

    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x018312FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x008312FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000A2902;
    data_array[1] = 0x006400C0;
    data_array[2] = 0x6400110F;
    data_array[3] = 0x0000110F;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072902;
    data_array[1] = 0x005500C0;
    data_array[2] = 0x00040001;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0xA4002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00C02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xB3002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x005000C0;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x81002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77C12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x82C42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x82002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02C42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x49C42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xC6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03B02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052902;
    data_array[1] = 0x021102F5;
    data_array[2] = 0x00000011;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x50C52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x66C52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xB2002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000F5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xB4002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000F5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xB6002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000F5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xB8002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000F5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02F52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xBA002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03F52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xB4002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xC0C52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x061005C4;
    data_array[2] = 0x10150502;
    data_array[3] = 0x02071005;
    data_array[4] = 0x00101505;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000C4;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x91002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x005019C5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x00BCBCD8;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x00B804C5;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xBB002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x80C52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40D02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x000000D1;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2902;
    data_array[1] = 0x000000CB;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000CB;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00FF00FF;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0xFF00FFCB;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000CB;
    data_array[2] = 0x00FF00FF;
    data_array[3] = 0x00FF00FF;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xC0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000CB;
    data_array[2] = 0x00050500;
    data_array[3] = 0x05000500;
    data_array[4] = 0x05050005;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xD0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x050505CB;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000505;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xE0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x050005CB;
    data_array[2] = 0x05050005;
    data_array[3] = 0x00050505;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xF0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2902;
    data_array[1] = 0xFFFFFFCB;
    data_array[2] = 0xFFFFFFFF;
    data_array[3] = 0xFFFFFFFF;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000CC;
    data_array[2] = 0x002D2E00;
    data_array[3] = 0x2A002900;
    data_array[4] = 0x100A000C;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x04020ECC;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00002D2E;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x2A0029CC;
    data_array[2] = 0x0F09000B;
    data_array[3] = 0x0003010D;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x000000CC;
    data_array[2] = 0x002E2D00;
    data_array[3] = 0x2A002900;
    data_array[4] = 0x0B09000F;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xC0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102902;
    data_array[1] = 0x01030DCC;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00002E2D;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xD0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x2A0029CC;
    data_array[2] = 0x0C0A0010;
    data_array[3] = 0x0002040E;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2902;
    data_array[1] = 0x10038BCE;
    data_array[2] = 0x8910038A;
    data_array[3] = 0x03881003;
    data_array[4] = 0x00000010;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x0000F0CE;
    data_array[2] = 0xF00000F0;
    data_array[3] = 0x00F00000;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x840738CE;
    data_array[2] = 0x001000FE;
    data_array[3] = 0xFF840638;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x850538CE;
    data_array[2] = 0x00100000;
    data_array[3] = 0x01850438;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xC0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x850338CE;
    data_array[2] = 0x00100002;
    data_array[3] = 0x03850238;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xD0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x850138CE;
    data_array[2] = 0x00100004;
    data_array[3] = 0x05850038;
    data_array[4] = 0x00001000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x80002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x000070CF;
    data_array[2] = 0x00000010;
    data_array[3] = 0x10000070;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0x90002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x000070CF;
    data_array[2] = 0x00000010;
    data_array[3] = 0x10000070;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x000070CF;
    data_array[2] = 0x00000010;
    data_array[3] = 0x10000070;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xB0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2902;
    data_array[1] = 0x000070CF;
    data_array[2] = 0x00000010;
    data_array[3] = 0x10000070;
    data_array[4] = 0x00000000;
    dsi_set_cmdq(data_array, 5, 1);
    data_array[0] = 0xC0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2902;
    data_array[1] = 0x200104CF;
    data_array[2] = 0x01000020;
    data_array[3] = 0x08030001;
    dsi_set_cmdq(data_array, 4, 1);
    data_array[0] = 0xB5002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072902;
    data_array[1] = 0x30ED3BC5;
    data_array[2] = 0x0030ED3B;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xA4D92300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152902;
    data_array[1] = 0x130E04E1;
    data_array[2] = 0x090D070E;
    data_array[3] = 0x0F090608;
    data_array[4] = 0x0B0F0E07;
    data_array[5] = 0x00000008;
    dsi_set_cmdq(data_array, 6, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00112902;
    data_array[1] = 0x140F04E2;
    data_array[2] = 0x090E070E;
    data_array[3] = 0x0F090608;
    data_array[4] = 0x0B0F0E07;
    data_array[5] = 0x00000008;
    dsi_set_cmdq(data_array, 6, 1);

    data_array[0] = 0xA0002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02C12300; // 0xFFC11500;
    dsi_set_cmdq(data_array, 1, 1);
    //Orise mode disable
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0xFFFFFFFF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x24532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00110500;      //Sleep Out
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;     //Display On
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

}


static struct LCM_setting_table lcm_sleep_in_setting[] = {
    // Display off sequence
    {0x28, 10, {0x00}},

    // Sleep Mode On
    {0x10, 120, {0x00}},

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
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ZERO);
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
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
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
    unsigned int default_level = 0;//145;
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

static unsigned int Calc_LCD_Vol(unsigned int *a)
{
  char i,j;
  unsigned int temp;
  for(i=0;i<10;i++)
    for(j=i;j<10;j++)
    {
      if(a[i]>a[j])
      {
        temp=a[i];
        a[i]=a[j];
        a[j]=temp;
      }
    }
  temp=0;
  for(i=2;i<8;i++)
    temp=temp+a[i];

  return temp/6;
}
extern unsigned char lcd_type;
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define LCM_ID_8675_BY_CPT 0x40
static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[5];
    unsigned int array[16];

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
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

    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xDA, buffer, 1);
    id = buffer[0];
    #ifdef BUILD_LK
        printf("lk BY_CPT_LCD  debug: BY id = 0x%x\n", id);
    #else
        printk("kernel BY_CPT_LCD  horse debug: BY id = 0x%x\n",id);
    #endif


    if(id == LCM_ID_8675_BY_CPT)
        return 1;
    else
        return 0;

}

#if 0
#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif
static unsigned int lcm_esd_test = FALSE;

#ifndef BUILD_LK
extern void Yulong_set_HS_read();
extern void Yulong_restore_HS_read();
#endif


static unsigned int lcm_esd_check(void)
{

    #ifndef BUILD_LK
    char buffer[3],buffer1[3],buffer2[3],buffer3[3],buffer4[3];
    unsigned int data_array[16];
    int array[4];
    if(lcm_esd_test)
    {
    lcm_esd_test=FALSE;
     return TRUE;
    }

    lcd_type=2;
    Yulong_set_HS_read();
    array[0]=0x00013708;
    dsi_set_cmdq(array,1,1);
    read_reg_v2(0x0a,buffer,1);

    array[0]=0x00013708;
    dsi_set_cmdq(array,1,1);
    read_reg_v2(0x0b,buffer1,1);

    array[0]=0x00013708;
    dsi_set_cmdq(array,1,1);
    read_reg_v2(0x0c,buffer2,1);

    array[0]=0x00013708;
    dsi_set_cmdq(array,1,1);
    read_reg_v2(0x0d,buffer3,1);

    //array[0]=0x00013708;
    // dsi_set_cmdq(array,1,1);
    // read_reg_v2(0x0e,buffer4,1);

    Yulong_restore_HS_read();
    //printk("yulong lcm_esd_check reg 0xa=%x 0xb=%x 0xc=%x 0xd=%x 0xe=%x \n",buffer[0],buffer1[0],buffer2[0],buffer3[0],buffer4[0]);
    if((buffer[0]==0x9c)&&(buffer1[0]==0x00)&&(buffer2[0]==0x07)&&(buffer3[0]==0x00))//&&(buffer4[0]==0x80))
    {
       return false;
    }
    else
    {
       return true;
    }
    #endif

 }

 static unsigned int lcm_esd_recover(void)
 {
 #ifndef BUILD_LK
    printk("lcm_esd_recover start\n");
 #endif
    lcm_util.set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_PWR_EN,0);
    MDELAY(50);
    lcm_init();
 #ifndef BUILD_LK
    printk("lcm_esd_recover end\n");
 #endif
    return true;
 }
#endif
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
LCM_DRIVER  otm1283a_dsi_vdo_by_lg_lcm_drv =
{
  .name           = "otm1283a_dsi_by_lg",
  .set_util_funcs = lcm_set_util_funcs,
  .get_params     = lcm_get_params,
  .init           = lcm_init,
  .suspend        = lcm_suspend,
  .resume         = lcm_resume,
  .compare_id     = lcm_compare_id,
  //.esd_check      = lcm_esd_check,
  //.esd_recover    = lcm_esd_recover,
  .set_backlight  = lcm_setbacklight,
    .vddi_power_on  = lcm_vddi_power_on,
    .vddi_power_off  = lcm_vddi_power_off,
    .vddi_power_init = lcm_vddi_power_init,
#if (LCM_DSI_CMD_MODE)
  .update         = lcm_update
#endif
};
