/*
 *drivers/input/touchscreen/ft5x06_ex_fun.c
 *
 *FocalTech IC driver expand function for debug.
 *
 *Copyright (c) 2010  Focal tech Ltd.
 *
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation, and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *Note:the error code of EIO is the general error in this file.
 */

#include "tpd.h"


#include "focaltech_ex_fun.h"

#include "tpd_custom_fts.h"

#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>

#include "test_lib.h"

#define MT6582
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);

//shihaobin@yulong.com add ESD protect begin 20150729
#if FT_ESD_PROTECT
int apk_debug_flag = 0;
#endif
//shihaobin@yulong.com add ESD protect begin 20150729

/*---------------------------------------------------------------------------
*
*
*TOUCHSCREEN SHORTTEST
*
*--------------------------------------------------------------------------*/
//[BEGIN] liushilong@yulong.com add scaptest define variable and function 2015-5-25
struct i2c_client *G_Client = NULL;
static bool shorttest = false;
/*
#ifdef  FT5446_openshort_test
int Save_rawData0[TX_NUM_MAX][RX_NUM_MAX];
int Save_rawData1[TX_NUM_MAX][RX_NUM_MAX];
int Save_rawData2[TX_NUM_MAX][RX_NUM_MAX];
int Save_rawData3[TX_NUM_MAX][RX_NUM_MAX];

int TX_NUM;
int RX_NUM;
int total_item;
int Normalize;
int SCab_1;
int SCab_2;
int SCab_3;
int SCab_4;
int SCab_5;
int SCab_6;
int SCab_7;
int SCab_8;
#endif  //FT5446_openshort_test
*/
#ifdef FT5446_openshort_test
int FTS_I2c_Read_ShortTest(unsigned char * wBuf, int wLen, unsigned char *rBuf, int rLen)
{
    if(NULL == G_Client)
    {
        return -1;
    }

    return fts_i2c_Read(G_Client, wBuf, wLen, rBuf, rLen);
}

int FTS_I2c_Write_ShortTest(unsigned char * wBuf, int wLen)
{
    //unsigned char buf[2] = {0};
    //buf[0] = regaddr;
    //buf[1] = regvalue;
    if(NULL == G_Client)
    {
        return -1;
    }

    return fts_i2c_Write(G_Client, wBuf, wLen);
}
#endif  //FT5446_openshort_test

#ifdef FT5446_openshort_test
#define FTXXXX_INI_FILEPATH "/system/etc/"
#endif
//[END]

/*---------------------------------------------------------------------------
*
*
*FUNCTION DECLARATION
*
*--------------------------------------------------------------------------*/

int hid_to_i2c(struct i2c_client * client);

//int fts_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf,u32 dw_lenth);
int fts_6x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_6x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_5x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_5x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_5x46_ctpm_fw_upgrade(struct i2c_client *client, u8* pbt_buf, u32 dw_lenth);

static DEFINE_MUTEX(g_device_mutex);

int fts_ctpm_auto_clb(struct i2c_client *client)
{
    unsigned char uc_temp = 0x00;
    unsigned char i = 0;

    /*start auto CLB */
    msleep(200);

    fts_write_reg(client, 0, FTS_FACTORYMODE_VALUE);
    /*make sure already enter factory mode */
    msleep(100);
    /*write command to start calibration */
    fts_write_reg(client, 2, 0x4);
    msleep(300);

    if ((fts_updateinfo_curr.CHIP_ID==0x11) ||
        (fts_updateinfo_curr.CHIP_ID==0x12) ||
        (fts_updateinfo_curr.CHIP_ID==0x13) ||
        (fts_updateinfo_curr.CHIP_ID==0x14)) //5x36,5x36i
    {
        for(i=0;i<100;i++)
        {
            fts_read_reg(client, 0x02, &uc_temp);
            if (0x02 == uc_temp || 0xFF == uc_temp)
            {
                /*if 0x02, then auto clb ok, else 0xff, auto clb failure*/
                break;
            }
            msleep(20);
        }
    } else {
        for(i=0;i<100;i++)
        {
            fts_read_reg(client, 0, &uc_temp);
            if (0x0 == ((uc_temp&0x70)>>4))  /*return to normal mode, calibration finish*/
            {
                break;
            }
            msleep(20);
        }
    }
    /*calibration OK */
    msleep(300);
    fts_write_reg(client, 0, FTS_FACTORYMODE_VALUE);  /*goto factory mode for store */
    msleep(100);  /*make sure already enter factory mode */
    fts_write_reg(client, 2, 0x5);  /*store CLB result */
    msleep(300);
    fts_write_reg(client, 0, FTS_WORKMODE_VALUE); /*return to normal mode */
    msleep(300);

  /*store CLB result OK */
  return 0;
}


extern int fts_ctpm_fw_upgrade_with_i_file(int tp);
unsigned char bootloader_ID;//add by liyongfeng
extern unsigned char read_tp_bootloader_version(void);
extern u8 focaltech_type;
extern bool real_isGolden;

u8 fts_ctpm_get_i_file_ver(void)
{
    u16 ui_sz;

    bootloader_ID=read_tp_bootloader_version();

    if((bootloader_ID==0x51)||(bootloader_ID==TW_OFILM_ID))
    {
        if(FALSE == real_isGolden)
        {
            ui_sz = sizeof(OFILM_CTPM_FW000);
            if (ui_sz > 2)
            {
                return OFILM_CTPM_FW000[ui_sz - 2];
            }
        } else {
            ui_sz = sizeof(OFILM_CTPM_FW001);
            if (ui_sz > 2)
            {
                return OFILM_CTPM_FW001[ui_sz - 2];
            }
        }
    }
    else if((bootloader_ID==0x80)||(bootloader_ID==TW_YEJI_ID))
    {
        if(FALSE == real_isGolden)
        {
            ui_sz = sizeof(YEJI_CTPM_FW000);
            if (ui_sz > 2)
            {
                return YEJI_CTPM_FW000[ui_sz - 2];
            }
        } else {
            ui_sz = sizeof(YEJI_CTPM_FW001);
            if (ui_sz > 2)
            {
                return YEJI_CTPM_FW001[ui_sz - 2];
            }
        }
    }
    else if((bootloader_ID==0x3B)||(bootloader_ID==TW_BOEN_ID))
    {
        if(FALSE == real_isGolden)
        {
            ui_sz = sizeof(BOEN_CTPM_FW000);
            if (ui_sz > 2)
            {
                return BOEN_CTPM_FW000[ui_sz - 2];
            }
        } else {
            ui_sz = sizeof(BOEN_CTPM_FW001);
            if (ui_sz > 2)
            {
                return BOEN_CTPM_FW001[ui_sz - 2];
            }
        }
    }
    else
    {
        printk("ERROR:Focaltech cann't get version from i file\n");
    }
    return 0x00;  /*default value */
}

/**********************************************************************
*
*update project setting
*only update these settings for COB project, or for some special case
*
**********************************************************************/
int fts_ctpm_update_project_setting(struct i2c_client *client)
{
    u8 uc_i2c_addr; /*I2C slave address (7 bit address)*/
    u8 uc_io_voltage; /*IO Voltage 0---3.3v;  1----1.8v*/
    u8 uc_panel_factory_id; /*TP panel factory ID*/
    u8 buf[FTS_SETTING_BUF_LEN];
    u8 reg_val[2] = {0};
    u8 auc_i2c_write_buf[10] = {0};
    u8 packet_buf[FTS_SETTING_BUF_LEN + 6];
    u32 i = 0;
    int i_ret;

    uc_i2c_addr = client->addr;
    uc_io_voltage = 0x0;
    uc_panel_factory_id = 0x5a;


    /*Step 1:Reset  CTPM
    *write 0xaa to register 0xfc
    */
    fts_write_reg(client, 0xfc, 0xaa);
    msleep(50);

    /*write 0x55 to register 0xfc */
    fts_write_reg(client, 0xfc, 0x55);
    msleep(30);

    /*********Step 2:Enter upgrade mode *****/
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do {
        i++;
        i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
        msleep(5);
    } while (i_ret <= 0 && i < 5);


    /*********Step 3:check READ-ID***********************/
    auc_i2c_write_buf[0] = 0x90;
    auc_i2c_write_buf[1] = 0x00;
    auc_i2c_write_buf[2] = 0x00;
    auc_i2c_write_buf[3] = 0x00;

    fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

    //if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)
    if (reg_val[0] == 0x79 && reg_val[1] == 0x11)
    dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
       reg_val[0], reg_val[1]);
    else
        return -EIO;

    auc_i2c_write_buf[0] = 0xcd;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
    dev_dbg(&client->dev, "bootloader version = 0x%x\n", reg_val[0]);

    /*--------- read current project setting  ---------- */
    /*set read start address */
    buf[0] = 0x3;
    buf[1] = 0x0;
    buf[2] = 0x78;
    buf[3] = 0x0;

    fts_i2c_Read(client, buf, 4, buf, FTS_SETTING_BUF_LEN);
    dev_dbg(&client->dev, "[FTS] old setting: uc_i2c_addr = 0x%x,\
      uc_io_voltage = %d, uc_panel_factory_id = 0x%x\n",
      buf[0], buf[2], buf[4]);

    /*--------- Step 4:erase project setting --------------*/
    auc_i2c_write_buf[0] = 0x63;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(100);

    /*----------  Set new settings ---------------*/
    buf[0] = uc_i2c_addr;
    buf[1] = ~uc_i2c_addr;
    buf[2] = uc_io_voltage;
    buf[3] = ~uc_io_voltage;
    buf[4] = uc_panel_factory_id;
    buf[5] = ~uc_panel_factory_id;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    packet_buf[2] = 0x78;
    packet_buf[3] = 0x0;
    packet_buf[4] = 0;
    packet_buf[5] = FTS_SETTING_BUF_LEN;

    for (i = 0; i < FTS_SETTING_BUF_LEN; i++)
        packet_buf[6 + i] = buf[i];

    fts_i2c_Write(client, packet_buf, FTS_SETTING_BUF_LEN + 6);
    msleep(100);

    /********* reset the new FW***********************/
    auc_i2c_write_buf[0] = 0x07;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);

    msleep(200);
    return 0;
}

