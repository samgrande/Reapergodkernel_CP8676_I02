/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 OV8865qtechmipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *---------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <asm/system.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov8865qtechmipiraw_Sensor.h"

#define PFX "OV8865_qtech_camera_sensor"
//#define LOG_WRN(format, args...) xlog_printk(ANDROID_LOG_WARN ,PFX, "[%S] " format, __FUNCTION__, ##args)
//#defineLOG_INF(format, args...) xlog_printk(ANDROID_LOG_INFO ,PFX, "[%s] " format, __FUNCTION__, ##args)
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)

#define OV8865_QTECH_MODULE_ID 0x06

static DEFINE_SPINLOCK(imgsensor_drv_lock);
extern int get_device_info(char* buf);//add for factory by wangyi
static int info_limit = 0;
static char factory_module_id[50] = {0};//modify module_id length by weiwenping@yulong.com 20150612
static  kal_uint16 g_otp_id = 0;
#define i2c_Speed 100  //add by gaoatao@yulong.com for i2c speed to 100k 2015-5-26 13:47:19

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = OV8865_SENSOR_ID,

	.checksum_value = 0xfdd9c02a,  //0xa2230d9f,//rome check

	.pre = {
		.pclk = 74400000,									//record different mode's pclk
		.linelength = 1923,  /*[actually 1923*2]	*/			//record different mode's linelength
		.framelength = 1280, //1248 =>1280 ,increase vblank time	//record different mode's framelength
		.startx = 0,										//record different mode's startx of grabwindow
		.starty = 0,										//record different mode's starty of grabwindow
		.grabwindow_width = 1632,							//record different mode's width of grabwindow
		.grabwindow_height = 1224,							//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 23,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap = {//30fps 74.4M bps/lane
		.pclk = 148800000,//this value just for calculate shutter
		.linelength = 1944,  /*[actually 2008*2]	*/
	    .framelength = 2550,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 23,
		.max_framerate = 300,
	},
	.cap1 = {//24fps 600M bps/lane
		.pclk = 120000000,//this value just for calculate shutter
		.linelength = 1944,  /*[actually 2008*2]	*/
		.framelength = 4080,//modify for 15fps at mt6735 //2550,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 23,
		.max_framerate = 150,	//less than 13M(include 13M),cap1 max framerate is 24fps,16M max framerate is 20fps, 20M max framerate is 15fps
	},
	.normal_video = {
		.pclk = 148800000,
		.linelength = 2582,       /*[actually 2582*2]	*/
		.framelength = 1908,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 1836,
		.mipi_data_lp2hs_settle_dc = 23,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 74400000,
		.linelength = 1153, /*[actually 1153*2]	*/
		.framelength = 537,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640,
		.grabwindow_height = 480,
		.mipi_data_lp2hs_settle_dc = 23,
		.max_framerate = 1200,
	},
	.slim_video = { //equal preview setting
		.pclk = 74400000,
		.linelength = 1923,  /*[actually 1923*2]	*/		//record different mode's linelength
		.framelength = 1280,								//record different mode's framelength
		.startx = 0,										//record different mode's startx of grabwindow
		.starty = 0,										//record different mode's starty of grabwindow
		.grabwindow_width = 1632,							//record different mode's width of grabwindow
		.grabwindow_height = 1224,							//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 23,
		.max_framerate = 300,
	},

	.margin = 6,
	.min_shutter = 1,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num ,don't support Slow motion

	.cap_delay_frame = 2,
	.pre_delay_frame = 2,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,//0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x6C,0xff},
};




