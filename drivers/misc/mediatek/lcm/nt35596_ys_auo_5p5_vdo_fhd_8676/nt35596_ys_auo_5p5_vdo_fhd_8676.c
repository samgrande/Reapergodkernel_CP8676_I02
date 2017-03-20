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
        params->dsi.vertical_backporch                  = 6;// 20   1 8
        params->dsi.vertical_frontporch                 = 7; // 1  12
        params->dsi.vertical_active_line                = FRAME_HEIGHT;

        params->dsi.horizontal_sync_active              = 10;// 10  2
        params->dsi.horizontal_backporch                = 46; //25;//100;
        params->dsi.horizontal_frontporch               = 96; //24;//86;
        params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

        params->physical_width = 68;
        params->physical_height = 121;

        params->dsi.PLL_CLOCK=  410; //410;
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

    data_array[0] = 0xEEFF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4F242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xC8382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x27392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x771E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F1D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x717E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x317C2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x01FF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x55012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4A062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x24072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0C082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7D0B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7D0C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xB00E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xAE0F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x10122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4A142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x12152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x12162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x551A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x131B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x131E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00282300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00662300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x82582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x825C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x825D2300;
    dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x025E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x31722300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x05FF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x01002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0B012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0C022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x09032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0A042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x10072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x130D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x150E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x170F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x01102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0B112300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x0C122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x09132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0A142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x10172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x131D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x151E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x171F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xED252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x58292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x122A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x012B2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x064B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x114C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x204D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x024E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x024F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x20502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x61512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x63532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x77542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xED552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x155F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x75602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x04682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x406C2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x01762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x807A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xA37B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xD87C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x607D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x157F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x81802300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05832300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x08932300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x10942300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x008A2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x04FF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xFF082300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x01FF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02FF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x04FF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01FB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00FF2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x08D32300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x07D42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00FF2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F9B2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00110500;      //Sleep Out
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;     //Display On
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
    MDELAY(100);

    lcm_util.set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_PWR_EN,0);
    MDELAY(30);  //recommand 10ms (+50ms)  300ms

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(100);

}


static void lcm_resume_power(void)
{
    lcm_init_power();
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
    MDELAY(10);

    //mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

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
    if(id==0x22)
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
LCM_DRIVER nt35596_dsi_vdo_ys_auo_lcm_drv =
{
    .name           = "nt35596_ys_auo",
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