int fts_ctpm_auto_upgrade(struct i2c_client *client)
{
    u8 uc_host_fm_ver = FTS_REG_FW_VER;
    u8 uc_tp_fm_ver;
    int i_ret;

    fts_read_reg(client, FTS_REG_FW_VER, &uc_tp_fm_ver);
    uc_host_fm_ver = fts_ctpm_get_i_file_ver();

    printk("[FTS] uc_tp_fm_ver = 0x%x, uc_host_fm_ver = 0x%x\n",uc_tp_fm_ver, uc_host_fm_ver);

    if (/*the firmware in touch panel maybe corrupted */
        uc_tp_fm_ver == FTS_REG_FW_VER ||
        /*the firmware in host flash is new, need upgrade */
        uc_tp_fm_ver < uc_host_fm_ver)
    {
        msleep(100);
        dev_dbg(&client->dev, "[FTS] uc_tp_fm_ver = 0x%x, uc_host_fm_ver = 0x%x\n",
                    uc_tp_fm_ver, uc_host_fm_ver);
        i_ret = fts_ctpm_fw_upgrade_with_i_file(bootloader_ID);
        if (i_ret == 0) {
        msleep(300);
        uc_host_fm_ver = fts_ctpm_get_i_file_ver();
        dev_dbg(&client->dev, "[FTS] upgrade to new version 0x%x\n",
            uc_host_fm_ver);
        }
        else
        {
          pr_err("[FTS] upgrade failed ret=%d.\n", i_ret);
          return -EIO;
        }
    }
  return 0;
}

#if 0
/*
*get upgrade information depend on the ic type
*/
static void fts_get_upgrade_info(struct Upgrade_Info *upgrade_info)
{
  switch (DEVICE_IC_TYPE) {
  case IC_FT5X06:
    upgrade_info->delay_55 = FT5X06_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT5X06_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT5X06_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT5X06_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT5X06_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT5X06_UPGRADE_EARSE_DELAY;
    break;
  case IC_FT5606:
    upgrade_info->delay_55 = FT5606_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT5606_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT5606_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT5606_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT5606_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT5606_UPGRADE_EARSE_DELAY;
    break;
  case IC_FT5316:
    upgrade_info->delay_55 = FT5316_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT5316_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT5316_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT5316_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT5316_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT5316_UPGRADE_EARSE_DELAY;
    break;
  case IC_FT6208:
    upgrade_info->delay_55 = FT6208_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT6208_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT6208_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT6208_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT6208_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT6208_UPGRADE_EARSE_DELAY;
    break;
  case IC_FT6x06:
    upgrade_info->delay_55 = FT6X06_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT6X06_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT6X06_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT6X06_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT6X06_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT6X06_UPGRADE_EARSE_DELAY;
    break;
  case IC_FT5x06i:
    upgrade_info->delay_55 = FT5X06_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT5X06_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT5X06_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT5X06_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT5X06_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT5X06_UPGRADE_EARSE_DELAY;
    break;
  case IC_FT5x36:
    upgrade_info->delay_55 = FT5X36_UPGRADE_55_DELAY;
    upgrade_info->delay_aa = FT5X36_UPGRADE_AA_DELAY;
    upgrade_info->upgrade_id_1 = FT5X36_UPGRADE_ID_1;
    upgrade_info->upgrade_id_2 = FT5X36_UPGRADE_ID_2;
    upgrade_info->delay_readid = FT5X36_UPGRADE_READID_DELAY;
    upgrade_info->delay_earse_flash = FT5X36_UPGRADE_EARSE_DELAY;
    break;
  default:
    break;
  }
}

#endif

extern void delay_qt_ms(unsigned long  w_ms);

int hid_to_i2c(struct i2c_client * client)
{
    u8 auc_i2c_write_buf[5] = {0};
    int bRet = 0;

    auc_i2c_write_buf[0] = 0xeb;
    auc_i2c_write_buf[1] = 0xaa;
    auc_i2c_write_buf[2] = 0x09;

    fts_i2c_Write(client, auc_i2c_write_buf, 3);

    msleep(10);

    auc_i2c_write_buf[0] = auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = 0;

    fts_i2c_Read(client, auc_i2c_write_buf, 0, auc_i2c_write_buf, 3);

    if(0xeb==auc_i2c_write_buf[0] && 0xaa==auc_i2c_write_buf[1] && 0x08==auc_i2c_write_buf[2])
    {
        bRet = 1;
    }
    else bRet = 0;

    return bRet;

}

#if 0
int fts_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf,
        u32 dw_lenth)
{
  u8 reg_val[2] = {0};
  u32 i = 0;
  u32 packet_number;
  u32 j;
  u32 temp;
  u32 lenght;
  u8 packet_buf[FTS_PACKET_LENGTH + 6];
  u8 auc_i2c_write_buf[10];
  u8 bt_ecc;
  int i_ret;
  u8 is_5336_fwsize_30 = 0;
  struct Upgrade_Info upgradeinfo;

  fts_get_upgrade_info(&upgradeinfo);

  if(pbt_buf[dw_lenth-12] == 30)
  {
    is_5336_fwsize_30 = 1;
  }
  else
  {
    is_5336_fwsize_30 = 0;
  }
  printk("is_5336_fwsize_30 is %d\n" ,is_5336_fwsize_30);
  for (i = 0; i < FTS_UPGRADE_LOOP; i++)
  {
          msleep(100);
    printk("[FTS] Step 1:Reset  CTPM\n");
    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc */
    if (DEVICE_IC_TYPE == IC_FT6208 || DEVICE_IC_TYPE == IC_FT6x06)
      fts_write_reg(client, 0xbc, FTS_UPGRADE_AA);
    else
      fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
    msleep(upgradeinfo.delay_aa);

    /*write 0x55 to register 0xfc */
    if(DEVICE_IC_TYPE == IC_FT6208 || DEVICE_IC_TYPE == IC_FT6x06)
      fts_write_reg(client, 0xbc, FTS_UPGRADE_55);
    else
      fts_write_reg(client, 0xfc, FTS_UPGRADE_55);

    msleep(upgradeinfo.delay_55);


    /*********Step 2:Enter upgrade mode *****/
    printk("[FTS] Step 2:Enter upgrade mode \n");
    #if 1
      auc_i2c_write_buf[0] = FTS_UPGRADE_55;
      auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
      do {
        i++;
        i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
        msleep(5);
      } while (i_ret <= 0 && i < 5);
    #else
      auc_i2c_write_buf[0] = FTS_UPGRADE_55;
      fts_i2c_Write(client, auc_i2c_write_buf, 1);
      msleep(5);
      auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
      fts_i2c_Write(client, auc_i2c_write_buf, 1);
    #endif
#if 1
    /*********Step 3:check READ-ID***********************/
    msleep(upgradeinfo.delay_readid);
    auc_i2c_write_buf[0] = 0x90;
    auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
    fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

    if (reg_val[0] == upgradeinfo.upgrade_id_1
      && reg_val[1] == upgradeinfo.upgrade_id_2) {
      printk("[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
        reg_val[0], reg_val[1]);
      break;
    } else {
      dev_err(&client->dev, "[FTS] Step 3 error: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
        reg_val[0], reg_val[1]);
    }
#endif
  }
  if (i > FTS_UPGRADE_LOOP)
    return -EIO;

  auc_i2c_write_buf[0] = 0xcd;
  fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
  printk("bootloader version : 0x%x\n", reg_val[0]);

  /*Step 4:erase app and panel paramenter area*/
  DBG("Step 4:erase app and panel paramenter area\n");
  if(is_5336_fwsize_30)
  {
    auc_i2c_write_buf[0] = 0x61;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);  /*erase app area */
    msleep(upgradeinfo.delay_earse_flash);
    /*erase panel parameter area */
    auc_i2c_write_buf[0] = 0x63;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(100);
  }
  else
  {
    auc_i2c_write_buf[0] = 0x61;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);  /*erase app area */
    msleep(upgradeinfo.delay_earse_flash);
  }

  /*********Step 5:write firmware(FW) to ctpm flash*********/
  bt_ecc = 0;
  DBG("Step 5:write firmware(FW) to ctpm flash\n");

  dw_lenth = dw_lenth - 14;
  packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
  packet_buf[0] = 0xbf;
  packet_buf[1] = 0x00;

  for (j = 0; j < packet_number; j++) {
    temp = j * FTS_PACKET_LENGTH;
    packet_buf[2] = (u8) (temp >> 8);
    packet_buf[3] = (u8) temp;
    lenght = FTS_PACKET_LENGTH;
    packet_buf[4] = (u8) (lenght >> 8);
    packet_buf[5] = (u8) lenght;

    for (i = 0; i < FTS_PACKET_LENGTH; i++) {
      packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
      bt_ecc ^= packet_buf[6 + i];
    }

    fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
    msleep(FTS_PACKET_LENGTH / 6 + 1);
    if((((j+1) * FTS_PACKET_LENGTH)%1024)==0)
    DBG("write bytes:0x%04x\n", (j+1) * FTS_PACKET_LENGTH);
    //delay_qt_ms(FTS_PACKET_LENGTH / 6 + 1);
  }

  if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
    temp = packet_number * FTS_PACKET_LENGTH;
    packet_buf[2] = (u8) (temp >> 8);
    packet_buf[3] = (u8) temp;
    temp = (dw_lenth) % FTS_PACKET_LENGTH;
    packet_buf[4] = (u8) (temp >> 8);
    packet_buf[5] = (u8) temp;

    for (i = 0; i < temp; i++) {
      packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
      bt_ecc ^= packet_buf[6 + i];
    }

    fts_i2c_Write(client, packet_buf, temp + 6);
    msleep(20);
  }
  /*send the last twelve byte */
  for (i = 0; i<12; i++)
  {
    if (is_5336_fwsize_30 && DEVICE_IC_TYPE==IC_FT5x36)
    {
      temp = 0x7ff4 + i;
    }
    else if (DEVICE_IC_TYPE==IC_FT5x36)
    {
      temp = 0x7bf4 + i;
    }
    packet_buf[2] = (u8)(temp>>8);
    packet_buf[3] = (u8)temp;
    temp =1;
    packet_buf[4] = (u8)(temp>>8);
    packet_buf[5] = (u8)temp;
    packet_buf[6] = pbt_buf[ dw_lenth + i ];
    bt_ecc ^= packet_buf[6];

      fts_i2c_Write(client, packet_buf, 7);
    msleep(20);

  }

  /*********Step 6: read out checksum***********************/
  /*send the opration head */
  DBG("Step 6: read out checksum\n");
  auc_i2c_write_buf[0] = 0xcc;
  fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
  if (reg_val[0] != bt_ecc) {
    dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
          reg_val[0],
          bt_ecc);
    return -EIO;
  }

  /*********Step 7: reset the new FW***********************/
  DBG("Step 7: reset the new FW\n");
  auc_i2c_write_buf[0] = 0x07;
  fts_i2c_Write(client, auc_i2c_write_buf, 1);
  msleep(300);  /*make sure CTP startup normally */

  return 0;
}
#endif

