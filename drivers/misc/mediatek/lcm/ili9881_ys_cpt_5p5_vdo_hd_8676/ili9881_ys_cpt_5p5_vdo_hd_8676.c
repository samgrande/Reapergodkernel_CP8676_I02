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
    params->dsi.intermediat_buffer_num = 2;

    params->dsi.vertical_sync_active                = 2;// 3    2
    params->dsi.vertical_backporch                  = 20;//13;// 20   1
    params->dsi.vertical_frontporch                 = 20;//10; // 1  12
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active              = 4;// 10  2
    params->dsi.horizontal_backporch                = 160;//65;//100;
    params->dsi.horizontal_frontporch               = 130;//64;//86;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->physical_width = 68;
    params->physical_height = 121;

    params->dsi.PLL_CLOCK=241;//2015.06.02 lijianbin@yulong.com modify for emi test
    params->dsi.ssc_disable = 1;
    params->dsi.noncont_clock = TRUE;
    params->dsi.noncont_clock_period = 2;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;
}

static void init_lcm_registers(void)
{
    unsigned int data_array[16];


    data_array[0] = 0x00043902;
    data_array[1] = 0x078198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x20032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x06042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x010A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2F0B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x44102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2F162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2F172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x501B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xBB1C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0C1D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xC0232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x30242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x23312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x67332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xAB352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x23372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x67392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x893A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xAB3B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xCD3C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xEF3D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x11502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x06512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0C522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0D532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0E542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x08662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x08672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0C682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0D692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0E6A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0F6B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x017A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x067C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00043902;
    data_array[1] = 0x088198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0xB4762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2B742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x158E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x047b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x25722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3a872300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4c7E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x056c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x10492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x802d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x65502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x29532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x65542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x38552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47582300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00043902;
    data_array[1] = 0x018198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x0A222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7c532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x78552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00A02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x18A12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x25A22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x12A32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x14A42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x26A52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x19A62300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1cA72300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87A82300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1dA92300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x29AA2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84AB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1fAC2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1eAD2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x52AE2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x25AF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2aB02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x50B12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x5eB22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3fB32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00C02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x16C12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x23C22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x12C32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x14C42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x25C52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x19C62300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1cC72300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x85C82300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1dC92300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x29CA2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83CB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1fCC2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1eCD2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x52CE2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x25CF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2aD02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x50D12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x5dD22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3fD32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00043902;
    data_array[1] = 0x068198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x04002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00043902;
    data_array[1] = 0x008198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x24532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00552300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00352300;
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
    {0x28, 20, {0x00}},

    // Sleep Mode On
    {0x10, 100, {0x00}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
static void lcm_init(void)
{
    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10); 
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
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

    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ZERO);
    MDELAY(10); 
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
}


static void lcm_resume(void)
{
    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
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

#define LCM_ID_8676_YS_CPT 0x21
static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[5];
    unsigned int array[16];

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
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
        printf("lk YS_CPT_LCD  debug: BY id = 0x%x\n", id);
    #else
        printk("kernel YS_CPT_LCD  horse debug: BY id = 0x%x\n",id);
    #endif


    if(id == LCM_ID_8676_YS_CPT)
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
LCM_DRIVER  ili9881_dsi_vdo_ys_cpt_lcm_drv =
{
  .name           = "ili9881_dsi_ys_cpt",
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