static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not.
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x6C,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 3264, 2448, 0, 0, 3264, 2448,    1632,1224,0,0,  1632,1224,	  0,0,1632,1224}, // Preview
 { 3264, 2448, 0, 0, 3264, 2448, 	3264,2448,0,0,  3264,2448,	  0,0,3264,2448}, // capture
 { 3264, 2448, 0, 0, 3264, 2448, 	3264,2448,0,306,3264,1836,	  0,0,3264,1836},// video
 { 3264, 2448, 0, 0, 3264, 2448, 	3264,2448,0,0,  640, 480,	  0,0,640 ,480},//hight speed video  //hkm need check
 { 3264, 2448, 0, 0, 3264, 2448, 	1632,1224,0,0,  1632,1224,	  0,0,1632,1224}};// slim video


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    kdSetI2CSpeed(i2c_Speed);//add by gaoatao@yulong.com for i2c speed to 100k 2015-5-26 13:47:19
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    kdSetI2CSpeed(i2c_Speed);//add by gaoatao@yulong.com for i2c speed to 100k 2015-5-26 13:47:19
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}
//ov8865 otp ADD
static kal_uint16 OV8865_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    kdSetI2CSpeed(i2c_Speed);//add by gaoatao@yulong.com for i2c speed to 100k 2015-5-26 13:47:19
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void OV8865_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    kdSetI2CSpeed(i2c_Speed);//add by gaoatao@yulong.com for i2c speed to 100k 2015-5-26 13:47:19
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}
//#define RG_Ratio_Typical                                                282 // typical value for Sunny module AF( Yulong project )
//#define BG_Ratio_Typical                                                286 // typical value for Sunny module AF( Yulong project )
struct otp_struct {
int module_integrator_id;
int lens_id;
int production_year;
int production_month;
int production_day;
int rg_ratio;
int bg_ratio;
int light_rg;
int light_bg;
int lenc[62];
int VCM_start;
int VCM_end;
int VCM_dir;
};
static struct otp_struct current_otp;
static int BG_Ratio_Typical_8865;
static int RG_Ratio_Typical_8865;
// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int check_otp_info(int index)  //(struct msm_sensor_ctrl_t *s_ctrl, int index)
{
	uint16_t flag;
	OV8865_write_cmos_sensor(0x3d84,0xC0);
	OV8865_write_cmos_sensor(0x3d88,0x70);
	OV8865_write_cmos_sensor(0x3d89,0x10);
	OV8865_write_cmos_sensor(0x3d8A,0x70);
	OV8865_write_cmos_sensor(0x3d8B,0x10);
        OV8865_write_cmos_sensor(0x3d81,0x01);
	msleep(5);
        flag=OV8865_read_cmos_sensor(0x7010);
	if (index == 1) {
		flag = (flag>>6) & 0x03;
	} else if (index == 2) {
		flag = (flag>>4) & 0x03;
	} else if (index ==3) {
		flag = (flag>>2) & 0x03;
	}
        OV8865_write_cmos_sensor(0x7010,0x00);
	if (flag == 0x00) {
		return 0;
	} else if (flag & 0x02) {
		return 1;
	} else {
		return 2;
	}
}
static int read_otp_module_id_info(int index, struct otp_struct *otp_ptr)
{
	int i;
	int start_addr = 0, end_addr = 0;
        int module_id=0;;

	if (index == 1) {
		start_addr = 0x7011;
		end_addr = 0x7015;
	}
	else if (index == 2) {
		start_addr = 0x7016;
		end_addr = 0x701a;
	}
	else if (index == 3) {
		start_addr = 0x701b;
		end_addr = 0x701f;
	}
	OV8865_write_cmos_sensor(0x3d84,0xC0);
	OV8865_write_cmos_sensor(0x3d88,((start_addr >> 8) & 0xff));

	OV8865_write_cmos_sensor(0x3d89,(start_addr & 0xff));

	OV8865_write_cmos_sensor(0x3d8A,(end_addr >> 8) & 0xff);

	OV8865_write_cmos_sensor(0x3d8B,(end_addr & 0xff));

	OV8865_write_cmos_sensor(0x3d81,0x01);
        msleep(5);
	(*otp_ptr).module_integrator_id=OV8865_read_cmos_sensor(start_addr);
       if((*otp_ptr).module_integrator_id !=0)
         {module_id=(*otp_ptr).module_integrator_id;
          }
	// clear otp buffer
	for (i = start_addr; i <= end_addr; i++) {
	         OV8865_write_cmos_sensor(i,0x00);
             }
    LOG_INF("gaoatao ov8865 module_id  = 0x%x ", module_id);  //add by gaoatao@yulong.com for test 2015-5-25 11:30:13
	return module_id;
}
static int slect_Typical_index()
 {  int i;
    int otp_index;
    int temp;
    int index;
          OV8865_write_cmos_sensor(0x0103, 0x01);
     mdelay(50);
    OV8865_write_cmos_sensor(0x0100, 0x01);
     for (i = 1;i <= 3; i++) {
		temp = check_otp_info(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i > 3) {
		return 1;
	}
	index=read_otp_module_id_info(otp_index, &current_otp);
       return index;
   }
static int check_otp_wb(int index)
{
int flag;
OV8865_write_cmos_sensor(0x3d84, 0xC0);
//partial mode OTP write start address
OV8865_write_cmos_sensor(0x3d88, 0x70);
OV8865_write_cmos_sensor(0x3d89, 0x20);
// partial mode OTP write end address
OV8865_write_cmos_sensor(0x3d8A, 0x70);
OV8865_write_cmos_sensor(0x3d8B, 0x20);
// read otp into buffer
OV8865_write_cmos_sensor(0x3d81, 0x01);
mdelay(5);
//select group
flag = OV8865_read_cmos_sensor(0x7020);
if (index == 1)
{
flag = (flag>>6) & 0x03;
}
else if (index == 2)
{
flag = (flag>>4) & 0x03;
}
else if (index == 3)
{
flag = (flag>>2) & 0x03;
}
// clear otp buffer
OV8865_write_cmos_sensor( 0x7020, 0x00);
if (flag == 0x00) {
return 0;
}
else if (flag & 0x02) {
return 1;
}
else {
return 2;
}
}

// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int check_otp_lenc(int index)
{
int flag;
OV8865_write_cmos_sensor(0x3d84, 0xC0);
//partial mode OTP write start address
OV8865_write_cmos_sensor(0x3d88, 0x70);
OV8865_write_cmos_sensor(0x3d89, 0x3A);
// partial mode OTP write end address
OV8865_write_cmos_sensor(0x3d8A, 0x70);
OV8865_write_cmos_sensor(0x3d8B, 0x3A);
// read otp into buffer
OV8865_write_cmos_sensor(0x3d81, 0x01);
mdelay(5);
flag = OV8865_read_cmos_sensor(0x703a);
if (index == 1)
{
flag = (flag>>6) & 0x03;
}
else if (index == 2)
{
flag = (flag>>4) & 0x03;
}
else if (index == 3)
{
flag = (flag>> 2)& 0x03;
}
// clear otp buffer
OV8865_write_cmos_sensor( 0x703a, 0x00);
if (flag == 0x00) {
return 0;
}
else if (flag & 0x02) {
return 1;
}
else {
return 2;
}
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 0,
static int read_otp_wb(int index, struct otp_struct *otp_ptr)
{
int i;
int temp;
int start_addr, end_addr;
if (index == 1) {
start_addr = 0x7021;
end_addr = 0x7025;
}
else if (index == 2) {
start_addr = 0x7026;
end_addr = 0x702a;
}
else if (index == 3) {
start_addr = 0x702b;
end_addr = 0x702f;
}
OV8865_write_cmos_sensor(0x3d84, 0xC0);
//partial mode OTP write start address
OV8865_write_cmos_sensor(0x3d88, (start_addr >> 8) & 0xff);
OV8865_write_cmos_sensor(0x3d89, start_addr & 0xff);
// partial mode OTP write end address
OV8865_write_cmos_sensor(0x3d8A, (end_addr >> 8) & 0xff);
OV8865_write_cmos_sensor(0x3d8B, end_addr & 0xff);
// read otp into buffer
OV8865_write_cmos_sensor(0x3d81, 0x01);
mdelay(5);
temp = OV8865_read_cmos_sensor(start_addr + 4);
(*otp_ptr).rg_ratio = (OV8865_read_cmos_sensor(start_addr)<<2) + ((temp>>6) & 0x03);
(*otp_ptr).bg_ratio = (OV8865_read_cmos_sensor(start_addr + 1)<<2) + ((temp>>4) & 0x03);
(*otp_ptr).light_rg = (OV8865_read_cmos_sensor(start_addr + 2) <<2) + ((temp>>2) & 0x03);
(*otp_ptr).light_bg = (OV8865_read_cmos_sensor(start_addr + 3)<<2) + (temp & 0x03);

LOG_INF("---wwp---rg_ratio = %d, bg_ratio = %d, l_rg = %d, l_bg = %d\r\n", (*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio,(*otp_ptr).light_rg,(*otp_ptr).light_bg);

// clear otp buffer
for (i=start_addr; i<=end_addr; i++) {
OV8865_write_cmos_sensor(i, 0x00);
}
return 0;
}
// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 0,
static int read_otp_lenc(int index, struct otp_struct *otp_ptr)
{
int i;
int start_addr, end_addr;
if (index == 1) {
start_addr = 0x703b;
end_addr = 0x7078;
}
else if (index == 2) {
start_addr = 0x7079;
end_addr = 0x70b6;
}
else if (index == 3) {
start_addr = 0x70b7;
end_addr = 0x70f4;
}
OV8865_write_cmos_sensor(0x3d84, 0xC0);
//partial mode OTP write start address
OV8865_write_cmos_sensor(0x3d88, (start_addr >> 8) & 0xff);
OV8865_write_cmos_sensor(0x3d89, start_addr & 0xff);
// partial mode OTP write end address
OV8865_write_cmos_sensor(0x3d8A, (end_addr >> 8) & 0xff);
OV8865_write_cmos_sensor(0x3d8B, end_addr & 0xff);
// read otp into buffer
OV8865_write_cmos_sensor(0x3d81, 0x01);
mdelay(10);
LOG_INF("---wwp---LSC:\r\n");
for(i=0; i<62; i++) {
(* otp_ptr).lenc[i]=OV8865_read_cmos_sensor((start_addr + i));
if ((i%16) == 0)
{
LOG_INF("\r\n");
}
LOG_INF("0x%x ", (* otp_ptr).lenc[i]);
}
// clear otp buffer
for (i=start_addr; i<=end_addr; i++) {
OV8865_write_cmos_sensor(i, 0x00);
}
return 0;
}
// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
static int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
if (R_gain>0x400) {
OV8865_write_cmos_sensor(0x5018, R_gain>>6);
OV8865_write_cmos_sensor(0x5019, R_gain & 0x003f);
}
if (G_gain>0x400) {
OV8865_write_cmos_sensor(0x501A, G_gain>>6);
OV8865_write_cmos_sensor(0x501B, G_gain & 0x003f);
}
if (B_gain>0x400) {
OV8865_write_cmos_sensor(0x501C, B_gain>>6);
OV8865_write_cmos_sensor(0x501D, B_gain & 0x003f);
}
return 0;
}
// otp_ptr: pointer of otp_struct
static int update_lenc(struct otp_struct * otp_ptr)
{
int i, temp;
temp = OV8865_read_cmos_sensor(0x5000);
temp = 0x80 | temp;
LOG_INF("---wwp------write---------\r\n");

for(i=0;i<62;i++) {
OV8865_write_cmos_sensor(0x5800 + i, (*otp_ptr).lenc[i]);
if ((i%16) == 0)
{
LOG_INF("\r\n");
}
LOG_INF("0x%x ", (* otp_ptr).lenc[i]);

}
OV8865_write_cmos_sensor(0x5000, temp);
return 0;
}
// call this function after OV8865 initialization
// return value: 0 update success
// 1, no OTP
static int update_otp_wb()
{
struct otp_struct current_otp;
int i;
int otp_index;
int temp;
int rg,bg;
int R_gain, G_gain, B_gain;
// R/G and B/G of current camera module is read out from sensor OTP
// check first OTP with valid data
for(i=1;i<=3;i++) {
temp = check_otp_wb(i);
LOG_INF("----------wwp---------Func = %s, Line = %d, index = %d, return_valid = %d\r\n", __FUNCTION__, __LINE__, i, temp);
if (temp == 2) {
otp_index = i;
break;
}
}
if (i>3) {
// no valid wb OTP data
return 1;
}
read_otp_wb(otp_index, &current_otp);

/*
if(current_otp.light_rg==0) {
// no light source information in OTP, light factor = 1
rg = current_otp.rg_ratio;
LOG_INF("----------wwp---------Func = %s, Line = %d, rg = %d\r\n", __FUNCTION__, __LINE__, rg);
}
else {
rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;
LOG_INF("----------wwp---------Func = %s, Line = %d, rg = %d\r\n", __FUNCTION__, __LINE__, rg);
}
if(current_otp.light_bg==0) {
bg = current_otp.bg_ratio;
LOG_INF("----------wwp---------Func = %s, Line = %d, bg = %d\r\n", __FUNCTION__, __LINE__, bg);
}
else {
bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;
LOG_INF("----------wwp---------Func = %s, Line = %d, bg = %d\r\n", __FUNCTION__, __LINE__, bg);
}
*/

rg = current_otp.rg_ratio;
bg = current_otp.bg_ratio;

//calculate G gain
int nR_G_gain, nB_G_gain, nG_G_gain;
int nBase_gain;
nR_G_gain = (RG_Ratio_Typical_8865*1000) / rg;
nB_G_gain = (BG_Ratio_Typical_8865*1000) / bg;
nG_G_gain = 1000;
LOG_INF("----------wwp---------Func = %s, Line = %d, RG_Ratio_Typical_8865 = %d\r\n", __FUNCTION__, __LINE__, RG_Ratio_Typical_8865);
if (nR_G_gain < 1000 || nB_G_gain < 1000)
{
if (nR_G_gain < nB_G_gain)
nBase_gain = nR_G_gain;
else
nBase_gain = nB_G_gain;
}
else
{
nBase_gain = nG_G_gain;
}
LOG_INF("----------wwp---------Func = %s, Line = %d, nBase_gain = %d\r\n", __FUNCTION__, __LINE__, nBase_gain);
R_gain = 0x400 * nR_G_gain / (nBase_gain);
B_gain = 0x400 * nB_G_gain / (nBase_gain);
G_gain = 0x400 * nG_G_gain / (nBase_gain);
LOG_INF("----------wwp---------Func = %s, Line = %d, R_gain = %d, B_gain = %d, G_gain = %d\r\n", __FUNCTION__, __LINE__, R_gain, B_gain, G_gain);
update_awb_gain(R_gain, G_gain, B_gain);
return 0;
}
// call this function after OV8865 initialization
// return value: 0 update success
// 1, no OTP
static int update_otp_lenc()
{
struct otp_struct current_otp;
int i;
int otp_index;
int temp;
// check first lens correction OTP with valid data
for(i=1;i<=3;i++) {
temp = check_otp_lenc(i);
LOG_INF("---wwp---Func = %s, Line = %d, index = %d, return_valid = %d\r\n", __FUNCTION__, __LINE__, i, temp);
if (temp == 2) {
otp_index = i;
break;
}
}
if (i>3) {
// no valid WB OTP data
return 1;
}
read_otp_lenc(otp_index, &current_otp);
update_lenc(&current_otp);
// success
return 0;
}
//OTP ADD END
static void set_dummy()
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	write_cmos_sensor(0x380c, imgsensor.line_length >> 8);
	write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);

}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable? \n", framerate,min_framelength_en);

   frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	//dummy_line = frame_length - imgsensor.min_frame_length;
	//if (dummy_line < 0)
		//imgsensor.dummy_line = 0;
	//else
		//imgsensor.dummy_line = dummy_line;
	//imgsensor.frame_length = frame_length + imgsensor.dummy_line;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static void set_shutter(kal_uint16 shutter)
{
    unsigned long flags;
    kal_uint16 realtime_fps = 0;
    kal_uint32 frame_length = 0;
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);


    spin_lock(&imgsensor_drv_lock);
    if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
        imgsensor.frame_length = shutter + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);
    shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
    shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296,0);
        else if(realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146,0);
        else {
        // Extend frame length
        write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
        write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
        }
    } else {
        // Extend frame length
        write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
        write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
    }

    // Update Shutter
    write_cmos_sensor(0x3502, (shutter << 4) & 0xFF);
    write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF);
    write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
    LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}


static kal_uint16 gain2reg(const kal_uint16 gain)
{

	kal_uint16 reg_gain = 0x0000;
	kal_uint16 OV8865_GAIN_BASE = 128;


	reg_gain = gain*OV8865_GAIN_BASE/BASEGAIN;
	return (kal_uint16)reg_gain;


}



/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;


	if (gain < BASEGAIN || gain > 32 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 32 * BASEGAIN)
			gain = 32 * BASEGAIN;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x3508,(reg_gain>>8));
	write_cmos_sensor(0x3509,(reg_gain&0xff));

	return gain;
}	/*	set_gain  */


static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
  //ov8865  Don't support