int fts_6x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf,
        u32 dw_lenth)
{
    u8 reg_val[2] = {0};
    u32 i = 0;
    u32 packet_number;
    u32 j;
    u32 temp;
    u32 lenght;
    u32 fw_length;
    u8 packet_buf[FTS_PACKET_LENGTH + 6];
    u8 auc_i2c_write_buf[10];
    u8 bt_ecc;
    int i_ret;

    if(pbt_buf[0] != 0x02)
    {
        DBG("[FTS] FW first byte is not 0x02. so it is invalid \n");
        return -1;
    }

    if(dw_lenth > 0x11f)
    {
        fw_length = ((u32)pbt_buf[0x100]<<8) + pbt_buf[0x101];
        if(dw_lenth < fw_length)
        {
            DBG("[FTS] Fw length is invalid \n");
            return -1;
        }
    }
    else
    {
        DBG("[FTS] Fw length is invalid \n");
        return -1;
    }

    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register 0xbc */

        fts_write_reg(client, 0xbc, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        /*write 0x55 to register 0xbc */
        fts_write_reg(client, 0xbc, FTS_UPGRADE_55);

        msleep(fts_updateinfo_curr.delay_55);

        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        fts_i2c_Write(client, auc_i2c_write_buf, 1);

        auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
        fts_i2c_Write(client, auc_i2c_write_buf, 1);
        msleep(fts_updateinfo_curr.delay_readid);

        /*********Step 3:check READ-ID***********************/
        auc_i2c_write_buf[0] = 0x90;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
          0x00;
        reg_val[0] = 0x00;
        reg_val[1] = 0x00;
        fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


        if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
            && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
                DBG("[FTS] Step 3: GET CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
                    reg_val[0], reg_val[1]);
                break;
        } else {
                dev_err(&client->dev, "[FTS] Step 3: GET CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
                    reg_val[0], reg_val[1]);
        }
    }

    if (i >= FTS_UPGRADE_LOOP)
        return -EIO;

    auc_i2c_write_buf[0] = 0x90;
    auc_i2c_write_buf[1] = 0x00;
    auc_i2c_write_buf[2] = 0x00;
    auc_i2c_write_buf[3] = 0x00;
    auc_i2c_write_buf[4] = 0x00;
    fts_i2c_Write(client, auc_i2c_write_buf, 5);

    //auc_i2c_write_buf[0] = 0xcd;
    //fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);

    /*Step 4:erase app and panel paramenter area*/
    DBG("Step 4:erase app and panel paramenter area\n");
    auc_i2c_write_buf[0] = 0x61;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);  /*erase app area */
    msleep(fts_updateinfo_curr.delay_earse_flash);

    for(i = 0;i < 200;i++)
    {
        auc_i2c_write_buf[0] = 0x6a;
        auc_i2c_write_buf[1] = 0x00;
        auc_i2c_write_buf[2] = 0x00;
        auc_i2c_write_buf[3] = 0x00;
        reg_val[0] = 0x00;
        reg_val[1] = 0x00;
        fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);
        if(0xb0 == reg_val[0] && 0x02 == reg_val[1])
        {
            DBG("[FTS] erase app finished \n");
            break;
        }
        msleep(50);
    }

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    DBG("Step 5:write firmware(FW) to ctpm flash\n");

    dw_lenth = fw_length;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;

    for (j = 0; j < packet_number; j++) {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (lenght >> 8);
        packet_buf[5] = (u8) lenght;

        for (i = 0; i < FTS_PACKET_LENGTH; i++) {
            packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6 + i];
        }

        fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);

        for(i = 0;i < 30;i++)
        {
            auc_i2c_write_buf[0] = 0x6a;
            auc_i2c_write_buf[1] = 0x00;
            auc_i2c_write_buf[2] = 0x00;
            auc_i2c_write_buf[3] = 0x00;
            reg_val[0] = 0x00;
            reg_val[1] = 0x00;
            fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);
            if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
            {
                DBG("[FTS] write a block data finished \n");
                break;
            }
            msleep(1);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (temp >> 8);
        packet_buf[5] = (u8) temp;

        for (i = 0; i < temp; i++) {
            packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6 + i];
        }

        fts_i2c_Write(client, packet_buf, temp + 6);

        for(i = 0;i < 30;i++)
        {
            auc_i2c_write_buf[0] = 0x6a;
            auc_i2c_write_buf[1] = 0x00;
            auc_i2c_write_buf[2] = 0x00;
            auc_i2c_write_buf[3] = 0x00;
            reg_val[0] = 0x00;
            reg_val[1] = 0x00;
            fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);
            if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
            {
                DBG("[FTS] write a block data finished \n");
                break;
            }
            msleep(1);
        }
    }


    /*********Step 6: read out checksum***********************/
    /*send the opration head */
    DBG("Step 6: read out checksum\n");
    auc_i2c_write_buf[0] = 0xcc;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
    if (reg_val[0] != bt_ecc) {
        dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
          reg_val[0], bt_ecc);
        return -EIO;
    }

    /*********Step 7: reset the new FW***********************/
    DBG("Step 7: reset the new FW\n");
    auc_i2c_write_buf[0] = 0x07;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(300);  /*make sure CTP startup normally */

    return 0;
}

int fts_6x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
    u8 reg_val[2] = {0};
    u32 i = 0;
    u32 packet_number;
    u32 j;
    u32 temp;
    u32 lenght;
    u8 packet_buf[FTS_PACKET_LENGTH + 6];
    u8 auc_i2c_write_buf[10];
    u8 bt_ecc;
    int i_ret;


    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register 0xbc */

        fts_write_reg(client, 0xbc, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        /*write 0x55 to register 0xbc */
        fts_write_reg(client, 0xbc, FTS_UPGRADE_55);

        msleep(fts_updateinfo_curr.delay_55);

        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        auc_i2c_write_buf[1] = FTS_UPGRADE_AA;

        do {
            i++;
            i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
            msleep(5);
        } while (i_ret <= 0 && i < 5);


        /*********Step 3:check READ-ID***********************/
        msleep(fts_updateinfo_curr.delay_readid);
        auc_i2c_write_buf[0] = 0x90;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
          0x00;
        fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


        if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
          && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
            DBG("[FTS] Step 3: CTPM ID OK ,ID1 = 0x%x,ID2 = 0x%x\n",
                reg_val[0], reg_val[1]);
            break;
        } else {
            dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
                reg_val[0], reg_val[1]);
        }
    }
    if (i > FTS_UPGRADE_LOOP)
        return -EIO;
    auc_i2c_write_buf[0] = 0xcd;

    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);


    /*Step 4:erase app and panel paramenter area*/
    DBG("Step 4:erase app and panel paramenter area\n");
    auc_i2c_write_buf[0] = 0x61;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);  /*erase app area */
    msleep(fts_updateinfo_curr.delay_earse_flash);
    /*erase panel parameter area */
    auc_i2c_write_buf[0] = 0x63;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(100);

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    DBG("Step 5:write firmware(FW) to ctpm flash\n");

    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;

    for (j = 0; j < packet_number; j++) {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (lenght >> 8);
        packet_buf[5] = (u8) lenght;

        for (i = 0; i < FTS_PACKET_LENGTH; i++) {
            packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6 + i];
        }

        fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
        msleep(FTS_PACKET_LENGTH / 6 + 1);
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (temp >> 8);
        packet_buf[5] = (u8) temp;

        for (i = 0; i < temp; i++) {
            packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6 + i];
        }

        fts_i2c_Write(client, packet_buf, temp + 6);
        msleep(20);
    }

    /*send the last six byte */
    for (i = 0; i < 6; i++) {
        temp = 0x6ffa + i;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        temp = 1;
        packet_buf[4] = (u8) (temp >> 8);
        packet_buf[5] = (u8) temp;
        packet_buf[6] = pbt_buf[dw_lenth + i];
        bt_ecc ^= packet_buf[6];
        fts_i2c_Write(client, packet_buf, 7);
        msleep(20);
    }


    /*********Step 6: read out checksum***********************/
    /*send the opration head */
    DBG("Step 6: read out checksum\n");
    auc_i2c_write_buf[0] = 0xcc;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
    if (reg_val[0] != bt_ecc) {
        dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
            reg_val[0], bt_ecc);
        return -EIO;
    }

    /*********Step 7: reset the new FW***********************/
    DBG("Step 7: reset the new FW\n");
    auc_i2c_write_buf[0] = 0x07;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(300);  /*make sure CTP startup normally */

    return 0;
}

