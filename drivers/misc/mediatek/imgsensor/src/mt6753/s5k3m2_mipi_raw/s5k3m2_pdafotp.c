#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/xlog.h>


#define PFX "S5K3M2_pdafotp"
#define LOG_INF(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

#define S5K3M2_EEPROM_READ_ID  0xA0
#define S5K3M2_EEPROM_WRITE_ID   0xA3
#define S5K3M2_I2C_SPEED        100 //modified by gaoatao@yulong.com for i2c speed 100k 2015-5-19 19:59:30
#define S5K3M2_MAX_OFFSET		0xFFFF

#define DATA_SIZE 2048
BYTE s5k3m2_eeprom_data[DATA_SIZE]= {0};
static bool get_done = false;
static int last_size = 0;
static int last_offset = 0;


static bool selective_read_eeprom(kal_uint16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    if(addr > S5K3M2_MAX_OFFSET)
        return false;
	kdSetI2CSpeed(S5K3M2_I2C_SPEED);

	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, S5K3M2_EEPROM_READ_ID)<0)
		return false;
//    LOG_INF(" gaoatao pdaf iReadCAM_CAL_16 addr =0x%x data = 0x%x \n", addr, *data);
    return true;
}

static bool _read_3m2_eeprom(kal_uint16 addr, BYTE* data, kal_uint32 size ){
	int i = 0;
	int offset = addr;
	for(i = 0; i < size; i++) {
		if(!selective_read_eeprom(offset, &data[i])){
			return false;
		}
		offset++;
	}
	get_done = true;
	last_size = size;
	last_offset = addr;
    return true;
}

bool read_3m2_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size){
/*
	if(!get_done || last_size != size || last_offset != addr) {
		if(!_read_3m2_eeprom(addr, s5k3m2_eeprom_data, size)){
			get_done = 0;
            last_size = 0;
            last_offset = 0;
			return false;
		}
	}

	memcpy(data, s5k3m2_eeprom_data, size);
    return true;
    */

	LOG_INF("read 3m2 eeprom, addr = %d\n", addr);
	LOG_INF("read 3m2 eeprom, size = %d\n", size);

	int i = 0;
	kal_uint16 start = 0x765;
	kal_uint32 offset = 0;
	kal_uint32 length = 496;

	if(!_read_3m2_eeprom(start, s5k3m2_eeprom_data+offset, length)){
		get_done = 0;
              last_size = 0;
              last_offset = 0;
		return false;
	}

 	start=0x957;
	offset += length;
	length = 806;
	if(!_read_3m2_eeprom(start, s5k3m2_eeprom_data+offset, length)){
		get_done = 0;
              last_size = 0;
              last_offset = 0;
		return false;
	}

 	start=0xc7f;
	offset += length;
	length = 102;
	if(!_read_3m2_eeprom(start, s5k3m2_eeprom_data+offset, length)){
		get_done = 0;
              last_size = 0;
              last_offset = 0;
		return false;
	}

	last_size = offset + length;
	memcpy(data, s5k3m2_eeprom_data, size);

	return true;
}


