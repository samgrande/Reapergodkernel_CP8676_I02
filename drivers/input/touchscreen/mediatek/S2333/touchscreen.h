#ifndef _TOUCHSCREEN_H

#define _TOUCHSCREEN_H
#include <linux/device.h>
typedef enum 
{
	ID_MAIN	        =0,
	ID_SUB		=1,
	ID_INVALID	=2,
}touch_id_type;

typedef enum 
{
	MODE_NORMAL     =0,
	MODE_GLOVE 	=1,	           
	MODE_WINDOW     =2,	         
}touch_mode_type;	

typedef enum 
{
	OREITATION_INVALID	=0,
	OREITATION_0       	=1,
	OREITATION_90 		=2,	           
	OREITATION_180		=3,	    
	OREITATION_270		=4,
}touch_oreitation_type;
typedef struct touchscreen_funcs {
 touch_id_type touch_id;			// 0--外屏；1---内屏
 int touch_type;					// 1---电容屏，2---电阻屏
 int (*active)(void);				// 1--当前使用状态，0--待机状态    
 int (*firmware_need_update)(struct device *,struct device_attribute *,char *);// 1--需要升级固件，0--固件已经是最新
 int (*firmware_do_update)(struct device *,struct device_attribute *,const char *, size_t);	//系统写"update"  
 int (*need_calibrate)(struct device *,struct device_attribute *,char * );		// 1--需要校准，0--不需要校准
 int (*calibrate)(struct device *,struct device_attribute *, const char *, size_t);				//系统写"calibrate"        
 int (*get_firmware_version)(struct device *,struct device_attribute *,char * );//返回长度
 int (*reset_touchscreen)(void);	//通信写"reset"
 touch_mode_type (*get_mode)(void);//输入法"handwrite" "normal"   
 int (*set_mode)(touch_mode_type );//输入法"handwrite" "normal"       
 touch_oreitation_type (*get_oreitation)(void);//传感器应用"oreitation:X"      
 int (*set_oreitation)(touch_oreitation_type );	//传感器应用"oreitation:X"      
 int (*read_regs)(char * );			//buf[256]: ef ab [寄存器]：值  
 int (*write_regs)(const char * );	//buf[256]: ef ab [寄存器]：值 
 int (*debug)(int );				//开关调试模式
 int (*get_vendor)(char *);	
 int (*get_wakeup_gesture)(char * ); //获取当前系统设置的模式。
 int (*gesture_ctrl)(const char * ); //设置滑动解锁的模式，包括（双击,UP,DOWN,RIGHT,LEFT,C,E,O,W,M）
 int (*smarthull_ctrl)(int);    //设置smarthull的开关
}touchscreen_ops_tpye;
extern int touchscreen_set_ops(touchscreen_ops_tpye *ops);
#endif /* _TOUCHSCREEN_H */