int fts_5x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
    u8 reg_val[2] = {0};
    u32 i = 0;
    u8 is_5336_new_bootloader = 0;
    u8 is_5336_fwsize_30 = 0;
    u32  packet_number;
    u32  j;
    u32  temp;
    u32  lenght;
    u8  packet_buf[FTS_PACKET_LENGTH + 6];
    u8    auc_i2c_write_buf[10];
    u8    bt_ecc;
    int i_ret;

    if(pbt_buf[dw_lenth-12] == 30)
    {
        is_5336_fwsize_30 = 1;
    }
    else
    {
        is_5336_fwsize_30 = 0;
    }

    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register 0xfc*/
        /*write 0xaa to register 0xfc*/
        fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        /*write 0x55 to register 0xfc*/
        fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
        msleep(fts_updateinfo_curr.delay_55);


        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        auc_i2c_write_buf[1] = FTS_UPGRADE_AA;

        i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);

        /*********Step 3:check READ-ID***********************/
        msleep(fts_updateinfo_curr.delay_readid);
        auc_i2c_write_buf[0] = 0x90;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;

        fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

        if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
            && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)
        {
            dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
            break;
        }
        else
        {
            dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAILD,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
            continue;
        }
    }

    if (i >= FTS_UPGRADE_LOOP)
        return -EIO;

    auc_i2c_write_buf[0] = 0xcd;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);

    /*********20130705 mshl ********************/
    if (reg_val[0] <= 4)
    {
        is_5336_new_bootloader = BL_VERSION_LZ4 ;
    }
    else if(reg_val[0] == 7)
    {
        is_5336_new_bootloader = BL_VERSION_Z7 ;
    }
    else if(reg_val[0] >= 0x0f)
    {
        is_5336_new_bootloader = BL_VERSION_GZF ;
    }

    /*********Step 4:erase app and panel paramenter area ********************/
    if(is_5336_fwsize_30)
    {
        auc_i2c_write_buf[0] = 0x61;
        fts_i2c_Write(client, auc_i2c_write_buf, 1); /*erase app area*/
        msleep(fts_updateinfo_curr.delay_earse_flash);

        auc_i2c_write_buf[0] = 0x63;
        fts_i2c_Write(client, auc_i2c_write_buf, 1); /*erase config area*/
        msleep(50);
    }
    else
    {
        auc_i2c_write_buf[0] = 0x61;
        fts_i2c_Write(client, auc_i2c_write_buf, 1); /*erase app area*/
        msleep(fts_updateinfo_curr.delay_earse_flash);
    }

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;

    if(is_5336_new_bootloader == BL_VERSION_LZ4 || is_5336_new_bootloader == BL_VERSION_Z7 )
    {
        dw_lenth = dw_lenth - 8;
    }
    else if(is_5336_new_bootloader == BL_VERSION_GZF)
    {
        dw_lenth = dw_lenth - 14;
    }
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8)(temp>>8);
        packet_buf[3] = (u8)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (u8)(lenght>>8);
        packet_buf[5] = (u8)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6+i];
        }

        fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH+6);
        msleep(FTS_PACKET_LENGTH/6 + 1);
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8)(temp>>8);
        packet_buf[3] = (u8)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (u8)(temp>>8);
        packet_buf[5] = (u8)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6+i];
        }

        fts_i2c_Write(client, packet_buf, temp+6);
        msleep(20);
    }



    /*send the last six byte*/
    if(is_5336_new_bootloader == BL_VERSION_LZ4 || is_5336_new_bootloader == BL_VERSION_Z7 )
    {
        for (i = 0; i<6; i++)
        {
            if (is_5336_new_bootloader  == BL_VERSION_Z7 /*&& DEVICE_IC_TYPE==IC_FT5x36*/)
            {
                temp = 0x7bfa + i;
            }
            else if(is_5336_new_bootloader == BL_VERSION_LZ4)
            {
                temp = 0x6ffa + i;
            }

            packet_buf[2] = (u8)(temp>>8);
            packet_buf[3] = (u8)temp;
            temp =1;
            packet_buf[4] = (u8)(temp>>8);
            packet_buf[5] = (u8)temp;
            packet_buf[6] = pbt_buf[ dw_lenth + i];
            bt_ecc ^= packet_buf[6];

            fts_i2c_Write(client, packet_buf, 7);
            msleep(10);
        }
    }
    else if(is_5336_new_bootloader == BL_VERSION_GZF)
    {
        for (i = 0; i<12; i++)
        {
            if (is_5336_fwsize_30 /*&& DEVICE_IC_TYPE==IC_FT5x36*/)
            {
                temp = 0x7ff4 + i;
            }
            else if (1/*DEVICE_IC_TYPE==IC_FT5x36*/)
            {
                temp = 0x7bf4 + i;
            }

            packet_buf[2] = (u8)(temp>>8);
            packet_buf[3] = (u8)temp;
            temp =1;
            packet_buf[4] = (u8)(temp>>8);
            packet_buf[5] = (u8)temp;
            packet_buf[6] = pbt_buf[ dw_lenth + i];
            bt_ecc ^= packet_buf[6];

            fts_i2c_Write(client, packet_buf, 7);
            msleep(10);
        }
    }

    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    auc_i2c_write_buf[0] = 0xcc;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);

    if(reg_val[0] != bt_ecc)
    {
        dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
            reg_val[0], bt_ecc);
        return -EIO;
    }

    /*********Step 7: reset the new FW***********************/
    auc_i2c_write_buf[0] = 0x07;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(300);  /*make sure CTP startup normally*/

    return 0;
}

int fts_5x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
    u8 reg_val[2] = {0};
    u32 i = 0;
    u32 packet_number;
    u32 j;
    u32 temp;
    u32 lenght;
    u8 packet_buf[FTS_PACKET_LENGTH + 6];
    u8 auc_i2c_write_buf[10];
    u8 bt_ecc;
    int i_ret;


    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register 0xfc */
        fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        /*write 0x55 to register 0xfc */
        fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
        msleep(fts_updateinfo_curr.delay_55);
        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
        do {
            i++;
            i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
            msleep(5);
        } while (i_ret <= 0 && i < 5);


        /*********Step 3:check READ-ID***********************/
        msleep(fts_updateinfo_curr.delay_readid);
        auc_i2c_write_buf[0] = 0x90;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
          0x00;
        fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


        if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
        && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
            DBG("[FTS] Step 3: CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
                reg_val[0], reg_val[1]);
            break;
        } else {
            dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
            reg_val[0], reg_val[1]);
        }
    }
    if (i >= FTS_UPGRADE_LOOP)
        return -EIO;

    /*Step 4:erase app and panel paramenter area*/
    DBG("Step 4:erase app and panel paramenter area\n");
    auc_i2c_write_buf[0] = 0x61;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);  /*erase app area */
    msleep(fts_updateinfo_curr.delay_earse_flash);
    /*erase panel parameter area */
    auc_i2c_write_buf[0] = 0x63;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(100);

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    DBG("Step 5:write firmware(FW) to ctpm flash\n");

    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;

    for (j = 0; j < packet_number; j++) {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (lenght >> 8);
        packet_buf[5] = (u8) lenght;

        for (i = 0; i < FTS_PACKET_LENGTH; i++) {
            packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6 + i];
        }

        fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
        msleep(FTS_PACKET_LENGTH / 6 + 1);
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (temp >> 8);
        packet_buf[5] = (u8) temp;

        for (i = 0; i < temp; i++) {
            packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
            bt_ecc ^= packet_buf[6 + i];
        }

        fts_i2c_Write(client, packet_buf, temp + 6);
        msleep(20);
    }


    /*send the last six byte */
    for (i = 0; i < 6; i++) {
        temp = 0x6ffa + i;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        temp = 1;
        packet_buf[4] = (u8) (temp >> 8);
        packet_buf[5] = (u8) temp;
        packet_buf[6] = pbt_buf[dw_lenth + i];
        bt_ecc ^= packet_buf[6];
        fts_i2c_Write(client, packet_buf, 7);
        msleep(20);
    }


    /*********Step 6: read out checksum***********************/
    /*send the opration head */
    DBG("Step 6: read out checksum\n");
    auc_i2c_write_buf[0] = 0xcc;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
    if (reg_val[0] != bt_ecc) {
        dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
            reg_val[0],bt_ecc);
        return -EIO;
    }

    /*********Step 7: reset the new FW***********************/
    DBG("Step 7: reset the new FW\n");
    auc_i2c_write_buf[0] = 0x07;
    fts_i2c_Write(client, auc_i2c_write_buf, 1);
    msleep(300);  /*make sure CTP startup normally */
    return 0;
}

