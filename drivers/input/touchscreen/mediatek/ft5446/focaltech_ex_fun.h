#ifndef __FOCALTECH_EX_FUN_H__
#define __FOCALTECH_EX_FUN_H__

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>

#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>

#define FTS_UPGRADE_AA          0xAA
#define FTS_UPGRADE_55          0x55

#define BL_VERSION_LZ4        0
#define BL_VERSION_Z7         1
#define BL_VERSION_GZF        2

/*****************************************************************************/
//liushilong@yulong.com modify i2c transfer length threshold, it should less than 8 byte
#define FTS_PACKET_LENGTH           128
#define FTS_SETTING_BUF_LEN         128
#define FTS_DMA_BUF_SIZE        4096

#define FTS_UPGRADE_LOOP        3

#define FTS_FACTORYMODE_VALUE     0x40
#define FTS_WORKMODE_VALUE        0x00

int fts_ctpm_auto_upgrade(struct i2c_client *client);

/*create sysfs for debug*/
int fts_create_sysfs(struct i2c_client * client);
void fts_release_sysfs(struct i2c_client * client);

int ft5x0x_create_apk_debug_channel(struct i2c_client *client);
void ft5x0x_release_apk_debug_channel(void);

//[BEGIN] liushilong@yulong.com add tw shorttest switch 2015-5-23
#define FT5446_openshort_test
//[END]

/*
*fts_write_reg- write register
*@client: handle of i2c
*@regaddr: register address
*@regvalue: register value
*
*/
extern int fts_i2c_Read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen);
extern int fts_i2c_Write(struct i2c_client *client, char *writebuf, int writelen);
extern int fts_write_reg(struct i2c_client * client,u8 regaddr, u8 regvalue);
extern int fts_read_reg(struct i2c_client * client,u8 regaddr, u8 *regvalue);

//[BEGIN] liushilong@yulong.com open shorttest i2c read/write function 2015-5-23
#ifdef	FT5446_openshort_test
int FTS_I2c_Read_ShortTest(unsigned char * wBuf, int wLen, unsigned char *rBuf, int rLen);
int FTS_I2c_Write_ShortTest(unsigned char * wBuf, int wLen);
#endif	//FT5446_openshort_test
//[END]

#endif
