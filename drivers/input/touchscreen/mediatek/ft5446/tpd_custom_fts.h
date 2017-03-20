/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
//#include <linux/io.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rtpm_prio.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>
#include <cust_eint.h>
#include <linux/jiffies.h>
#include <pmic_drv.h>


struct Upgrade_Info {
    u8 CHIP_ID;
    u8 FTS_NAME[20];
    u8 TPD_MAX_POINTS;
    u8 AUTO_CLB;
    u16 delay_aa;           /*delay of write FT_UPGRADE_AA */
    u16 delay_55;           /*delay of write FT_UPGRADE_55 */
    u8 upgrade_id_1;        /*upgrade id 1 */
    u8 upgrade_id_2;        /*upgrade id 2 */
    u16 delay_readid;       /*delay of read id */
    u16 delay_earse_flash;  /*delay of earse flash*/
};

extern struct Upgrade_Info fts_updateinfo_curr;

/**********************Custom define begin**********************************************/

#define TW_OFILM_ID             0x51
#define TW_YEJI_ID              0x80
#define TW_BOEN_ID              0x3B

//[BEGIN] liushilong@yulong.com modify ofilm/boen firmware for v19 2015-7-15
static unsigned char OFILM_CTPM_FW000[]=
{
#include "coolpad_8681_0x51_001_V1c_D01_20150801_app.i"
};

static unsigned char YEJI_CTPM_FW000[]=
{
#include "coolpad_8681_0x80_001_V16_D01_20150703_app.i"
};

static unsigned char BOEN_CTPM_FW000[]=
{
#include "coolpad_8681_0x3B_001_V1c_D01_20150801_app.i"
};


//define golden firmware file
static unsigned char OFILM_CTPM_FW001[]=
{
#include "coolpad_8681_0x51_00G_V1c_D01_20150801_app.i"
};

static unsigned char YEJI_CTPM_FW001[]=
{
#include "coolpad_8681_0x80_00G_V16_D01_20150703_app.i"
};

static unsigned char BOEN_CTPM_FW001[]=
{
#include "coolpad_8681_0x3B_00G_V1c_D01_20150801_app.i"
};
//[END]

/**********************Params define begin**********************************************/

//add by liyongfeng end
/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE
#define TPD_I2C_NUMBER           1
#define TPD_WAKEUP_TRIAL         60
#define TPD_WAKEUP_DELAY         100

#define TPD_SUPPORT_I2C_DMA   0       //better enable it if hardware platform supported

#define TPD_VELOCITY_CUSTOM_X 15
#define TPD_VELOCITY_CUSTOM_Y 20

#define TPD_POWER_SOURCE_CUSTOM  PMIC_APP_CAP_TOUCH_VDD

#define TPD_DELAY                (2*HZ/100)
#define TPD_RES_X                1080
#define TPD_RES_Y                1920
#define TPD_CALIBRATION_MATRIX  {962,0,0,0,1600,0,0,0};

#define TPD_OK                    0
#define TPD_MAX_RESET_COUNT       3
#define FTS_HAVE_TOUCH_KEY        1

//#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_TREMBLE_ELIMINATION
//#define TPD_HAVE_BUTTON
//#define TPD_BUTTON_WIDTH        (80)
//#define TPD_BUTTON_HEIGH        (60)
//#define TPD_KEY_COUNT           3
//#define TPD_KEYS                {KEY_MENU,KEY_HOMEPAGE,KEY_BACK}
//#define TPD_KEYS_DIM            {{270,2000,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH},{540,2000,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH},{810,2000,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH}}


#define TPD_MAX_POINTS_2                        2
#define TPD_MAX_POINTS_5                        5
#define TPD_MAXPOINTS_10                        10
#define AUTO_CLB_NEED                           1
#define AUTO_CLB_NONEED                         0

#define FTS_NULL               0x0
#define FTS_TRUE               0x01
#define FTS_FALSE              0x0
#define I2C_CTPM_ADDRESS       0x70

//register define
#define DEVICE_MODE            0x00
#define GEST_ID                0x01
#define TD_STATUS              0x02
//point1 info from 0x03~0x08
//point2 info from 0x09~0x0E
//point3 info from 0x0F~0x14
//point4 info from 0x15~0x1A
//point5 info from 0x1B~0x20
//register define

// if need the gesture funtion,enable this MACRO
#define FTS_GESTRUE

// window and glove mode
#define TW_GLOVE_SWITCH 1
#define TW_WINDOW_SWITCH 1
#define CONFIG_COVER_WINDOW_SIZE

// apk_debug
#define FTS_APK_DEBUG

