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
 touch_id_type touch_id;			// 0--������1---����
 int touch_type;					// 1---��������2---������
 int (*active)(void);				// 1--��ǰʹ��״̬��0--����״̬    
 int (*firmware_need_update)(struct device *,struct device_attribute *,char *);// 1--��Ҫ�����̼���0--�̼��Ѿ�������
 int (*firmware_do_update)(struct device *,struct device_attribute *,const char *, size_t);	//ϵͳд"update"  
 int (*need_calibrate)(struct device *,struct device_attribute *,char * );		// 1--��ҪУ׼��0--����ҪУ׼
 int (*calibrate)(struct device *,struct device_attribute *, const char *, size_t);				//ϵͳд"calibrate"        
 int (*get_firmware_version)(struct device *,struct device_attribute *,char * );//���س���
 int (*reset_touchscreen)(void);	//ͨ��д"reset"
 touch_mode_type (*get_mode)(void);//���뷨"handwrite" "normal"   
 int (*set_mode)(touch_mode_type );//���뷨"handwrite" "normal"       
 touch_oreitation_type (*get_oreitation)(void);//������Ӧ��"oreitation:X"      
 int (*set_oreitation)(touch_oreitation_type );	//������Ӧ��"oreitation:X"      
 int (*read_regs)(char * );			//buf[256]: ef ab [�Ĵ���]��ֵ  
 int (*write_regs)(const char * );	//buf[256]: ef ab [�Ĵ���]��ֵ 
 int (*debug)(int );				//���ص���ģʽ
 int (*get_vendor)(char *);	
 int (*get_wakeup_gesture)(char * ); //��ȡ��ǰϵͳ���õ�ģʽ��
 int (*gesture_ctrl)(const char * ); //���û���������ģʽ��������˫��,UP,DOWN,RIGHT,LEFT,C,E,O,W,M��
 int (*smarthull_ctrl)(int);    //����smarthull�Ŀ���
}touchscreen_ops_tpye;
extern int touchscreen_set_ops(touchscreen_ops_tpye *ops);
#endif /* _TOUCHSCREEN_H */