int fts_5x46_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
    u8 reg_val[4] = {0};
    u32 i = 0;
    u32 packet_number;
    u32 j;
    u32 temp;
    u32 lenght;
    u8 packet_buf[FTS_PACKET_LENGTH + 6];
    u8 auc_i2c_write_buf[10];
    u8 bt_ecc;
    int i_ret;

    i_ret = hid_to_i2c(client);

    if(i_ret == 0)
    {
        DBG("[FTS] hid change to i2c fail ! \n");
    }

    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*********Step 1:Reset  CTPM *****/
        DBG("Step 1:Reset  CTPM\n");
        /*write 0xaa to register 0xfc */
        fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        //write 0x55 to register 0xfc
        fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
        msleep(200);
        /*********Step 2:Enter upgrade mode *****/
        DBG("Step 2:Enter upgrade mode\n");
        i_ret = hid_to_i2c(client);

        if(i_ret == 0)
        {
            DBG("[FTS] hid change to i2c fail ! \n");
            continue;
        }

        msleep(10);

        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
        i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
        if(i_ret < 0)
        {
            DBG("[FTS] failed writing  0x55 and 0xaa ! \n");
            continue;
        }

        /*********Step 3:check READ-ID***********************/
        msleep(1);
        auc_i2c_write_buf[0] = 0x90;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;

        reg_val[0] = reg_val[1] = 0x00;

        fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

        if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
          && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
          DBG("Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
            reg_val[0], reg_val[1]);
          break;
        } else {
          dev_err(&client->dev, "[FTS] error Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
            reg_val[0], reg_val[1]);

          continue;
        }
    }
  if (i >= FTS_UPGRADE_LOOP )
    return -EIO;

  /*Step 4:erase app and panel paramenter area*/
  DBG("Step 4:erase app and panel paramenter area\n");
  auc_i2c_write_buf[0] = 0x61;
  fts_i2c_Write(client, auc_i2c_write_buf, 1);  //erase app area
  msleep(1350);

  for(i = 0;i < 15;i++)
  {
    auc_i2c_write_buf[0] = 0x6a;
    reg_val[0] = reg_val[1] = 0x00;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);

    if(0xF0==reg_val[0] && 0xAA==reg_val[1])
    {
      break;
    }
    msleep(50);

  }

  auc_i2c_write_buf[0] = 0xB0;
  auc_i2c_write_buf[1] = (u8) ((dw_lenth >> 16) & 0xFF);
  auc_i2c_write_buf[2] = (u8) ((dw_lenth >> 8) & 0xFF);
  auc_i2c_write_buf[3] = (u8) (dw_lenth & 0xFF);

  fts_i2c_Write(client, auc_i2c_write_buf, 4);

  /*********Step 5:write firmware(FW) to ctpm flash*********/
  bt_ecc = 0;
  DBG("Step 5:write firmware(FW) to ctpm flash\n");
  temp = 0;
  packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
  packet_buf[0] = 0xbf;
  packet_buf[1] = 0x00;
  for (j = 0; j < packet_number; j++) {
    temp = j * FTS_PACKET_LENGTH;
    packet_buf[2] = (u8) (temp >> 8);
    packet_buf[3] = (u8) temp;
    lenght = FTS_PACKET_LENGTH;
    packet_buf[4] = (u8) (lenght >> 8);
    packet_buf[5] = (u8) lenght;

    for (i = 0; i < FTS_PACKET_LENGTH; i++) {
      packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
      bt_ecc ^= packet_buf[6 + i];
    }
    fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);

    for(i = 0;i < 30;i++)
    {
      auc_i2c_write_buf[0] = 0x6a;
      reg_val[0] = reg_val[1] = 0x00;
      fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);
      if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
      {
        break;
      }
      msleep(1);
    }
  }

  if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
    temp = packet_number * FTS_PACKET_LENGTH;
    packet_buf[2] = (u8) (temp >> 8);
    packet_buf[3] = (u8) temp;
    temp = (dw_lenth) % FTS_PACKET_LENGTH;
    packet_buf[4] = (u8) (temp >> 8);
    packet_buf[5] = (u8) temp;

    for (i = 0; i < temp; i++) {
      packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
      bt_ecc ^= packet_buf[6 + i];
    }
    fts_i2c_Write(client, packet_buf, temp + 6);

    for(i = 0;i < 30;i++)
    {
      auc_i2c_write_buf[0] = 0x6a;
      reg_val[0] = reg_val[1] = 0x00;

      fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);

      if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
      {
        break;
      }
      msleep(1);

    }
  }

  msleep(50);

  /*********Step 6: read out checksum***********************/
  /*send the opration head */
  DBG("Step 6: read out checksum\n");
  auc_i2c_write_buf[0] = 0x64;
  fts_i2c_Write(client, auc_i2c_write_buf, 1);
  msleep(300);

  temp = 0;
  auc_i2c_write_buf[0] = 0x65;
  auc_i2c_write_buf[1] = (u8)(temp >> 16);
  auc_i2c_write_buf[2] = (u8)(temp >> 8);
  auc_i2c_write_buf[3] = (u8)(temp);
  temp = dw_lenth;
  auc_i2c_write_buf[4] = (u8)(temp >> 8);
  auc_i2c_write_buf[5] = (u8)(temp);
  i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 6);
  msleep(dw_lenth/256);

  for(i = 0;i < 100;i++)
  {
    auc_i2c_write_buf[0] = 0x6a;
    reg_val[0] = reg_val[1] = 0x00;
    fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);

    if (0xF0==reg_val[0] && 0x55==reg_val[1])
    {
      break;
    }
    msleep(1);

  }
  auc_i2c_write_buf[0] = 0x66;
  fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
  if (reg_val[0] != bt_ecc)
  {
    dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
          reg_val[0],
          bt_ecc);

    return -EIO;
  }
  printk(KERN_WARNING "checksum %X %X \n",reg_val[0],bt_ecc);
  /*********Step 7: reset the new FW***********************/
  DBG("Step 7: reset the new FW\n");
  auc_i2c_write_buf[0] = 0x07;
  fts_i2c_Write(client, auc_i2c_write_buf, 1);
  msleep(130);  //make sure CTP startup normally

  return 0;
}


/*sysfs debug*/

/*
*get firmware size

@firmware_name:firmware name
*note:the firmware default path is sdcard.
  if you want to change the dir, please modify by yourself.
*/
static int fts_GetFirmwareSize(char *firmware_name)
{
  struct file *pfile = NULL;
  struct inode *inode;
  unsigned long magic;
  off_t fsize = 0;
  char filepath[128];
  memset(filepath, 0, sizeof(filepath));

  sprintf(filepath, "%s", firmware_name);

  if (NULL == pfile)
    pfile = filp_open(filepath, O_RDONLY, 0);

  if (IS_ERR(pfile)) {
    pr_err("error occured while opening file %s.\n", filepath);
    return -EIO;
  }

  inode = pfile->f_dentry->d_inode;
  magic = inode->i_sb->s_magic;
  fsize = inode->i_size;
  filp_close(pfile, NULL);
  return fsize;
}



/*
*read firmware buf for .bin file.

@firmware_name: fireware name
@firmware_buf: data buf of fireware

note:the firmware default path is sdcard.
  if you want to change the dir, please modify by yourself.
*/
static int fts_ReadFirmware(char *firmware_name,
             unsigned char *firmware_buf)
{
  struct file *pfile = NULL;
  struct inode *inode;
  unsigned long magic;
  off_t fsize;
  char filepath[128];
  loff_t pos;
  mm_segment_t old_fs;

  memset(filepath, 0, sizeof(filepath));
  sprintf(filepath, "%s", firmware_name);
  if (NULL == pfile)
    pfile = filp_open(filepath, O_RDONLY, 0);
  if (IS_ERR(pfile)) {
    pr_err("error occured while opening file %s.\n", filepath);
    return -EIO;
  }

  inode = pfile->f_dentry->d_inode;
  magic = inode->i_sb->s_magic;
  fsize = inode->i_size;
  old_fs = get_fs();
  set_fs(KERNEL_DS);
  pos = 0;
  vfs_read(pfile, firmware_buf, fsize, &pos);
  filp_close(pfile, NULL);
  set_fs(old_fs);

  return 0;
}



/*
upgrade with *.bin file
*/