#if 0 //control demo
  LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
  if (imgsensor.ihdr_en) {

	  spin_lock(&imgsensor_drv_lock);
	  if (le > imgsensor.min_frame_length - imgsensor_info.margin)
		  imgsensor.frame_length = le + imgsensor_info.margin;
	  else
		  imgsensor.frame_length = imgsensor.min_frame_length;

	  if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		  imgsensor.frame_length = imgsensor_info.max_frame_length;
	  spin_unlock(&imgsensor_drv_lock);

	  if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
	  if (se < imgsensor_info.min_shutter) se = imgsensor_info.min_shutter;

	  // Extend frame length first
	  write_cmos_sensor(xxxx, imgsensor.frame_length >> 8);
	  write_cmos_sensor(xxxx, imgsensor.frame_length & 0xFF);

	//write le & se
	  write_cmos_sensor(xxxx, (le >> 8);
	  write_cmos_sensor(xxxx, (le & 0xFF);

	  write_cmos_sensor(xxxx, (se >> 8);
	  write_cmos_sensor(xxxx, (se & 0xFF);

	  set_gain(gain);
#endif
}


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{

	kal_int16 mirror=0,flip=0;

	LOG_INF("image_mirror = %d\n", image_mirror);

	mirror= OV8865_read_cmos_sensor(0x3820);
	flip  = OV8865_read_cmos_sensor(0x3821);

    switch (imgMirror)
    {
        case IMAGE_H_MIRROR://IMAGE_NORMAL:
            write_cmos_sensor(0x3820, (mirror & (0xF9)));//Set normal
            write_cmos_sensor(0x3821, (flip & (0xF9)));	//Set normal
            break;
        case IMAGE_NORMAL://IMAGE_V_MIRROR:
            write_cmos_sensor(0x3820, (mirror & (0xF9)));//Set flip
            write_cmos_sensor(0x3821, (flip | (0x06)));	//Set flip
            break;
        case IMAGE_HV_MIRROR://IMAGE_H_MIRROR:
            write_cmos_sensor(0x3820, (mirror |(0x06)));	//Set mirror
            write_cmos_sensor(0x3821, (flip & (0xF9)));	//Set mirror
            break;
        case IMAGE_V_MIRROR://IMAGE_HV_MIRROR:
            write_cmos_sensor(0x3820, (mirror |(0x06)));	//Set mirror & flip
            write_cmos_sensor(0x3821, (flip |(0x06)));	//Set mirror & flip
            break;
    }

}
#endif

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	/*	night_mode	*/


static void sensor_init(void)
{
	LOG_INF("E\n");

	write_cmos_sensor(0x0103,0x01);// ; software reset
	mdelay(10);
	write_cmos_sensor(0x0100,0x00);// ; software standby
	write_cmos_sensor(0x0100,0x00);//
	write_cmos_sensor(0x0100,0x00);//
	write_cmos_sensor(0x0100,0x00);//
	write_cmos_sensor(0x3638,0xff);// ; analog control

	write_cmos_sensor(0x0302,0x1f);// ; PLL

	write_cmos_sensor(0x0303,0x00);// ; PLL
	write_cmos_sensor(0x0304,0x03);// ; PLL
	write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
	write_cmos_sensor(0x030e,0x00);// ; PLL
	write_cmos_sensor(0x030f,0x09);// ; PLL
	write_cmos_sensor(0x0312,0x01);// ; PLL
	write_cmos_sensor(0x031e,0x0c);// ; PLL
	write_cmos_sensor(0x3015,0x01);// ; clock Div
	write_cmos_sensor(0x3018,0x72);// ; MIPI 4 lane
	write_cmos_sensor(0x3020,0x93);// ; clock normal, pclk/1
	write_cmos_sensor(0x3022,0x01);// ; pd_mini enable when rst_sync
	write_cmos_sensor(0x3031,0x0a);// ; 10-bit
	write_cmos_sensor(0x3106,0x01);// ; PLL
	write_cmos_sensor(0x3305,0xf1);//
	write_cmos_sensor(0x3308,0x00);//
	write_cmos_sensor(0x3309,0x28);//
	write_cmos_sensor(0x330a,0x00);//
	write_cmos_sensor(0x330b,0x20);//
	write_cmos_sensor(0x330c,0x00);//
	write_cmos_sensor(0x330d,0x00);//
	write_cmos_sensor(0x330e,0x00);//
	write_cmos_sensor(0x330f,0x40);//
	write_cmos_sensor(0x3307,0x04);//
	write_cmos_sensor(0x3604,0x04);// ; analog control
	write_cmos_sensor(0x3602,0x30);//
	write_cmos_sensor(0x3605,0x00);//
	write_cmos_sensor(0x3607,0x20);//
	write_cmos_sensor(0x3608,0x11);//
	write_cmos_sensor(0x3609,0x68);//
	write_cmos_sensor(0x360a,0x40);//
	write_cmos_sensor(0x360c,0xdd);//
	write_cmos_sensor(0x360e,0x0c);//
	write_cmos_sensor(0x3610,0x07);//

	write_cmos_sensor(0x3612,0x86);//AM24  power save ov_Revise
	write_cmos_sensor(0x3613,0x58);//AM24
	write_cmos_sensor(0x3614,0x28);//
	write_cmos_sensor(0x3617,0x40);//
	write_cmos_sensor(0x3618,0x5a);//
	write_cmos_sensor(0x3619,0x9b);//
	write_cmos_sensor(0x361c,0x00);//
	write_cmos_sensor(0x361d,0x60);//
	write_cmos_sensor(0x3631,0x60);//
	write_cmos_sensor(0x3633,0x10);//
	write_cmos_sensor(0x3634,0x10);//
	write_cmos_sensor(0x3635,0x10);//
	write_cmos_sensor(0x3636,0x10);//AM24
	write_cmos_sensor(0x3641,0x55);// ; MIPI settings
	write_cmos_sensor(0x3646,0x86);// ; MIPI settings
	write_cmos_sensor(0x3647,0x27);// ; MIPI settings
	write_cmos_sensor(0x364a,0x1b);// ; MIPI settings


	//write_cmos_sensor(0x3500,0x00);// ; exposurre HH
	//write_cmos_sensor(0x3501,0x4c);// ; expouere H
	//write_cmos_sensor(0x3502,0x00);// ; exposure L


	write_cmos_sensor(0x3503,0x00);// ; gain no delay, exposure no delay

	write_cmos_sensor(0x3508,0x02);// ; gain H
	write_cmos_sensor(0x3509,0x00);// ; gain L

	write_cmos_sensor(0x3700,0x24);// ; sensor control AM24
	write_cmos_sensor(0x3701,0x0c);//
	write_cmos_sensor(0x3702,0x28);//
	write_cmos_sensor(0x3703,0x19);//
	write_cmos_sensor(0x3704,0x14);//
	write_cmos_sensor(0x3705,0x00);//
	write_cmos_sensor(0x3706,0x38);//AM24
	write_cmos_sensor(0x3707,0x04);//
	write_cmos_sensor(0x3708,0x24);//
	write_cmos_sensor(0x3709,0x40);//
	write_cmos_sensor(0x370a,0x00);//
	write_cmos_sensor(0x370b,0xb8);//AM24
	write_cmos_sensor(0x370c,0x04);//
	write_cmos_sensor(0x3718,0x12);//
	write_cmos_sensor(0x3719,0x31);//
	write_cmos_sensor(0x3712,0x42);//
	write_cmos_sensor(0x3714,0x12);//
	write_cmos_sensor(0x371e,0x19);//
	write_cmos_sensor(0x371f,0x40);//
	write_cmos_sensor(0x3720,0x05);//
	write_cmos_sensor(0x3721,0x05);//
	write_cmos_sensor(0x3724,0x02);//
	write_cmos_sensor(0x3725,0x02);//
	write_cmos_sensor(0x3726,0x06);//
	write_cmos_sensor(0x3728,0x05);//
	write_cmos_sensor(0x3729,0x02);//
	write_cmos_sensor(0x372a,0x03);//
	write_cmos_sensor(0x372b,0x53);//
	write_cmos_sensor(0x372c,0xa3);//
	write_cmos_sensor(0x372d,0x53);//
	write_cmos_sensor(0x372e,0x06);//
	write_cmos_sensor(0x372f,0x10);//
	write_cmos_sensor(0x3730,0x01);//
	write_cmos_sensor(0x3731,0x06);//
	write_cmos_sensor(0x3732,0x14);//
	write_cmos_sensor(0x3733,0x10);//
	write_cmos_sensor(0x3734,0x40);//
	write_cmos_sensor(0x3736,0x20);//
	write_cmos_sensor(0x373a,0x02);//
	write_cmos_sensor(0x373b,0x0c);//
	write_cmos_sensor(0x373c,0x0a);//
	write_cmos_sensor(0x373e,0x03);//
	write_cmos_sensor(0x3755,0x40);//
	write_cmos_sensor(0x3758,0x00);//
	write_cmos_sensor(0x3759,0x4c);//
	write_cmos_sensor(0x375a,0x06);//
	write_cmos_sensor(0x375b,0x13);//
	write_cmos_sensor(0x375c,0x40);//am26
	write_cmos_sensor(0x375d,0x02);//
	write_cmos_sensor(0x375e,0x00);//
	write_cmos_sensor(0x375f,0x14);//
	write_cmos_sensor(0x3767,0x1c);//am26
	write_cmos_sensor(0x3768,0x04);//
	write_cmos_sensor(0x3769,0x20);//
	write_cmos_sensor(0x376c,0xc0);//am26
	write_cmos_sensor(0x376d,0xc0);//am26
	write_cmos_sensor(0x376a,0x08);//
	write_cmos_sensor(0x3761,0x00);//
	write_cmos_sensor(0x3762,0x00);//
	write_cmos_sensor(0x3763,0x00);//
	write_cmos_sensor(0x3766,0xff);//
	write_cmos_sensor(0x376b,0x42);//
	write_cmos_sensor(0x3772,0x23);//
	write_cmos_sensor(0x3773,0x02);//
	write_cmos_sensor(0x3774,0x16);//
	write_cmos_sensor(0x3775,0x12);//
	write_cmos_sensor(0x3776,0x08);//
	write_cmos_sensor(0x37a0,0x44);//
	write_cmos_sensor(0x37a1,0x3d);//
	write_cmos_sensor(0x37a2,0x3d);//
	write_cmos_sensor(0x37a3,0x01);//
	write_cmos_sensor(0x37a4,0x00);//
	write_cmos_sensor(0x37a5,0x08);//
	write_cmos_sensor(0x37a6,0x00);//
	write_cmos_sensor(0x37a7,0x44);//
	write_cmos_sensor(0x37a8,0x58);//AM24
	write_cmos_sensor(0x37a9,0x58);//AM24
	write_cmos_sensor(0x3760,0x00);//
	write_cmos_sensor(0x376f,0x01);//
	write_cmos_sensor(0x37aa,0x44);//
	write_cmos_sensor(0x37ab,0x2e);//
	write_cmos_sensor(0x37ac,0x2e);//
	write_cmos_sensor(0x37ad,0x33);//
	write_cmos_sensor(0x37ae,0x0d);//
	write_cmos_sensor(0x37af,0x0d);//
	write_cmos_sensor(0x37b0,0x00);//
	write_cmos_sensor(0x37b1,0x00);//
	write_cmos_sensor(0x37b2,0x00);//
	write_cmos_sensor(0x37b3,0x42);//
	write_cmos_sensor(0x37b4,0x42);//
	write_cmos_sensor(0x37b5,0x33);//
	write_cmos_sensor(0x37b6,0x00);//
	write_cmos_sensor(0x37b7,0x00);//
	write_cmos_sensor(0x37b8,0x00);//
	write_cmos_sensor(0x37b9,0xff);// ; sensor control

	write_cmos_sensor(0x3800,0x00);// ; X start H
	write_cmos_sensor(0x3801,0x0c);// ; X start L

	write_cmos_sensor(0x3802,0x00);// ; Y start H
	write_cmos_sensor(0x3803,0x0c);// ; Y start L

	write_cmos_sensor(0x3804,0x0c);// ; X end H
	write_cmos_sensor(0x3805,0xd3);// ; X end L
	write_cmos_sensor(0x3806,0x09);// ; Y end H
	write_cmos_sensor(0x3807,0xa3);// ; Y end L

	write_cmos_sensor(0x3808,0x06);// ; X output size H
	write_cmos_sensor(0x3809,0x60);// ; X output size L
	write_cmos_sensor(0x380a,0x04);// ; Y output size H
	write_cmos_sensor(0x380b,0xc8);// ; Y output size L

	write_cmos_sensor(0x380c,0x07);// ; HTS H
	write_cmos_sensor(0x380d,0x83);// ; HTS L
	write_cmos_sensor(0x380e,0x04);// ; VTS H
	write_cmos_sensor(0x380f,0xe0);// ; VTS L

	write_cmos_sensor(0x3810,0x00);// ; ISP X win H
	write_cmos_sensor(0x3811,0x04);// ; ISP X win L
	write_cmos_sensor(0x3813,0x04);// ; ISP Y win L
	write_cmos_sensor(0x3814,0x03);// ; X inc odd
	write_cmos_sensor(0x3815,0x01);// ; X inc even
	write_cmos_sensor(0x3820,0x00);// ; flip off
	write_cmos_sensor(0x3821,0x67);// ; hsync_en_o, fst_vbin, mirror on
	write_cmos_sensor(0x382a,0x03);// ; Y inc odd
	write_cmos_sensor(0x382b,0x01);// ; Y inc even
	write_cmos_sensor(0x3830,0x08);// ; ablc_use_num[5:1]
	write_cmos_sensor(0x3836,0x02);// ; zline_use_num[5:1]
	write_cmos_sensor(0x3837,0x18);// ; vts_add_dis, cexp_gt_vts_offs=8
	write_cmos_sensor(0x3841,0xff);// ; auto size
	write_cmos_sensor(0x3846,0x88);// ; Y/X boundary pixel numbber for auto size mode


	//3d85/3d8c/3d8d  for otp auto load at power on
	write_cmos_sensor(0x3d85,0x06);//AM24a
	write_cmos_sensor(0x3d8c,0x75);//AM24a
	write_cmos_sensor(0x3d8d,0xef);//AM24a


	write_cmos_sensor(0x3f08,0x0b);//

	write_cmos_sensor(0x4000,0xf1);// ; our range trig en, format chg en, gan chg en, exp chg en, median en
	write_cmos_sensor(0x4001,0x14);// ; left 32 column, final BLC offset limitation enable
	write_cmos_sensor(0x4005,0x10);// ; BLC target

	write_cmos_sensor(0x4006,0x04);// ;revise for ZSD ON/OFF unstable,MTK
	write_cmos_sensor(0x4007,0x04);// ;

	write_cmos_sensor(0x400b,0x0c);// ; start line =0, offset limitation en, cut range function en
	write_cmos_sensor(0x400d,0x10);// ; offset trigger threshold
	write_cmos_sensor(0x401b,0x00);//
	write_cmos_sensor(0x401d,0x00);//
	write_cmos_sensor(0x4020,0x01);// ; anchor left start H
	write_cmos_sensor(0x4021,0x20);// ; anchor left start L
	write_cmos_sensor(0x4022,0x01);// ; anchor left end H
	write_cmos_sensor(0x4023,0x9f);// ; anchor left end L
	write_cmos_sensor(0x4024,0x03);// ; anchor right start H
	write_cmos_sensor(0x4025,0xe0);// ; anchor right start L
	write_cmos_sensor(0x4026,0x04);// ; anchor right end H
	write_cmos_sensor(0x4027,0x5f);// ; anchor right end L
	write_cmos_sensor(0x4028,0x00);// ; top zero line start
	write_cmos_sensor(0x4029,0x02);// ; top zero line number
	write_cmos_sensor(0x402a,0x04);// ; top black line start
	write_cmos_sensor(0x402b,0x04);// ; top black line number
	write_cmos_sensor(0x402c,0x02);// ; bottom zero line start
	write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
	write_cmos_sensor(0x402e,0x08);// ; bottom black line start
	write_cmos_sensor(0x402f,0x02);// ; bottom black line number
	write_cmos_sensor(0x401f,0x00);// ; anchor one disable
	write_cmos_sensor(0x4034,0x3f);// ; limitation BLC offset
	write_cmos_sensor(0x4300,0xff);// ; clip max H
	write_cmos_sensor(0x4301,0x00);// ; clip min H
	write_cmos_sensor(0x4302,0x0f);// ; clip min L/clip max L
	write_cmos_sensor(0x4500,0x40);// ; ADC sync control
	write_cmos_sensor(0x4503,0x10);//
	write_cmos_sensor(0x4601,0x74);// ; V FIFO control
	write_cmos_sensor(0x481f,0x32);// ; clk_prepare_min

	write_cmos_sensor(0x4837,0x16);// ; clock period


	write_cmos_sensor(0x4850,0x10);// ; lane select
	write_cmos_sensor(0x4851,0x32);// ; lane select
	write_cmos_sensor(0x4b00,0x2a);// ; LVDS settings
	write_cmos_sensor(0x4b0d,0x00);// ; LVDS settings
	write_cmos_sensor(0x4d00,0x04);// ; temperature sensor
	write_cmos_sensor(0x4d01,0x18);// ; temperature sensor
	write_cmos_sensor(0x4d02,0xc3);// ; temperature sensor
	write_cmos_sensor(0x4d03,0xff);// ; temperature sensor
	write_cmos_sensor(0x4d04,0xff);// ; temperature sensor
	write_cmos_sensor(0x4d05,0xff);// ; temperature sensor

	write_cmos_sensor(0x5000,0x96);// ; LENC on, MWB on, BPC on, WPC on
	write_cmos_sensor(0x5001,0x01);// ; BLC on

	write_cmos_sensor(0x5002,0x08);// ; vario pixel off
	write_cmos_sensor(0x5901,0x00);//
	write_cmos_sensor(0x5e00,0x00);// ; test pattern off
	write_cmos_sensor(0x5e01,0x41);// ; window cut enable
	write_cmos_sensor(0x0100,0x01);// ; wake up, streaming
	write_cmos_sensor(0x5800,0x1d);// ; lens correction
	write_cmos_sensor(0x5801,0x0e);//
	write_cmos_sensor(0x5802,0x0c);//
	write_cmos_sensor(0x5803,0x0c);//
	write_cmos_sensor(0x5804,0x0f);//
	write_cmos_sensor(0x5805,0x22);//
	write_cmos_sensor(0x5806,0x0a);//
	write_cmos_sensor(0x5807,0x06);//
	write_cmos_sensor(0x5808,0x05);//
	write_cmos_sensor(0x5809,0x05);//
	write_cmos_sensor(0x580a,0x07);//
	write_cmos_sensor(0x580b,0x0a);//
	write_cmos_sensor(0x580c,0x06);//
	write_cmos_sensor(0x580d,0x02);//
	write_cmos_sensor(0x580e,0x00);//
	write_cmos_sensor(0x580f,0x00);//
	write_cmos_sensor(0x5810,0x03);//
	write_cmos_sensor(0x5811,0x07);//
	write_cmos_sensor(0x5812,0x06);//
	write_cmos_sensor(0x5813,0x02);//
	write_cmos_sensor(0x5814,0x00);//
	write_cmos_sensor(0x5815,0x00);//
	write_cmos_sensor(0x5816,0x03);//
	write_cmos_sensor(0x5817,0x07);//
	write_cmos_sensor(0x5818,0x09);//
	write_cmos_sensor(0x5819,0x06);//
	write_cmos_sensor(0x581a,0x04);//
	write_cmos_sensor(0x581b,0x04);//
	write_cmos_sensor(0x581c,0x06);//
	write_cmos_sensor(0x581d,0x0a);//
	write_cmos_sensor(0x581e,0x19);//
	write_cmos_sensor(0x581f,0x0d);//
	write_cmos_sensor(0x5820,0x0b);//
	write_cmos_sensor(0x5821,0x0b);//
	write_cmos_sensor(0x5822,0x0e);//
	write_cmos_sensor(0x5823,0x22);//
	write_cmos_sensor(0x5824,0x23);//
	write_cmos_sensor(0x5825,0x28);//
	write_cmos_sensor(0x5826,0x29);//
	write_cmos_sensor(0x5827,0x27);//
	write_cmos_sensor(0x5828,0x13);//
	write_cmos_sensor(0x5829,0x26);//
	write_cmos_sensor(0x582a,0x33);//
	write_cmos_sensor(0x582b,0x32);//
	write_cmos_sensor(0x582c,0x33);//
	write_cmos_sensor(0x582d,0x16);//
	write_cmos_sensor(0x582e,0x14);//
	write_cmos_sensor(0x582f,0x30);//
	write_cmos_sensor(0x5830,0x31);//
	write_cmos_sensor(0x5831,0x30);//
	write_cmos_sensor(0x5832,0x15);//
	write_cmos_sensor(0x5833,0x26);//
	write_cmos_sensor(0x5834,0x23);//
	write_cmos_sensor(0x5835,0x21);//
	write_cmos_sensor(0x5836,0x23);//
	write_cmos_sensor(0x5837,0x05);//
	write_cmos_sensor(0x5838,0x36);//
	write_cmos_sensor(0x5839,0x27);//
	write_cmos_sensor(0x583a,0x28);//
	write_cmos_sensor(0x583b,0x26);//
	write_cmos_sensor(0x583c,0x24);//
	write_cmos_sensor(0x583d,0xdf);// ; lens correction

	//add 5b00~5b05 for OTP DPC control registers update-ov
	write_cmos_sensor(0x5b00,0x02);//
	write_cmos_sensor(0x5b01,0xd0);//
	write_cmos_sensor(0x5b02,0x03);//
	write_cmos_sensor(0x5b03,0xff);//
	write_cmos_sensor(0x5b05,0x6c);//
	//write_cmos_sensor(0x4800,0x5c);// ; mipi gate:lens start/end

}

	/*	sensor_init  */


static void preview_setting(void)
{
	LOG_INF("Enter\n");

	write_cmos_sensor(0x0100,0x00);//; software standby

	write_cmos_sensor(0x0302,0x1f);// ; PLL
	write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
	write_cmos_sensor(0x030f,0x09);//; PLL
	write_cmos_sensor(0x3018,0x72);//
	write_cmos_sensor(0x3106,0x01);//

	//write_cmos_sensor(0x3501,0x28);//; expouere H
	//write_cmos_sensor(0x3502,0x90);//; exposure L

	write_cmos_sensor(0x3700,0x24);//; sensor control  AM24
	write_cmos_sensor(0x3701,0x0c);//
	write_cmos_sensor(0x3702,0x28);//
	write_cmos_sensor(0x3703,0x19);//
	write_cmos_sensor(0x3704,0x14);//
	write_cmos_sensor(0x3706,0x38);//AM24
	write_cmos_sensor(0x3707,0x04);//
	write_cmos_sensor(0x3708,0x24);//
	write_cmos_sensor(0x3709,0x40);//
	write_cmos_sensor(0x370a,0x00);//
	write_cmos_sensor(0x370b,0xb8);//AM24
	write_cmos_sensor(0x370c,0x04);//
	write_cmos_sensor(0x3718,0x12);//
	write_cmos_sensor(0x3712,0x42);//
	write_cmos_sensor(0x371e,0x19);//
	write_cmos_sensor(0x371f,0x40);//
	write_cmos_sensor(0x3720,0x05);//
	write_cmos_sensor(0x3721,0x05);//
	write_cmos_sensor(0x3724,0x02);//
	write_cmos_sensor(0x3725,0x02);//
	write_cmos_sensor(0x3726,0x06);//
	write_cmos_sensor(0x3728,0x05);//
	write_cmos_sensor(0x3729,0x02);//
	write_cmos_sensor(0x372a,0x03);//
	write_cmos_sensor(0x372b,0x53);//
	write_cmos_sensor(0x372c,0xa3);//
	write_cmos_sensor(0x372d,0x53);//
	write_cmos_sensor(0x372e,0x06);//
	write_cmos_sensor(0x372f,0x10);//
	write_cmos_sensor(0x3730,0x01);//
	write_cmos_sensor(0x3731,0x06);//
	write_cmos_sensor(0x3732,0x14);//
	write_cmos_sensor(0x3736,0x20);//
	write_cmos_sensor(0x373a,0x02);//
	write_cmos_sensor(0x373b,0x0c);//
	write_cmos_sensor(0x373c,0x0a);//
	write_cmos_sensor(0x373e,0x03);//
	write_cmos_sensor(0x375a,0x06);//
	write_cmos_sensor(0x375b,0x13);//
	write_cmos_sensor(0x375c,0x40);// am26
	write_cmos_sensor(0x375d,0x02);//
	write_cmos_sensor(0x375f,0x14);//
	write_cmos_sensor(0x3767,0x1c);//am26
	write_cmos_sensor(0x3769,0x20);//
	write_cmos_sensor(0x376c,0xc0);//am26
	write_cmos_sensor(0x376d,0xc0);//am26
	write_cmos_sensor(0x3772,0x23);//
	write_cmos_sensor(0x3773,0x02);//
	write_cmos_sensor(0x3774,0x16);//
	write_cmos_sensor(0x3775,0x12);//
	write_cmos_sensor(0x3776,0x08);//
	write_cmos_sensor(0x37a0,0x44);//
	write_cmos_sensor(0x37a1,0x3d);//
	write_cmos_sensor(0x37a2,0x3d);//
	write_cmos_sensor(0x37a3,0x01);//
	write_cmos_sensor(0x37a5,0x08);//
	write_cmos_sensor(0x37a7,0x44);//
	write_cmos_sensor(0x37a8,0x58);//AM24
	write_cmos_sensor(0x37a9,0x58);//AM24
	write_cmos_sensor(0x37aa,0x44);//
	write_cmos_sensor(0x37ab,0x2e);//
	write_cmos_sensor(0x37ac,0x2e);//
	write_cmos_sensor(0x37ad,0x33);//
	write_cmos_sensor(0x37ae,0x0d);//
	write_cmos_sensor(0x37af,0x0d);//
	write_cmos_sensor(0x37b3,0x42);//
	write_cmos_sensor(0x37b4,0x42);//
	write_cmos_sensor(0x37b5,0x33);//

	write_cmos_sensor(0x3808,0x06);//; X output size H	active
	write_cmos_sensor(0x3809,0x60);//; X output size L  (0x3809,0x60)
	write_cmos_sensor(0x380a,0x04);//; Y output size H	active
	write_cmos_sensor(0x380b,0xc8);//; Y output size L (0x380b,0xc8)

	write_cmos_sensor(0x380c,((imgsensor_info.pre.linelength >> 8) & 0xFF));//; HTS H
	write_cmos_sensor(0x380d,(imgsensor_info.pre.linelength & 0xFF));//; HTS L
	write_cmos_sensor(0x380e,((imgsensor_info.pre.framelength >> 8) & 0xFF));//; VTS H
	write_cmos_sensor(0x380f,(imgsensor_info.pre.framelength & 0xFF));//; VTS L

	write_cmos_sensor(0x3813,0x04);//; ISP Y win L
	write_cmos_sensor(0x3814,0x03);//; X inc odd
	write_cmos_sensor(0x3821,0x67);//; hsync_en_o, fst_vbin, mirror on
	write_cmos_sensor(0x382a,0x03);//; Y inc odd
	write_cmos_sensor(0x382b,0x01);//; Y inc even
	write_cmos_sensor(0x3830,0x08);//; ablc_use_num[5:1]
	write_cmos_sensor(0x3836,0x02);//; zline_use_num[5:1]
	write_cmos_sensor(0x3846,0x88);//; Y/X boundary pixel numbber for auto size mode
	write_cmos_sensor(0x3f08,0x0b);//

	write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en

	write_cmos_sensor(0x4001,0x14);//; left 32 column, final BLC offset limitation enable
	write_cmos_sensor(0x4020,0x01);//; anchor left start H
	write_cmos_sensor(0x4021,0x20);//; anchor left start L
	write_cmos_sensor(0x4022,0x01);//; anchor left end H
	write_cmos_sensor(0x4023,0x9f);//; anchor left end L
	write_cmos_sensor(0x4024,0x03);//; anchor right start H
	write_cmos_sensor(0x4025,0xe0);//; anchor right start L
	write_cmos_sensor(0x4026,0x04);//; anchor right end H
	write_cmos_sensor(0x4027,0x5f);//; anchor right end L
	write_cmos_sensor(0x402a,0x04);//; top black line start
	write_cmos_sensor(0x402b,0x04);//am26
	write_cmos_sensor(0x402c,0x02);//; bottom zero line start
	write_cmos_sensor(0x402d,0x02);//; bottom zero line number
	write_cmos_sensor(0x402e,0x08);//; bottom black line start
	write_cmos_sensor(0x4500,0x40);//; ADC sync control
	write_cmos_sensor(0x4601,0x74);//; V FIFO control

	write_cmos_sensor(0x4837,0x16);// ; clock period  hkm test mark

	write_cmos_sensor(0x5002,0x08);//; vario pixel off
	write_cmos_sensor(0x5901,0x00);//

	//mdelay(100);
	write_cmos_sensor(0x0100,0x01);//; wake up, streaming

}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("Enter! currefps:%d\n",currefps);

	if(currefps == 150)
	{
		write_cmos_sensor(0x0100,0x00);

		write_cmos_sensor(0x0302,0x19);//
		write_cmos_sensor(0x030d,0x1e);//
		write_cmos_sensor(0x030f,0x05);// ; PLL

		write_cmos_sensor(0x3018,0x72);//
		write_cmos_sensor(0x3106,0x01);//

		//write_cmos_sensor(0x3501,0x8f);// ; expouere H
		//write_cmos_sensor(0x3502,0xa0);// ; exposure L

		write_cmos_sensor(0x3700,0x48);// ; sensor control	AM24
		write_cmos_sensor(0x3701,0x18);//
		write_cmos_sensor(0x3702,0x50);//
		write_cmos_sensor(0x3703,0x32);//
		write_cmos_sensor(0x3704,0x28);//
		write_cmos_sensor(0x3706,0x70);//AM24
		write_cmos_sensor(0x3707,0x08);//
		write_cmos_sensor(0x3708,0x48);//
		write_cmos_sensor(0x3709,0x80);//
		write_cmos_sensor(0x370a,0x01);//
		write_cmos_sensor(0x370b,0x70);//AM24
		write_cmos_sensor(0x370c,0x07);//
		write_cmos_sensor(0x3718,0x14);//
		write_cmos_sensor(0x3712,0x44);//
		write_cmos_sensor(0x371e,0x31);//
		write_cmos_sensor(0x371f,0x7f);//
		write_cmos_sensor(0x3720,0x0a);//
		write_cmos_sensor(0x3721,0x0a);//
		write_cmos_sensor(0x3724,0x04);//
		write_cmos_sensor(0x3725,0x04);//
		write_cmos_sensor(0x3726,0x0c);//
		write_cmos_sensor(0x3728,0x0a);//
		write_cmos_sensor(0x3729,0x03);//
		write_cmos_sensor(0x372a,0x06);//
		write_cmos_sensor(0x372b,0xa6);//
		write_cmos_sensor(0x372c,0xa6);//
		write_cmos_sensor(0x372d,0xa6);//
		write_cmos_sensor(0x372e,0x0c);//
		write_cmos_sensor(0x372f,0x20);//
		write_cmos_sensor(0x3730,0x02);//
		write_cmos_sensor(0x3731,0x0c);//
		write_cmos_sensor(0x3732,0x28);//
		write_cmos_sensor(0x3736,0x30);//
		write_cmos_sensor(0x373a,0x04);//
		write_cmos_sensor(0x373b,0x18);//
		write_cmos_sensor(0x373c,0x14);//
		write_cmos_sensor(0x373e,0x06);//
		write_cmos_sensor(0x375a,0x0c);//
		write_cmos_sensor(0x375b,0x26);//
		write_cmos_sensor(0x375c,0x40);//am26
		write_cmos_sensor(0x375d,0x04);//
		write_cmos_sensor(0x375f,0x28);//
		write_cmos_sensor(0x3767,0x1e);//0x04
		write_cmos_sensor(0x3769,0x20);//
		write_cmos_sensor(0x376c,0xc0);//am26
		write_cmos_sensor(0x376d,0xc0);//am26
		write_cmos_sensor(0x3772,0x46);//
		write_cmos_sensor(0x3773,0x04);//
		write_cmos_sensor(0x3774,0x2c);//
		write_cmos_sensor(0x3775,0x13);//
		write_cmos_sensor(0x3776,0x10);//
		write_cmos_sensor(0x37a0,0x88);//
		write_cmos_sensor(0x37a1,0x7a);//
		write_cmos_sensor(0x37a2,0x7a);//
		write_cmos_sensor(0x37a3,0x02);//
		write_cmos_sensor(0x37a5,0x09);//
		write_cmos_sensor(0x37a7,0x88);//
		write_cmos_sensor(0x37a8,0xb0);//AM24
		write_cmos_sensor(0x37a9,0xb0);//AM24
		write_cmos_sensor(0x37aa,0x88);//
		write_cmos_sensor(0x37ab,0x5c);//
		write_cmos_sensor(0x37ac,0x5c);//
		write_cmos_sensor(0x37ad,0x55);//
		write_cmos_sensor(0x37ae,0x19);//
		write_cmos_sensor(0x37af,0x19);//
		write_cmos_sensor(0x37b3,0x84);//
		write_cmos_sensor(0x37b4,0x84);//
		write_cmos_sensor(0x37b5,0x66);//

		write_cmos_sensor(0x3808,0x0c);// ; X output size H
		write_cmos_sensor(0x3809,0xc0);// ; X output size L (0x3809,0xc0)
		write_cmos_sensor(0x380a,0x09);// ; Y output size H
		write_cmos_sensor(0x380b,0x90);// ; Y output size L (0x380b,0x90)

		write_cmos_sensor(0x380c,((imgsensor_info.cap1.linelength >> 8) & 0xFF));// ; HTS H //30fps
		write_cmos_sensor(0x380d,(imgsensor_info.cap1.linelength & 0xFF));// ; HTS L
		write_cmos_sensor(0x380e,((imgsensor_info.cap1.framelength >> 8) & 0xFF));//; VTS H
		write_cmos_sensor(0x380f,(imgsensor_info.cap1.framelength & 0xFF));//; VTS L

		write_cmos_sensor(0x3813,0x02);// ; ISP Y win L
		write_cmos_sensor(0x3814,0x01);// ; X inc odd
		write_cmos_sensor(0x3821,0x46);// ; hsync_en_o, fst_vbin, mirror on
		write_cmos_sensor(0x382a,0x01);// ; Y inc odd
		write_cmos_sensor(0x382b,0x01);// ; Y inc even
		write_cmos_sensor(0x3830,0x04);// ; ablc_use_num[5:1]
		write_cmos_sensor(0x3836,0x01);// ; zline_use_num[5:1]
		write_cmos_sensor(0x3846,0x48);// ; Y/X boundary pixel numbber for auto size mode
		write_cmos_sensor(0x3f08,0x16);//

		write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en

		write_cmos_sensor(0x4001,0x04);// ; left 32 column, final BLC offset limitation enable
		write_cmos_sensor(0x4020,0x02);// ; anchor left start H
		write_cmos_sensor(0x4021,0x40);// ; anchor left start L
		write_cmos_sensor(0x4022,0x03);// ; anchor left end H
		write_cmos_sensor(0x4023,0x3f);// ; anchor left end L
		write_cmos_sensor(0x4024,0x07);// ; anchor right start H
		write_cmos_sensor(0x4025,0xc0);// ; anchor right start L
		write_cmos_sensor(0x4026,0x08);// ; anchor right end H
		write_cmos_sensor(0x4027,0xbf);// ; anchor right end L
		write_cmos_sensor(0x402a,0x04);// ; top black line start
		write_cmos_sensor(0x402b,0x04);//am26
		write_cmos_sensor(0x402c,0x02);// ; bottom zero line start
		write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
		write_cmos_sensor(0x402e,0x08);// ; bottom black line start
		write_cmos_sensor(0x4500,0x68);// ; ADC sync control
		write_cmos_sensor(0x4601,0x10);// ; V FIFO control
		write_cmos_sensor(0x4837,0x1a);// ; clock period
		write_cmos_sensor(0x5002,0x08);// ; vario pixel off
		write_cmos_sensor(0x5901,0x00);//

		write_cmos_sensor(0x0100,0x01);//; wake up, streaming
	}
	else//normal capture
	{
		write_cmos_sensor(0x0100,0x00);

		write_cmos_sensor(0x0302,0x1f);// ; PLL
		write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
		write_cmos_sensor(0x030f,0x04);// ; PLL
		write_cmos_sensor(0x3018,0x72);//
		write_cmos_sensor(0x3106,0x01);//

		//write_cmos_sensor(0x3501,0x8f);// ; expouere H
		//write_cmos_sensor(0x3502,0xa0);// ; exposure L

		write_cmos_sensor(0x3700,0x48);// ; sensor control  AM24
		write_cmos_sensor(0x3701,0x18);//
		write_cmos_sensor(0x3702,0x50);//
		write_cmos_sensor(0x3703,0x32);//
		write_cmos_sensor(0x3704,0x28);//
		write_cmos_sensor(0x3706,0x70);//AM24
		write_cmos_sensor(0x3707,0x08);//
		write_cmos_sensor(0x3708,0x48);//
		write_cmos_sensor(0x3709,0x80);//
		write_cmos_sensor(0x370a,0x01);//
		write_cmos_sensor(0x370b,0x70);//AM24
		write_cmos_sensor(0x370c,0x07);//
		write_cmos_sensor(0x3718,0x14);//
		write_cmos_sensor(0x3712,0x44);//
		write_cmos_sensor(0x371e,0x31);//
		write_cmos_sensor(0x371f,0x7f);//
		write_cmos_sensor(0x3720,0x0a);//
		write_cmos_sensor(0x3721,0x0a);//
		write_cmos_sensor(0x3724,0x04);//
		write_cmos_sensor(0x3725,0x04);//
		write_cmos_sensor(0x3726,0x0c);//
		write_cmos_sensor(0x3728,0x0a);//
		write_cmos_sensor(0x3729,0x03);//
		write_cmos_sensor(0x372a,0x06);//
		write_cmos_sensor(0x372b,0xa6);//
		write_cmos_sensor(0x372c,0xa6);//
		write_cmos_sensor(0x372d,0xa6);//
		write_cmos_sensor(0x372e,0x0c);//
		write_cmos_sensor(0x372f,0x20);//
		write_cmos_sensor(0x3730,0x02);//
		write_cmos_sensor(0x3731,0x0c);//
		write_cmos_sensor(0x3732,0x28);//
		write_cmos_sensor(0x3736,0x30);//
		write_cmos_sensor(0x373a,0x04);//
		write_cmos_sensor(0x373b,0x18);//
		write_cmos_sensor(0x373c,0x14);//
		write_cmos_sensor(0x373e,0x06);//
		write_cmos_sensor(0x375a,0x0c);//
		write_cmos_sensor(0x375b,0x26);//
		write_cmos_sensor(0x375c,0x40);//am26
		write_cmos_sensor(0x375d,0x04);//
		write_cmos_sensor(0x375f,0x28);//
		write_cmos_sensor(0x3767,0x1e);//0x04
		write_cmos_sensor(0x3769,0x20);//
		write_cmos_sensor(0x376c,0xc0);//am26
		write_cmos_sensor(0x376d,0xc0);//am26
		write_cmos_sensor(0x3772,0x46);//
		write_cmos_sensor(0x3773,0x04);//
		write_cmos_sensor(0x3774,0x2c);//
		write_cmos_sensor(0x3775,0x13);//
		write_cmos_sensor(0x3776,0x10);//
		write_cmos_sensor(0x37a0,0x88);//
		write_cmos_sensor(0x37a1,0x7a);//
		write_cmos_sensor(0x37a2,0x7a);//
		write_cmos_sensor(0x37a3,0x02);//
		write_cmos_sensor(0x37a5,0x09);//
		write_cmos_sensor(0x37a7,0x88);//
		write_cmos_sensor(0x37a8,0xb0);//AM24
		write_cmos_sensor(0x37a9,0xb0);//AM24
		write_cmos_sensor(0x37aa,0x88);//
		write_cmos_sensor(0x37ab,0x5c);//
		write_cmos_sensor(0x37ac,0x5c);//
		write_cmos_sensor(0x37ad,0x55);//
		write_cmos_sensor(0x37ae,0x19);//
		write_cmos_sensor(0x37af,0x19);//
		write_cmos_sensor(0x37b3,0x84);//
		write_cmos_sensor(0x37b4,0x84);//
		write_cmos_sensor(0x37b5,0x66);//

		write_cmos_sensor(0x3808,0x0c);// ; X output size H
		write_cmos_sensor(0x3809,0xc0);// ; X output size L (0x3809,0xc0)
		write_cmos_sensor(0x380a,0x09);// ; Y output size H
		write_cmos_sensor(0x380b,0x90);// ; Y output size L (0x380b,0x90)

		write_cmos_sensor(0x380c,((imgsensor_info.cap.linelength >> 8) & 0xFF));// ; HTS H //30fps
		write_cmos_sensor(0x380d,(imgsensor_info.cap.linelength & 0xFF));// ; HTS L
		write_cmos_sensor(0x380e,((imgsensor_info.cap.framelength >> 8) & 0xFF));//; VTS H
		write_cmos_sensor(0x380f,(imgsensor_info.cap.framelength & 0xFF));//; VTS L

		write_cmos_sensor(0x3813,0x02);// ; ISP Y win L
		write_cmos_sensor(0x3814,0x01);// ; X inc odd
		write_cmos_sensor(0x3821,0x46);// ; hsync_en_o, fst_vbin, mirror on
		write_cmos_sensor(0x382a,0x01);// ; Y inc odd
		write_cmos_sensor(0x382b,0x01);// ; Y inc even
		write_cmos_sensor(0x3830,0x04);// ; ablc_use_num[5:1]
		write_cmos_sensor(0x3836,0x01);// ; zline_use_num[5:1]
		write_cmos_sensor(0x3846,0x48);// ; Y/X boundary pixel numbber for auto size mode
		write_cmos_sensor(0x3f08,0x16);//

		write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en

		write_cmos_sensor(0x4001,0x04);// ; left 32 column, final BLC offset limitation enable
		write_cmos_sensor(0x4020,0x02);// ; anchor left start H
		write_cmos_sensor(0x4021,0x40);// ; anchor left start L
		write_cmos_sensor(0x4022,0x03);// ; anchor left end H
		write_cmos_sensor(0x4023,0x3f);// ; anchor left end L
		write_cmos_sensor(0x4024,0x07);// ; anchor right start H
		write_cmos_sensor(0x4025,0xc0);// ; anchor right start L
		write_cmos_sensor(0x4026,0x08);// ; anchor right end H
		write_cmos_sensor(0x4027,0xbf);// ; anchor right end L
		write_cmos_sensor(0x402a,0x04);// ; top black line start
		write_cmos_sensor(0x402b,0x04);//am26
		write_cmos_sensor(0x402c,0x02);// ; bottom zero line start
		write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
		write_cmos_sensor(0x402e,0x08);// ; bottom black line start
		write_cmos_sensor(0x4500,0x68);// ; ADC sync control
		write_cmos_sensor(0x4601,0x10);// ; V FIFO control
		write_cmos_sensor(0x4837,0x16);// ; clock period
		write_cmos_sensor(0x5002,0x08);// ; vario pixel off
		write_cmos_sensor(0x5901,0x00);//

		write_cmos_sensor(0x0100,0x01);//; wake up, streaming
	    }
}


static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("Enter! currefps:%d\n",currefps);

	write_cmos_sensor(0x0100,0x00);

	write_cmos_sensor(0x0302,0x1f);// ; PLL
	write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
	write_cmos_sensor(0x030f,0x04);// ; PLL
	write_cmos_sensor(0x3018,0x72);//
	write_cmos_sensor(0x3106,0x01);//

	//write_cmos_sensor(0x3501,0x8f);//; expouere H
	//write_cmos_sensor(0x3502,0xa0);// ; exposure L

	write_cmos_sensor(0x3700,0x48);// ; sensor control AM24
	write_cmos_sensor(0x3701,0x18);//
	write_cmos_sensor(0x3702,0x50);//
	write_cmos_sensor(0x3703,0x32);//
	write_cmos_sensor(0x3704,0x28);//
	write_cmos_sensor(0x3706,0x70);//AM24
	write_cmos_sensor(0x3707,0x08);//
	write_cmos_sensor(0x3708,0x48);//
	write_cmos_sensor(0x3709,0x80);//
	write_cmos_sensor(0x370a,0x01);//
	write_cmos_sensor(0x370b,0x70);//AM24
	write_cmos_sensor(0x370c,0x07);//
	write_cmos_sensor(0x3718,0x14);//
	write_cmos_sensor(0x3712,0x44);//
	write_cmos_sensor(0x371e,0x31);//
	write_cmos_sensor(0x371f,0x7f);//
	write_cmos_sensor(0x3720,0x0a);//
	write_cmos_sensor(0x3721,0x0a);//
	write_cmos_sensor(0x3724,0x04);//
	write_cmos_sensor(0x3725,0x04);//
	write_cmos_sensor(0x3726,0x0c);//
	write_cmos_sensor(0x3728,0x0a);//
	write_cmos_sensor(0x3729,0x03);//
	write_cmos_sensor(0x372a,0x06);//
	write_cmos_sensor(0x372b,0xa6);//
	write_cmos_sensor(0x372c,0xa6);//
	write_cmos_sensor(0x372d,0xa6);//
	write_cmos_sensor(0x372e,0x0c);//
	write_cmos_sensor(0x372f,0x20);//
	write_cmos_sensor(0x3730,0x02);//
	write_cmos_sensor(0x3731,0x0c);//
	write_cmos_sensor(0x3732,0x28);//
	write_cmos_sensor(0x3736,0x30);//
	write_cmos_sensor(0x373a,0x04);//
	write_cmos_sensor(0x373b,0x18);//
	write_cmos_sensor(0x373c,0x14);//
	write_cmos_sensor(0x373e,0x06);//
	write_cmos_sensor(0x375a,0x0c);//
	write_cmos_sensor(0x375b,0x26);//
	write_cmos_sensor(0x375c,0x40);// am26
	write_cmos_sensor(0x375d,0x04);//
	write_cmos_sensor(0x375f,0x28);//
	write_cmos_sensor(0x3767,0x1e);//
	write_cmos_sensor(0x3769,0x20);//
	write_cmos_sensor(0x376c,0xc0);// am26
	write_cmos_sensor(0x376d,0xc0);// am26
	write_cmos_sensor(0x3772,0x46);//
	write_cmos_sensor(0x3773,0x04);//
	write_cmos_sensor(0x3774,0x2c);//
	write_cmos_sensor(0x3775,0x13);//
	write_cmos_sensor(0x3776,0x10);//
	write_cmos_sensor(0x37a0,0x88);//
	write_cmos_sensor(0x37a1,0x7a);//
	write_cmos_sensor(0x37a2,0x7a);//
	write_cmos_sensor(0x37a3,0x02);//
	write_cmos_sensor(0x37a5,0x09);//
	write_cmos_sensor(0x37a7,0x88);//
	write_cmos_sensor(0x37a8,0xb0);//AM24
	write_cmos_sensor(0x37a9,0xb0);//AM24
	write_cmos_sensor(0x37aa,0x88);//
	write_cmos_sensor(0x37ab,0x5c);//
	write_cmos_sensor(0x37ac,0x5c);//
	write_cmos_sensor(0x37ad,0x55);//
	write_cmos_sensor(0x37ae,0x19);//
	write_cmos_sensor(0x37af,0x19);//
	write_cmos_sensor(0x37b3,0x84);//
	write_cmos_sensor(0x37b4,0x84);//
	write_cmos_sensor(0x37b5,0x66);//

	write_cmos_sensor(0x3808,0x0c);// ; X output size H
	write_cmos_sensor(0x3809,0xc0);// ; X output size L	(0x3809,0xc0)
	write_cmos_sensor(0x380a,0x07);// ; Y output size H
	//write_cmos_sensor(0x380b,0x30);// ; Y output size L  (0x380b,0x2c)
	write_cmos_sensor(0x380b,0x2c);// ; Y output size L	(0x380b,0x2c)

	write_cmos_sensor(0x380c,((imgsensor_info.normal_video.linelength >> 8) & 0xFF));// ; HTS H
	write_cmos_sensor(0x380d,(imgsensor_info.normal_video.linelength & 0xFF));// ; HTS L
	write_cmos_sensor(0x380e,((imgsensor_info.normal_video.framelength >> 8) & 0xFF));// ; VTS H
	write_cmos_sensor(0x380f,(imgsensor_info.normal_video.framelength & 0xFF));// ; VTS L

	write_cmos_sensor(0x3813,0x02);// ; ISP Y win L
	write_cmos_sensor(0x3814,0x01);// ; X inc odd
	write_cmos_sensor(0x3821,0x46);// ; hsync_en_o, fst_vbin, mirror on
	write_cmos_sensor(0x382a,0x01);// ; Y inc odd
	write_cmos_sensor(0x382b,0x01);// ; Y inc even
	write_cmos_sensor(0x3830,0x04);// ; ablc_use_num[5:1]
	write_cmos_sensor(0x3836,0x01);// ; zline_use_num[5:1]
	write_cmos_sensor(0x3846,0x48);// ; Y/X boundary pixel numbber for auto size mode
	write_cmos_sensor(0x3f08,0x16);//

	write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en

	write_cmos_sensor(0x4001,0x04);// ; left 32 column, final BLC offset limitation enable
	write_cmos_sensor(0x4020,0x02);// ; anchor left start H
	write_cmos_sensor(0x4021,0x40);// ; anchor left start L
	write_cmos_sensor(0x4022,0x03);// ; anchor left end H
	write_cmos_sensor(0x4023,0x3f);// ; anchor left end L
	write_cmos_sensor(0x4024,0x07);// ; anchor right start H
	write_cmos_sensor(0x4025,0xc0);// ; anchor right start L
	write_cmos_sensor(0x4026,0x08);// ; anchor right end H
	write_cmos_sensor(0x4027,0xbf);// ; anchor right end L
	write_cmos_sensor(0x402a,0x04);// ; top black line start
	write_cmos_sensor(0x402b,0x04);//am26
	write_cmos_sensor(0x402c,0x02);// ; bottom zero line start
	write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
	write_cmos_sensor(0x402e,0x08);// ; bottom black line start
	write_cmos_sensor(0x4500,0x68);// ; ADC sync control
	write_cmos_sensor(0x4601,0x10);// ; V FIFO control

	write_cmos_sensor(0x4837,0x16);// ; clock period
	write_cmos_sensor(0x5002,0x08);// ; vario pixel off
	write_cmos_sensor(0x5901,0x00);//

	write_cmos_sensor(0x0100,0x01);//; wake up, streaming
}



static void hs_video_setting()
{
	LOG_INF("Enter!\n");

	write_cmos_sensor(0x0100,0x00);// ; software standby

	write_cmos_sensor(0x0302,0x1f);//;
	write_cmos_sensor(0x030d,0x1f);//;
	write_cmos_sensor(0x030f,0x09);//
	write_cmos_sensor(0x3018,0x72);//
	write_cmos_sensor(0x3106,0x01);//

	//write_cmos_sensor(0x3501,0x1e);//
	//write_cmos_sensor(0x3502,0x70);//

	write_cmos_sensor(0x3700,0x24);//
	write_cmos_sensor(0x3701,0x0c);//
	write_cmos_sensor(0x3702,0x28);//
	write_cmos_sensor(0x3703,0x19);//
	write_cmos_sensor(0x3704,0x14);//
	write_cmos_sensor(0x3706,0x38);//
	write_cmos_sensor(0x3707,0x04);//
	write_cmos_sensor(0x3708,0x24);//
	write_cmos_sensor(0x3709,0x40);//
	write_cmos_sensor(0x370a,0x00);//
	write_cmos_sensor(0x370b,0xb8);//
	write_cmos_sensor(0x370c,0x04);//
	write_cmos_sensor(0x3718,0x12);//
	write_cmos_sensor(0x3712,0x42);//
	write_cmos_sensor(0x371e,0x19);//
	write_cmos_sensor(0x371f,0x40);//
	write_cmos_sensor(0x3720,0x05);//
	write_cmos_sensor(0x3721,0x05);//
	write_cmos_sensor(0x3724,0x02);//
	write_cmos_sensor(0x3725,0x02);//
	write_cmos_sensor(0x3726,0x06);//
	write_cmos_sensor(0x3728,0x05);//
	write_cmos_sensor(0x3729,0x02);//
	write_cmos_sensor(0x372a,0x03);//
	write_cmos_sensor(0x372b,0x53);//
	write_cmos_sensor(0x372c,0xa3);//
	write_cmos_sensor(0x372d,0x53);//
	write_cmos_sensor(0x372e,0x06);//
	write_cmos_sensor(0x372f,0x10);//
	write_cmos_sensor(0x3730,0x01);//
	write_cmos_sensor(0x3731,0x06);//
	write_cmos_sensor(0x3732,0x14);//
	write_cmos_sensor(0x3736,0x20);//
	write_cmos_sensor(0x373a,0x02);//
	write_cmos_sensor(0x373b,0x0c);//
	write_cmos_sensor(0x373c,0x0a);//
	write_cmos_sensor(0x373e,0x03);//
	write_cmos_sensor(0x375a,0x06);//
	write_cmos_sensor(0x375b,0x13);//
	write_cmos_sensor(0x375d,0x02);//
	write_cmos_sensor(0x375f,0x14);//
	write_cmos_sensor(0x3767,0x18);//
	write_cmos_sensor(0x3769,0x20);//
	write_cmos_sensor(0x3772,0x23);//
	write_cmos_sensor(0x3773,0x02);//
	write_cmos_sensor(0x3774,0x16);//
	write_cmos_sensor(0x3775,0x12);//
	write_cmos_sensor(0x3776,0x08);//
	write_cmos_sensor(0x37a0,0x44);//
	write_cmos_sensor(0x37a1,0x3d);//
	write_cmos_sensor(0x37a2,0x3d);//
	write_cmos_sensor(0x37a3,0x01);//
	write_cmos_sensor(0x37a5,0x08);//
	write_cmos_sensor(0x37a7,0x44);//
	write_cmos_sensor(0x37a8,0x58);//
	write_cmos_sensor(0x37a9,0x58);//
	write_cmos_sensor(0x37aa,0x44);//
	write_cmos_sensor(0x37ab,0x2e);//
	write_cmos_sensor(0x37ac,0x2e);//
	write_cmos_sensor(0x37ad,0x33);//
	write_cmos_sensor(0x37ae,0x0d);//
	write_cmos_sensor(0x37af,0x0d);//
	write_cmos_sensor(0x37b3,0x42);//
	write_cmos_sensor(0x37b4,0x42);//
	write_cmos_sensor(0x37b5,0x33);//

	write_cmos_sensor(0x3808,0x02);////640
	write_cmos_sensor(0x3809,0x80);//
	write_cmos_sensor(0x380a,0x01);//480
	write_cmos_sensor(0x380b,0xe0);//

	write_cmos_sensor(0x380c,((imgsensor_info.hs_video.linelength >> 8) & 0xFF));// ; HTS H
	write_cmos_sensor(0x380d,(imgsensor_info.hs_video.linelength & 0xFF));// ; HTS L
	write_cmos_sensor(0x380e,((imgsensor_info.hs_video.framelength >> 8) & 0xFF));// ; VTS H
	write_cmos_sensor(0x380f,(imgsensor_info.hs_video.framelength & 0xFF));// ; VTS L

	write_cmos_sensor(0x3813,0x04);//
	write_cmos_sensor(0x3814,0x03);//
	write_cmos_sensor(0x3821,0x6f);//
	write_cmos_sensor(0x382a,0x05);//
	write_cmos_sensor(0x382b,0x03);//
	write_cmos_sensor(0x3830,0x08);//
	write_cmos_sensor(0x3836,0x02);//
	write_cmos_sensor(0x3846,0x88);//
	write_cmos_sensor(0x3f08,0x0b);//
	write_cmos_sensor(0x4000,0xf1);//
	write_cmos_sensor(0x4001,0x14);//
	write_cmos_sensor(0x4020,0x01);//
	write_cmos_sensor(0x4021,0x20);//
	write_cmos_sensor(0x4022,0x01);//
	write_cmos_sensor(0x4023,0x9f);//
	write_cmos_sensor(0x4024,0x03);//
	write_cmos_sensor(0x4025,0xe0);//
	write_cmos_sensor(0x4026,0x04);//
	write_cmos_sensor(0x4027,0x5f);//
	write_cmos_sensor(0x402a,0x02);//
	write_cmos_sensor(0x402b,0x02);//
	write_cmos_sensor(0x402c,0x00);//
	write_cmos_sensor(0x402d,0x00);//
	write_cmos_sensor(0x402e,0x04);//
	write_cmos_sensor(0x4500,0x40);//
	write_cmos_sensor(0x4601,0x50);//
	write_cmos_sensor(0x4837,0x15);//
	write_cmos_sensor(0x5002,0x0c);//
	write_cmos_sensor(0x5901,0x04);//

	write_cmos_sensor(0x0100,0x01);// ; wake up, streaming
}



static void slim_video_setting()
{
	LOG_INF("Enter!\n");

	write_cmos_sensor(0x0100,0x00);//; software standby

	write_cmos_sensor(0x0302,0x1f);// ; PLL
	write_cmos_sensor(0x030d,0x1f);// ; PLL   ;1e
	write_cmos_sensor(0x030f,0x09);//; PLL
	write_cmos_sensor(0x3018,0x72);//
	write_cmos_sensor(0x3106,0x01);//

	//write_cmos_sensor(0x3501,0x28);//; expouere H
	//write_cmos_sensor(0x3502,0x90);//; exposure L

	write_cmos_sensor(0x3700,0x24);//; sensor control  AM24
	write_cmos_sensor(0x3701,0x0c);//
	write_cmos_sensor(0x3702,0x28);//
	write_cmos_sensor(0x3703,0x19);//
	write_cmos_sensor(0x3704,0x14);//
	write_cmos_sensor(0x3706,0x38);//AM24
	write_cmos_sensor(0x3707,0x04);//
	write_cmos_sensor(0x3708,0x24);//
	write_cmos_sensor(0x3709,0x40);//
	write_cmos_sensor(0x370a,0x00);//
	write_cmos_sensor(0x370b,0xb8);//AM24
	write_cmos_sensor(0x370c,0x04);//
	write_cmos_sensor(0x3718,0x12);//
	write_cmos_sensor(0x3712,0x42);//
	write_cmos_sensor(0x371e,0x19);//
	write_cmos_sensor(0x371f,0x40);//
	write_cmos_sensor(0x3720,0x05);//
	write_cmos_sensor(0x3721,0x05);//
	write_cmos_sensor(0x3724,0x02);//
	write_cmos_sensor(0x3725,0x02);//
	write_cmos_sensor(0x3726,0x06);//
	write_cmos_sensor(0x3728,0x05);//
	write_cmos_sensor(0x3729,0x02);//
	write_cmos_sensor(0x372a,0x03);//
	write_cmos_sensor(0x372b,0x53);//
	write_cmos_sensor(0x372c,0xa3);//
	write_cmos_sensor(0x372d,0x53);//
	write_cmos_sensor(0x372e,0x06);//
	write_cmos_sensor(0x372f,0x10);//
	write_cmos_sensor(0x3730,0x01);//
	write_cmos_sensor(0x3731,0x06);//
	write_cmos_sensor(0x3732,0x14);//
	write_cmos_sensor(0x3736,0x20);//
	write_cmos_sensor(0x373a,0x02);//
	write_cmos_sensor(0x373b,0x0c);//
	write_cmos_sensor(0x373c,0x0a);//
	write_cmos_sensor(0x373e,0x03);//
	write_cmos_sensor(0x375a,0x06);//
	write_cmos_sensor(0x375b,0x13);//
	write_cmos_sensor(0x375c,0x40);// am26
	write_cmos_sensor(0x375d,0x02);//
	write_cmos_sensor(0x375f,0x14);//
	write_cmos_sensor(0x3767,0x1c);//am26
	write_cmos_sensor(0x3769,0x20);//
	write_cmos_sensor(0x376c,0xc0);//am26
	write_cmos_sensor(0x376d,0xc0);//am26
	write_cmos_sensor(0x3772,0x23);//
	write_cmos_sensor(0x3773,0x02);//
	write_cmos_sensor(0x3774,0x16);//
	write_cmos_sensor(0x3775,0x12);//
	write_cmos_sensor(0x3776,0x08);//
	write_cmos_sensor(0x37a0,0x44);//
	write_cmos_sensor(0x37a1,0x3d);//
	write_cmos_sensor(0x37a2,0x3d);//
	write_cmos_sensor(0x37a3,0x01);//
	write_cmos_sensor(0x37a5,0x08);//
	write_cmos_sensor(0x37a7,0x44);//
	write_cmos_sensor(0x37a8,0x58);//AM24
	write_cmos_sensor(0x37a9,0x58);//AM24
	write_cmos_sensor(0x37aa,0x44);//
	write_cmos_sensor(0x37ab,0x2e);//
	write_cmos_sensor(0x37ac,0x2e);//
	write_cmos_sensor(0x37ad,0x33);//
	write_cmos_sensor(0x37ae,0x0d);//
	write_cmos_sensor(0x37af,0x0d);//
	write_cmos_sensor(0x37b3,0x42);//
	write_cmos_sensor(0x37b4,0x42);//
	write_cmos_sensor(0x37b5,0x33);//

	write_cmos_sensor(0x3808,0x06);//; X output size H	active
	write_cmos_sensor(0x3809,0x60);//; X output size L	(0x3809,0x60)
	write_cmos_sensor(0x380a,0x04);//; Y output size H	active
	write_cmos_sensor(0x380b,0xc8);//; Y output size L (0x380b,0xc8)

	write_cmos_sensor(0x380c,((imgsensor_info.slim_video.linelength >> 8) & 0xFF));// ; HTS H
	write_cmos_sensor(0x380d,(imgsensor_info.slim_video.linelength & 0xFF));// ; HTS L
	write_cmos_sensor(0x380e,((imgsensor_info.slim_video.framelength >> 8) & 0xFF));// ; VTS H
	write_cmos_sensor(0x380f,(imgsensor_info.slim_video.framelength & 0xFF));// ; VTS L

	write_cmos_sensor(0x3813,0x04);//; ISP Y win L
	write_cmos_sensor(0x3814,0x03);//; X inc odd
	write_cmos_sensor(0x3821,0x67);//; hsync_en_o, fst_vbin, mirror on
	write_cmos_sensor(0x382a,0x03);//; Y inc odd
	write_cmos_sensor(0x382b,0x01);//; Y inc even
	write_cmos_sensor(0x3830,0x08);//; ablc_use_num[5:1]
	write_cmos_sensor(0x3836,0x02);//; zline_use_num[5:1]
	write_cmos_sensor(0x3846,0x88);//; Y/X boundary pixel numbber for auto size mode
	write_cmos_sensor(0x3f08,0x0b);//

	write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en

	write_cmos_sensor(0x4001,0x14);//; left 32 column, final BLC offset limitation enable
	write_cmos_sensor(0x4020,0x01);//; anchor left start H
	write_cmos_sensor(0x4021,0x20);//; anchor left start L
	write_cmos_sensor(0x4022,0x01);//; anchor left end H
	write_cmos_sensor(0x4023,0x9f);//; anchor left end L
	write_cmos_sensor(0x4024,0x03);//; anchor right start H
	write_cmos_sensor(0x4025,0xe0);//; anchor right start L
	write_cmos_sensor(0x4026,0x04);//; anchor right end H
	write_cmos_sensor(0x4027,0x5f);//; anchor right end L
	write_cmos_sensor(0x402a,0x04);//; top black line start
	write_cmos_sensor(0x402b,0x04);//am26
	write_cmos_sensor(0x402c,0x02);//; bottom zero line start
	write_cmos_sensor(0x402d,0x02);//; bottom zero line number
	write_cmos_sensor(0x402e,0x08);//; bottom black line start
	write_cmos_sensor(0x4500,0x40);//; ADC sync control
	write_cmos_sensor(0x4601,0x74);//; V FIFO control

	write_cmos_sensor(0x4837,0x16);// ; clock period

	write_cmos_sensor(0x5002,0x08);//; vario pixel off
	write_cmos_sensor(0x5901,0x00);//

	write_cmos_sensor(0x0100,0x01);//; wake up, streaming

}





/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	*sensorID : return the sensor ID
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 1;

	write_cmos_sensor(0x0103,0x01);// Reset sensor
	mdelay(2);

	//module have defferent  i2c address;
	while (0xff != imgsensor_info.i2c_addr_table[i])
	{
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);

		do {
			*sensor_id = ((read_cmos_sensor(0x300B) << 8) | read_cmos_sensor(0x300C));
			if (*sensor_id == imgsensor_info.sensor_id)
			{
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
				break;
			}

			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);

		i++;
		retry = 1;
	}

	if(*sensor_id == imgsensor_info.sensor_id)
	{ 
		if(OV8865_QTECH_MODULE_ID == slect_Typical_index())
		{
			sprintf(factory_module_id,"sub_camera:8M-Camera OV8865-QTECH\n");
			*sensor_id = OV8865_QTECH_SENSOR_ID;
			get_device_info(factory_module_id);
		}
		else
		{
			*sensor_id = 0xFFFFFFFF;
			return ERROR_SENSOR_CONNECT_FAIL;
		}
	}
	else
	{
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	LOG_INF("gaoatao sensor id: 0x%x\n", *sensor_id);
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;
	write_cmos_sensor(0x0103,0x01);// Reset sensor
	mdelay(2);

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor(0x300B) << 8) | read_cmos_sensor(0x300C));
			if (sensor_id == imgsensor_info.sensor_id) {
                               BG_Ratio_Typical_8865=281;
                                RG_Ratio_Typical_8865=245;
                                   break;


                        }
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);

		i++;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;
	printk("-----------wwp--------Line = %d-----OV8865\n ", __LINE__);
	sensor_init();
    if(update_otp_lenc()!=0)
	{
		printk("OV8865  no OTP \n ");
	}
	printk("-----------wwp------Line = %d-------OV8865	OTP \n ", __LINE__);

	if(update_otp_wb()!=0)
	{
		printk(" OV8865 no OTP \n ");
	}
    printk("-----------wwp------Line = %d-------OV8865	OTP \n ", __LINE__);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	imgsensor.test_pattern = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}	/*	open  */



