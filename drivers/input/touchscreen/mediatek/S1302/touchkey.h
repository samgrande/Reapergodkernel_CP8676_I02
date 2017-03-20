#ifndef _TOUCHKEY_H
#define _TOUCHKEY_H

typedef enum 
{
	ID_MAIN	=0,
	ID_SUB		=1,
	ID_INVALID	=2,
}touch_id_type;

//��д��ز���
//typedef enum 
//{
//	MODE_INVALID		=0,
//	MODE_NORMAL       	=1,
//	MODE_HANDWRITE 	=2,
//	MODE_GLOVE		=3,
//	MODE_MAX		 	=4,	         
//}touch_mode_type;	
typedef enum
{
	MODE_NORMAL		  =0,
	MODE_GLOVE      =1,
	MODE_WINDOW     =2,     
}touch_mode_type;	



typedef enum 
{
	OREITATION_INVALID	=0,
	OREITATION_0       		=1,
	OREITATION_90 			=2,	           
	OREITATION_180		=3,	    
	OREITATION_270		=4,
}touch_oreitation_type;

typedef struct touchkey_funcs {
 touch_id_type touch_id;			// 0--������1---����
 int touch_type;					// 1---��������2---������
 int (*active)(void);				// 1--��ǰʹ��״̬��0--����״̬    
 int (*firmware_need_update)(void);// 1--��Ҫ�����̼���0--�̼��Ѿ�������
 int (*firmware_do_update)(void);	//ϵͳд"update"  
 int (*need_calibrate)(void);		// 1--��ҪУ׼��0--����ҪУ׼
 int (*calibrate)(void);				//ϵͳд"calibrate"        
 int (*get_firmware_version)(char * );//���س���
 int (*reset_touchkey)(void);	//ͨ��д"reset"
 touch_mode_type (*get_mode)(void);//���뷨"handwrite" "normal"   
 int (*set_mode)(touch_mode_type );//���뷨"handwrite" "normal"       
 touch_oreitation_type (*get_oreitation)(void);//������Ӧ��"oreitation:X"      
 int (*set_oreitation)(touch_oreitation_type );	//������Ӧ��"oreitation:X"      
 int (*read_regs)(char * );			//buf[256]: ef ab [�Ĵ���]��ֵ  
 int (*write_regs)(const char * );	//buf[256]: ef ab [�Ĵ���]��ֵ 
 int (*debug)(int );				//���ص���ģʽ
 int (*get_vendor)(char *);	//��ȡTP��vendor id
 int (*get_wakeup_gesture)(char * ); //��ȡ��ǰϵͳ���õ�ģʽ��
 int (*gesture_ctrl)(const char * ); //���û���������ģʽ��������˫��,UP,DOWN,RIGHT,LEFT,C,E,O,W,M��
}touchkey_ops_tpye;

struct virtual_keys_button {
	int x;
	int y;
	int width;
	int height;
	unsigned int code;
	unsigned int key_status;
};

struct tw_platform_data{
	int  (*init)(void);	
	void  (*release)(void);
	int  (*power) (int on);	     
	int  (*reset) (int ms);     
	int  (*suspend)(void);
	int  (*resume)(void);    
	unsigned char (*get_id_pin)(void);
	struct virtual_keys_button *buttons;
	int nbuttons;
	unsigned long  irqflag;
	unsigned int gpio_irq;
	int screen_x;
	int screen_y;
	int key_debounce;
};

#endif /* _touchkey_H */

/**********************************************************************
* �������ƣ�touchkey_set_ops

* �������������ô������ڵ��������
				  
* ���������touchkey_ops_tpye

* ���������NONE  

* ����ֵ      ��0---�ɹ���-1---ʧ��

* ����˵����

* �޸�����         �޸���	              �޸�����
* --------------------------------------------------------------------
* 2011/11/19	   �봺��                  �� ��
**********************************************************************/
extern int touchkey_set_ops(touchkey_ops_tpye *ops);