int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
               char *firmware_name)
{
    u8 *pbt_buf = NULL;
    int i_ret=0;
    int fwsize = fts_GetFirmwareSize(firmware_name);
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(0);apk_debug_flag = 1;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    if (fwsize <= 0) {
        dev_err(&client->dev, "%s ERROR:Get firmware size failed\n", __func__);
        //shihaobin@yulong.com modify for EDS protect begin 20170729
        #if FT_ESD_PROTECT
            esd_switch(1);apk_debug_flag = 0;
        #endif
        //shihaobin@yulong.com modify for EDS protect end 20170729
        return -EIO;
    }

    if (fwsize < 8 || fwsize > 32 * 1024) {
        dev_dbg(&client->dev, "%s:FW length error\n", __func__);
        //shihaobin@yulong.com modify for EDS protect begin 20170729
        #if FT_ESD_PROTECT
            esd_switch(1);apk_debug_flag = 0;
        #endif
        //shihaobin@yulong.com modify for EDS protect end 20170729
        return -EIO;
    }


    /*=========FW upgrade========================*/
    pbt_buf = kmalloc(fwsize + 1, GFP_ATOMIC);

    if (fts_ReadFirmware(firmware_name, pbt_buf)) {
        dev_err(&client->dev, "%s() - ERROR: request_firmware failed\n", __func__);
        kfree(pbt_buf);
        //return -EIO;
        i_ret = -EIO;
        goto err_ret;
    }

    /*call the upgrade function */
    //i_ret = fts_ctpm_fw_upgrade(client, pbt_buf, fwsize);
    i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf, fwsize);

    if (i_ret != 0)
    {
        dev_err(&client->dev, "%s() - ERROR:[FTS] upgrade failed..\n", __func__);
    }
    else if(fts_updateinfo_curr.AUTO_CLB==AUTO_CLB_NEED)
    {
        fts_ctpm_auto_clb(client);
    }

err_ret:

  kfree(pbt_buf);
  //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(1);apk_debug_flag = 0;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

  return i_ret;
}

static ssize_t fts_tpfwver_show(struct device *dev,
          struct device_attribute *attr,
          char *buf)
{
  ssize_t num_read_chars = 0;
  u8 fwver = 0;
  struct i2c_client *client = container_of(dev, struct i2c_client, dev);

  mutex_lock(&g_device_mutex);

  if (fts_read_reg(client, FTS_REG_FW_VER, &fwver) < 0)
    num_read_chars = snprintf(buf, PAGE_SIZE,
          "get tp fw version fail!\n");
  else
    num_read_chars = snprintf(buf, PAGE_SIZE, "%02X\n", fwver);

  mutex_unlock(&g_device_mutex);

  return num_read_chars;
}

static ssize_t fts_tpfwver_store(struct device *dev,
          struct device_attribute *attr,
          const char *buf, size_t count)
{
  /*place holder for future use*/
  return -EPERM;
}



static ssize_t fts_tprwreg_show(struct device *dev,
          struct device_attribute *attr,
          char *buf)
{
  /*place holder for future use*/
  return -EPERM;
}

static ssize_t fts_tprwreg_store(struct device *dev,
          struct device_attribute *attr,
          const char *buf, size_t count)
{
  struct i2c_client *client = container_of(dev, struct i2c_client, dev);
  ssize_t num_read_chars = 0;
  int retval;
  long unsigned int wmreg = 0;
  u8 regaddr = 0xff, regvalue = 0xff;
  u8 valbuf[5] = {0};

  memset(valbuf, 0, sizeof(valbuf));
  mutex_lock(&g_device_mutex);
  num_read_chars = count - 1;

  if (num_read_chars != 2) {
    if (num_read_chars != 4) {
      pr_info("please input 2 or 4 character\n");
      goto error_return;
    }
  }

  memcpy(valbuf, buf, num_read_chars);
  retval = strict_strtoul(valbuf, 16, &wmreg);

  if (0 != retval) {
    dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
            "given input to a number." \
            "The given input was: \"%s\"\n",
            __func__, buf);
    goto error_return;
  }

  if (2 == num_read_chars) {
    /*read register*/
    regaddr = wmreg;
    if (fts_read_reg(client, regaddr, &regvalue) < 0)
      dev_err(&client->dev, "Could not read the register(0x%02x)\n",
            regaddr);
    else
      pr_info("the register(0x%02x) is 0x%02x\n",
          regaddr, regvalue);
  } else {
    regaddr = wmreg >> 8;
    regvalue = wmreg;
    if (fts_write_reg(client, regaddr, regvalue) < 0)
      dev_err(&client->dev, "Could not write the register(0x%02x)\n",
              regaddr);
    else
      dev_err(&client->dev, "Write 0x%02x into register(0x%02x) successful\n",
              regvalue, regaddr);
  }

error_return:
  mutex_unlock(&g_device_mutex);

  return count;
}

static ssize_t fts_fwupdate_show(struct device *dev,
          struct device_attribute *attr,
          char *buf)
{
  /* place holder for future use */
  return -EPERM;
}

/*upgrade from *.i*/
static ssize_t fts_fwupdate_store(struct device *dev,
          struct device_attribute *attr,
          const char *buf, size_t count)
{
  struct fts_ts_data *data = NULL;
  u8 uc_host_fm_ver;
  int i_ret;
  struct i2c_client *client = container_of(dev, struct i2c_client, dev);

  data = (struct fts_ts_data *)i2c_get_clientdata(client);

  mutex_lock(&g_device_mutex);
  bootloader_ID=read_tp_bootloader_version();   //add by sunxuebin
  //disable_irq(client->irq);
  mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
  i_ret = fts_ctpm_fw_upgrade_with_i_file(bootloader_ID);   //modified by sunxuebin
  if (i_ret == 0) {
    msleep(300);
    uc_host_fm_ver = fts_ctpm_get_i_file_ver();
    pr_info("%s [FTS] upgrade to new version 0x%x\n", __func__,
           uc_host_fm_ver);
  } else
    dev_err(&client->dev, "%s ERROR:[FTS] upgrade failed.\n",
          __func__);

  //enable_irq(client->irq);
  mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
  mutex_unlock(&g_device_mutex);

  return count;
}

static ssize_t fts_fwupgradeapp_show(struct device *dev,
          struct device_attribute *attr,
          char *buf)
{
  /*place holder for future use*/
  return -EPERM;
}


/*upgrade from app.bin*/
static ssize_t fts_fwupgradeapp_store(struct device *dev,
          struct device_attribute *attr,
          const char *buf, size_t count)
{
  ssize_t num_read_chars = 0;
  char fwname[128];
  struct i2c_client *client = container_of(dev, struct i2c_client, dev);

  memset(fwname, 0, sizeof(fwname));
  sprintf(fwname, "%s", buf);
  fwname[count - 1] = '\0';

  mutex_lock(&g_device_mutex);
  //disable_irq(client->irq);
       mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
  if(0==fts_ctpm_fw_upgrade_with_app_file(client, fwname))
  {
    num_read_chars = snprintf(buf, PAGE_SIZE,
          "FTP firmware upgrade success!\n");
  }
  else
  {
    num_read_chars = snprintf(buf, PAGE_SIZE,
          "FTP firmware upgrade fail!\n");
  }

  //enable_irq(client->irq);
  mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
  mutex_unlock(&g_device_mutex);

  return num_read_chars;
}

/*touchscreen short test*/
//[BEGIN] liushilong@yulong.com add ft5446 shorttest function and file for 8681_A01/M01/M02 2015-5-23
#ifdef FT5446_openshort_test
static int ftxxxx_GetInISize(char *config_name)
{
    struct file *pfile = NULL;
    struct inode *inode;
    unsigned long magic;
    off_t fsize = 0;
    char filepath[128];
    memset(filepath, 0, sizeof(filepath));

    sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH, config_name);

    printk("wfs %s%s\n", FTXXXX_INI_FILEPATH, config_name);

    printk("wfs filepath is %s\n", filepath);

    if (NULL == pfile) {
        pfile = filp_open(filepath, O_RDONLY, 0);
        printk("wfs open file succeed\n");
    }

    if (IS_ERR(pfile)) {
        printk("error occured while opening file %s.\n", filepath);
        return -EIO;
    }

    inode = pfile->f_dentry->d_inode;
    magic = inode->i_sb->s_magic;
    fsize = inode->i_size;
    filp_close(pfile, NULL);

    printk("wfs fsize is %d\n", (int)fsize);

    return fsize;
}