/*************************************************************************
* FUNCTION
*	close
*
* DESCRIPTION
*
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{

	/*No Need to implement this function*/

	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{


	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();

	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
* FUNCTION
*	capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);

		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);

	if(imgsensor.test_pattern == KAL_TRUE)
	{
		write_cmos_sensor(0x5E00,0x80);
		mdelay(40);
	}

	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{


	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = imgsensor_info.normal_video.max_framerate;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{


	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = imgsensor_info.hs_video.max_framerate;;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{


	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = imgsensor_info.slim_video.max_framerate;;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();

	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{

	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{



	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	//sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	//sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

			break;
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:// 2
			normal_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:// 3
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO: //9
			slim_video(image_window, sensor_config_data);
			break;
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;

	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            }
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}
	return ERROR_NONE;
}



static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);

    if(enable)
    {
		write_cmos_sensor(0x5000,0x16);// ; LENC off, MWB on, BPC on, WPC on
		write_cmos_sensor(0x5E00,0x80);
		mdelay(40);

    }
	else
	{
		write_cmos_sensor(0x5000,0x96);// ; LENC on, MWB on, BPC on, WPC on
		write_cmos_sensor(0x5E00,0x00);
		mdelay(40);
	}

	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
    unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
            set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
            night_mode((BOOL) *feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN:
            set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
            set_video_mode(*feature_data);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
            LOG_INF("current fps :%d\n", *feature_data);
			spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_SET_HDR:
			LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.ihdr_en = (BOOL)*feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
            LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", *feature_data);
            wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

            switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			break;
		default:
			break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */


static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 OV8865_QTECH_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	IMX208_MIPI_RAW_SensorInit	*/