/******************************************************************************/
/*Chip Device Type*/
#define IC_FT5X06           0 /*x=2,3,4*/
#define IC_FT5606           1 /*ft5506/FT5606/FT5816*/
#define IC_FT5316           2 /*ft5x16*/
#define IC_FT6208           3   /*ft6208*/
#define IC_FT6x06               4 /*ft6206/FT6306*/
#define IC_FT5x06i              5 /*ft5306i*/
#define IC_FT5x36               6 /*ft5336/ft5436/FT5436i*/
#define I2C_MASTER_CLOCK     80
#define DEVICE_IC_TYPE          IC_FT5x36

/*register address*/
#define FTS_REG_CHIP_ID       0xA3    //chip ID
#define FTS_REG_FW_VER        0xA6   //FW  version
#define FTS_REG_VENDOR_ID     0xA8   // TP vendor ID
#define FTS_REG_POINT_RATE      0x88   //report rate

#define TPD_SYSFS_DEBUG
#define FTS_CTL_IIC
#ifdef TPD_SYSFS_DEBUG
//#define TPD_AUTO_UPGRADE        // if need upgrade CTP FW when POWER ON,pls enable this MACRO
#endif

//shihaobin@yulong.com modify for EDS protect begin 20170729
#define FT_ESD_PROTECT          0
#if FT_ESD_PROTECT
void esd_switch(s32 on);
#endif
//shihaobin@yulong.com modify for EDS protect end 20170729

#define FTS_DBG
#ifdef FTS_DBG
#define DBG(fmt, args...)         printk("[FTS]" fmt, ## args)
#else
#define DBG(fmt, args...)         do{}while(0)
#endif
//add by liyongfeng start
enum fts_ts_regs {
  FTS_REG_THGROUP     = 0x80, /* touch threshold, related to sensitivity */
  FTS_REG_THPEAK      = 0x81,
  FTS_REG_THCAL     = 0x82,
  FTS_REG_THWATER     = 0x83,
  FTS_REG_THTEMP      = 0x84,
  FTS_REG_THDIFF      = 0x85,
  FTS_REG_CTRL        = 0x86,
  FTS_REG_TIMEENTERMONITOR    = 0x87,
  FTS_REG_PERIODACTIVE      = 0x88,      /* report rate */
  FTS_REG_PERIODMONITOR   = 0x89,
  FTS_REG_HEIGHT_B      = 0x8a,
  FTS_REG_MAX_FRAME     = 0x8b,
  FTS_REG_DIST_MOVE     = 0x8c,
  FTS_REG_DIST_POINT      = 0x8d,
  FTS_REG_FEG_FRAME     = 0x8e,
  FTS_REG_SINGLE_CLICK_OFFSET   = 0x8f,
  FTS_REG_DOUBLE_CLICK_TIME_MIN = 0x90,
  FTS_REG_SINGLE_CLICK_TIME   = 0x91,
  FTS_REG_LEFT_RIGHT_OFFSET   = 0x92,
  FTS_REG_UP_DOWN_OFFSET    = 0x93,
  FTS_REG_DISTANCE_LEFT_RIGHT   = 0x94,
  FTS_REG_DISTANCE_UP_DOWN    = 0x95,
  FTS_REG_ZOOM_DIS_SQR      = 0x96,
  FTS_REG_RADIAN_VALUE      = 0x97,
  FTS_REG_MAX_X_HIGH      = 0x98,
  FTS_REG_MAX_X_LOW     = 0x99,
  FTS_REG_MAX_Y_HIGH      = 0x9a,
  FTS_REG_MAX_Y_LOW     = 0x9b,
  FTS_REG_K_X_HIGH      = 0x9c,
  FTS_REG_K_X_LOW     = 0x9d,
  FTS_REG_K_Y_HIGH      = 0x9e,
  FTS_REG_K_Y_LOW     = 0x9f,
  FTS_REG_AUTO_CLB_MODE   = 0xa0,
  FTS_REG_LIB_VERSION_H   = 0xa1,
  FTS_REG_LIB_VERSION_L   = 0xa2,
  FTS_REG_CHIPID      = 0xa3,
  FTS_REG_MODE        = 0xa4,
  FTS_REG_PMODE     = 0xa5,   /* Power Consume Mode   */
  FTS_REG_FIRMID      = 0xa6,   /* Firmware version */
  FTS_REG_STATE     = 0xa7,
  FTS_REG_VENDORID      = 0xa8,
  FTS_REG_ERR       = 0xa9,
  FTS_REG_CLB       = 0xaa,
};
//add by liyongfeng end
#endif /* TOUCHPANEL_H__ */