static int ftxxxx_ReadInIData(char *config_name,
    char *config_buf)
{
    struct file *pfile = NULL;
    struct inode *inode;
    unsigned long magic;
    off_t fsize;
    char filepath[128];
    loff_t pos;
    mm_segment_t old_fs;

    memset(filepath, 0, sizeof(filepath));
    sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH, config_name);

    if (NULL == pfile)
        pfile = filp_open(filepath, O_RDONLY, 0);

    if (IS_ERR(pfile)) {
        printk("error occured while opening file %s.\n", filepath);
        return -EIO;
    }

    inode = pfile->f_dentry->d_inode;
    magic = inode->i_sb->s_magic;
    fsize = inode->i_size;
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_read(pfile, config_buf, fsize, &pos);
    filp_close(pfile, NULL);
    set_fs(old_fs);

    return 0;
}
static int ftxxxx_get_testparam_from_ini(char *config_name)
{
    char *filedata = NULL;

    int inisize = ftxxxx_GetInISize(config_name);

    printk("inisize = %d \n ", inisize);
    if (inisize <= 0) {
        printk("%s ERROR:Get firmware size failed\n",__func__);
        return -EIO;
    }

    filedata = kmalloc(inisize + 1, GFP_ATOMIC);

    if (ftxxxx_ReadInIData(config_name, filedata)) {
        printk("%s() - ERROR: request_firmware failed\n",
            __func__);
        kfree(filedata);
        return -EIO;
    } else {
        printk("ftxxxx_ReadInIData successful\n");
    }

    SetParamData(filedata);
    return 0;
}
//[Begin] liushilong@yulong.com mask scaptest output file 2015-6-1
#if 0
int CSV()
{
    int count = 0;//,frame_flag=0;
    //int err = 0;
    //int i = 0, j = 0,space=(11+TX_NUM);
    //int flag[8];
    //struct file *filp = NULL;
    //mm_segment_t oldfs = { 0 };


    struct file *pfile = NULL;
    struct file *pfile1 = NULL;
    struct inode *inode;
    unsigned long magic;
    off_t fsize = 0;
    loff_t pos;
    mm_segment_t old_fs;
    //char filepath[128];
    char *databuf = NULL;
    char *databuf1 = NULL;
    ssize_t err1;

    mutex_lock(&g_device_mutex);

    //memset(filepath, 0, sizeof(filepath));
    //sprintf(filepath, "/data/fts_scap_sample");
    //DBG("save auto test data to %s\n", filepath);

    databuf = kmalloc(10 * 1024, GFP_ATOMIC);
    if (databuf == NULL) {
        printk("alloc data buf fail !\n");
        return -1;
    }

    memset(databuf, 0, sizeof(databuf));

    databuf1 = kmalloc(10 * 1024, GFP_ATOMIC);
    if (databuf1 == NULL) {
        printk("alloc data buf fail !\n");
        return -1;
    }

    memset(databuf1, 0, sizeof(databuf1));
    //sprintf(databuf, "Project Code: %s\n", g_projectcode);


    if (NULL == pfile)
        pfile = filp_open("/data/1.csv", O_WRONLY|O_CREAT|O_TRUNC, 0777);

    if (IS_ERR(pfile)) {
        printk("error occured while opening file %s.\n", "");
        return -EIO;
    }

    old_fs = get_fs();
    set_fs(get_ds());
    inode = pfile->f_dentry->d_inode;
    magic = inode->i_sb->s_magic;
    fsize = inode->i_size;
    pos = 0;

    //printk("[Focal][%s]   456data result = !\n");
    count=testcsv(databuf);
    err1 = vfs_write(pfile, databuf, count, &pos);
    if (err1 < 0)
        DBG("write scap sample fail!\n");
    filp_close(pfile, NULL);
    set_fs(old_fs);
    //printk("[Focal][%s]   1111111111111 result = !\n");
    kfree(databuf);

    kfree(databuf1);
    mutex_unlock(&g_device_mutex);
    //printk("[Focal][%s]   878787 result = !\n");
    return 1;
}

int TXT()
{
    int count = 0;//,frame_flag=0;
    //int err = 0;
    //int i = 0, j = 0,space=(11+TX_NUM);
    //int flag[8];
    //struct file *filp = NULL;
    //mm_segment_t oldfs = { 0 };


    struct file *pfile = NULL;
    struct file *pfile1 = NULL;
    struct inode *inode;
    unsigned long magic;
    off_t fsize = 0;
    loff_t pos;
    mm_segment_t old_fs;
    //char filepath[128];
    char *databuf = NULL;
    char *databuf1 = NULL;
    ssize_t err1;

    mutex_lock(&g_device_mutex);

    //memset(filepath, 0, sizeof(filepath));
    //sprintf(filepath, "/data/fts_scap_sample");
    //DBG("save auto test data to %s\n", filepath);

    databuf = kmalloc(10 * 1024, GFP_ATOMIC);
    if (databuf == NULL) {
        printk("alloc data buf fail !\n");
        return -1;
    }

    memset(databuf, 0, sizeof(databuf));

    databuf1 = kmalloc(10 * 1024, GFP_ATOMIC);
    if (databuf1 == NULL) {
        printk("alloc data buf fail !\n");
        return -1;
    }

    memset(databuf1, 0, sizeof(databuf1));
    //sprintf(databuf, "Project Code: %s\n", g_projectcode);


    if (NULL == pfile)
        pfile = filp_open("/data/1.txt", O_WRONLY|O_CREAT|O_TRUNC, 0777);

    if (IS_ERR(pfile)) {
        printk("error occured while opening file %s.\n", "");
        return -EIO;
    }

    old_fs = get_fs();
    set_fs(get_ds());
    inode = pfile->f_dentry->d_inode;
    magic = inode->i_sb->s_magic;
    fsize = inode->i_size;
    pos = 0;

    count=testtxt(databuf);

    err1 = vfs_write(pfile, databuf, count, &pos);
    if (err1 < 0)
        DBG("write scap sample fail!\n");
    filp_close(pfile, NULL);
    set_fs(old_fs);
    //printk("[Focal][%s]   1111111111111 result = !\n");
    kfree(databuf);

    //filp->f_op->write(filp, buf, count, &filp->f_pos);
    //set_fs(oldfs);
    //filp_close(filp, NULL);


    kfree(databuf1);
    mutex_unlock(&g_device_mutex);
    //printk("[Focal][%s]   878787 result = !\n");
    return 1;
}
#endif
//[END]

static ssize_t ftxxxx_ftsscaptest_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
    //[BEGIN] liushilong@yulong.com mask output file for 8681 2015-5-25
    //TXT();
    //CSV();
    //[END]
    return sprintf(buf, "%d\n", shorttest);

}
//zax 20141116 --------------------
static ssize_t ftxxxx_ftsscaptest_store(struct device *dev,
struct device_attribute *attr,
    const char *buf, size_t count)
{
//[BEGIN] liushilong@yulong.com set ftsscaptest fail and do nothing
#if 1
    /* place holder for future use */
    char cfgname[128];
    //short *TestData=NULL;
    int iTxNumber=0;
    int iRxNumber=0;
    int i=0, j=0;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);
    G_Client=client;

    //  struct i2c_client *client = container_of(dev, struct i2c_client, dev);
    memset(cfgname, 0, sizeof(cfgname));
    sprintf(cfgname, "%s", buf);
    cfgname[count-1] = '\0';

    //Init_I2C_Read_Func(FTS_I2c_Read);
    //Init_I2C_Write_Func(FTS_I2c_Write);
    //StartTestTP();

    mutex_lock(&g_device_mutex);

    Init_I2C_Write_Func(FTS_I2c_Write_ShortTest);
    Init_I2C_Read_Func(FTS_I2c_Read_ShortTest);
    if(ftxxxx_get_testparam_from_ini(cfgname) <0)
        printk("get testparam from ini failure\n");
    else {
#if 1
        printk("tp test Start...\n");
        if(true == StartTestTP())
        {
            shorttest = true;
            printk("tp test pass\n");
        }
        else
        {
            shorttest = false;
            printk("tp test failure\n");
        }
#endif
    }

    mutex_unlock(&g_device_mutex);
    //[BEING] liushilong@yulong.com mask rawdata output file when write ftsscaptest 2015-5-26
    //TXT();
    //CSV();
    //[END]
#endif
//[END]
    return count;
}
#endif  //FT5446_openshort_test
//[END]

/*sysfs */
/*get the fw version
*example:cat ftstpfwver
*/
static DEVICE_ATTR(ftstpfwver, S_IRUGO | S_IWUSR, fts_tpfwver_show,
      fts_tpfwver_store);

/*upgrade from *.i
*example: echo 1 > ftsfwupdate
*/
static DEVICE_ATTR(ftsfwupdate, S_IRUGO | S_IWUSR, fts_fwupdate_show,
      fts_fwupdate_store);

/*read and write register
*read example: echo 88 > ftstprwreg ---read register 0x88
*write example:echo 8807 > ftstprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(ftstprwreg, S_IRUGO | S_IWUSR, fts_tprwreg_show,
      fts_tprwreg_store);



//shihaobin@yulong.com add for tw test debug begin 20150723
static ssize_t ft5x46_show_tw_reg_data(struct device *dev,
          struct device_attribute *attr,
          const char *buf)
{
  struct i2c_client *client = container_of(dev, struct i2c_client, dev);
  unsigned char reg_buf[5] = {0xa5, 0xa7, 0xad, 0xfd, 0xfe};
  int ret = -1;
  char data_buf[5];
  
  mutex_lock(&g_device_mutex);
  ret = fts_read_reg(client, reg_buf[0], &data_buf[0]);
  ret = fts_read_reg(client, reg_buf[1], &data_buf[1]);
  ret = fts_read_reg(client, reg_buf[2], &data_buf[2]);
  ret = fts_read_reg(client, reg_buf[3], &data_buf[3]);
  ret = fts_read_reg(client, reg_buf[4], &data_buf[4]);
  mutex_unlock(&g_device_mutex);
  
  if (ret < 0)
  {
    printk("[FTS] read debug reg error!\n");
    return sprintf(buf, "%d\n", ret);
  }
  else
  {
    printk("[FTS] read debug reg[0xa5]= 0x%x, reg[0xa7]= 0x%x, reg[0xad]= 0x%x, reg[0xfd]= 0x%x, reg[0xfe]= 0x%x\n",
                 data_buf[0],data_buf[1],data_buf[2],data_buf[3],data_buf[4]);
    return sprintf(buf, "reg[0xa5]= 0x%x, reg[0xa7]= 0x%x, reg[0xad]= 0x%x, reg[0xfd]= 0x%x, reg[0xfe]= 0x%x\n",
                    data_buf[0],data_buf[1],data_buf[2],data_buf[3],data_buf[4]);
  }

}

static DEVICE_ATTR(ftsdebug_reg, S_IRUGO | S_IWUSR, ft5x46_show_tw_reg_data,
      NULL);

//shihaobin@yulong.com add for tw test debug end 20150723


/*upgrade from app.bin
*example:echo "*_app.bin" > ftsfwupgradeapp
*/
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO | S_IWUSR, fts_fwupgradeapp_show,
      fts_fwupgradeapp_store);

//[BEGIN] liushilong@yulong.com add ft5446 ftsscaptest file node for get and set status 2015-5-25
#ifdef  FT5446_openshort_test
static DEVICE_ATTR(ftsscaptest, 0664, ftxxxx_ftsscaptest_show, ftxxxx_ftsscaptest_store);
#endif   //FT5446_openshort_test

/*add your attr in here*/
static struct attribute *fts_attributes[] = {
  //&dev_attr_ftstpfwver.attr,
  //&dev_attr_ftsfwupdate.attr,
  //&dev_attr_ftstprwreg.attr,
  //&dev_attr_ftsfwupgradeapp.attr,
#ifdef  FT5446_openshort_test
    &dev_attr_ftsscaptest.attr,
#endif  //FT5446_openshort_test
    &dev_attr_ftsdebug_reg.attr,    //shihaobin@yulong.com add for tw test debug 20150723
  NULL
};
//[END] for shorttest

static struct attribute_group fts_attribute_group = {
  .attrs = fts_attributes
};

/*create sysfs for debug*/
int fts_create_sysfs(struct i2c_client *client)
{
    int err;
#if 0
//#if TPD_SUPPORT_I2C_DMA
    I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, FTS_DMA_BUF_SIZE, &I2CDMABuf_pa, GFP_KERNEL);
    if(!I2CDMABuf_va)
    {
        dev_dbg(&client->dev,"%s Allocate DMA I2C Buffer failed!\n",__func__);
        return -EIO;
    }
    //printk("FTP: I2CDMABuf_pa=%x,val=%x val2=%x\n",&I2CDMABuf_pa,I2CDMABuf_pa,(unsigned char *)I2CDMABuf_pa);
#endif

    err = sysfs_create_group(&client->dev.kobj, &fts_attribute_group);
    if (0 != err) {
        dev_err(&client->dev,"%s() - ERROR: sysfs_create_group() failed.\n", __func__);
        sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
        return -EIO;
    } else {
        mutex_init(&g_device_mutex);
        pr_info("ft6x06:%s() - sysfs_create_group() succeeded.\n", __func__);
    }
    return err;
}

void fts_release_sysfs(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
    mutex_destroy(&g_device_mutex);
#if 0
//#if TPD_SUPPORT_I2C_DMA
    if(I2CDMABuf_va)
    {
        dma_free_coherent(NULL, FTS_DMA_BUF_SIZE, I2CDMABuf_va, I2CDMABuf_pa);
        I2CDMABuf_va = NULL;
        I2CDMABuf_pa = NULL;
    }
#endif
}

/*create apk debug channel*/

#define PROC_UPGRADE            0
#define PROC_READ_REGISTER      1
#define PROC_WRITE_REGISTER     2
#define PROC_RAWDATA            3
#define PROC_AUTOCLB            4
#define PROC_UPGRADE_INFO       5
#define PROC_WRITE_DATA         6
#define PROC_READ_DATA          7

#define PROC_NAME   "ft5x0x-debug"
static unsigned char proc_operate_mode = PROC_RAWDATA;
static struct proc_dir_entry *ft5x0x_proc_entry;

/*interface of write proc*/
static int ft5x0x_debug_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
    struct i2c_client *client = PDE_DATA(file_inode(filp));
    unsigned char writebuf[FTS_PACKET_LENGTH];
    int buflen = count;
    int writelen = 0;
    int ret = 0;
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(0);apk_debug_flag = 1;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    if (copy_from_user(&writebuf, buf, buflen))
    {
        printk( "%s:copy from user error\n", __func__);
        //shihaobin@yulong.com modify for EDS protect begin 20170729
        #if FT_ESD_PROTECT
            esd_switch(1);apk_debug_flag = 0;
        #endif
        //shihaobin@yulong.com modify for EDS protect end 20170729
        return -EFAULT;
    }
    proc_operate_mode = writebuf[0];

    switch (proc_operate_mode)
    {
    case PROC_UPGRADE:
    {
        char upgrade_file_path[128];
        memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
        sprintf(upgrade_file_path, "%s", writebuf + 1);
        upgrade_file_path[buflen-1] = '\0';
        printk("%s\n", upgrade_file_path);
        disable_irq(client->irq);

        ret = fts_ctpm_fw_upgrade_with_app_file(client, upgrade_file_path);

        enable_irq(client->irq);
        if (ret < 0)
        {
            printk( "%s:upgrade failed.\n", __func__);
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(1);apk_debug_flag = 0;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            return ret;
        }
    }
    break;
    case PROC_READ_REGISTER:
        writelen = 1;
        printk("%s:register addr=0x%02x\n", __func__, writebuf[1]);
        ret = fts_i2c_Write(client, writebuf + 1, writelen);
        if (ret < 0)
        {
            printk( "%s:write iic error\n", __func__);
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(1);apk_debug_flag = 0;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            return ret;
        }
        break;
    case PROC_WRITE_REGISTER:
        writelen = 2;
        ret = fts_i2c_Write(client, writebuf + 1, writelen);
        if (ret < 0)
        {
            printk( "%s:write iic error\n", __func__);
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(1);apk_debug_flag = 0;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            return ret;
        }
        break;
    case PROC_RAWDATA:
        break;
    case PROC_AUTOCLB:
        fts_ctpm_auto_clb(client);
        break;
    case PROC_READ_DATA:
    case PROC_WRITE_DATA:
        writelen = count - 1;
        ret = fts_i2c_Write(client, writebuf + 1, writelen);
        if (ret < 0)
        {
            printk( "%s:write iic error\n", __func__);
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(1);apk_debug_flag = 0;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            return ret;
        }
        break;

    default:
        break;
    }
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(1);apk_debug_flag = 0;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    return count;
}

/*interface of read proc*/
static int ft5x0x_debug_read(struct file *filp,char __user *buf,size_t count, loff_t *offp)
{
    struct i2c_client *client = PDE_DATA(file_inode(filp));
    int ret = 0, err = 0;
    u8 tx = 0, rx = 0;
    int i, j;
    unsigned char tmp[PAGE_SIZE];
    int num_read_chars = 0;
    int readlen = 0;
    u8 regvalue = 0x00, regaddr = 0x00;
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(0);apk_debug_flag = 1;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729
    switch (proc_operate_mode)
    {
    case PROC_UPGRADE:
        /*after calling ft5x0x_debug_write to upgrade*/
        regaddr = 0xA6;
        ret = fts_read_reg(client, regaddr, &regvalue);
        if (ret < 0)
            num_read_chars = sprintf(tmp, "%s", "get fw version failed.\n");
        else
            num_read_chars = sprintf(tmp, "current fw version:0x%02x\n", regvalue);
        break;
    case PROC_READ_REGISTER:
        readlen = 1;
        ret = fts_i2c_Read(client, NULL, 0, tmp, readlen);
        if (ret < 0)
        {
            printk( "%s:read iic error\n", __func__);
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(1);apk_debug_flag = 0;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            return ret;
        }
        else
            printk("%s:value=0x%02x\n", __func__, tmp[0]);
        num_read_chars = 1;
        break;
//[BEGIN] liushilong@yulong.com add apk debug read data for get data 2015-5-27
    case PROC_READ_DATA:
        readlen = count; 
        ret = fts_i2c_Read(client, NULL, 0, tmp, readlen); 
        if (ret < 0) {
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(1);apk_debug_flag = 0;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            dev_err(&client->dev, "%s:read iic error\n", __func__); 
            return ret;
        }
        num_read_chars = readlen;
        break;
//[END]
    case PROC_RAWDATA:
        break;
    default:
        break;
    }

    copy_to_user(buf, tmp, num_read_chars);
    //memcpy(page, buf, num_read_chars);
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(1);apk_debug_flag = 0;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    return num_read_chars;
}

static struct file_operations ft5x0x_proc_fops = {
    .read = ft5x0x_debug_read,
    .write = ft5x0x_debug_write,
};

int ft5x0x_create_apk_debug_channel(struct i2c_client * client)
{
    ft5x0x_proc_entry = proc_create_data(PROC_NAME, 0777, NULL, &ft5x0x_proc_fops, client);
    if (NULL == ft5x0x_proc_entry)
    {
        printk( "Couldn't create proc entry!\n");
        return -ENOMEM;
    }
    /*
    else
    {
        dev_info(&client->dev, "Create proc entry success!\n");
        ft5x0x_proc_entry->data = client;
        ft5x0x_proc_entry->write_proc = ft5x0x_debug_write;
        ft5x0x_proc_entry->read_proc = ft5x0x_debug_read;
    }
    */
    return 0;
    }

void ft5x0x_release_apk_debug_channel(void)
{
    if (ft5x0x_proc_entry)
        remove_proc_entry(PROC_NAME, NULL);
}
