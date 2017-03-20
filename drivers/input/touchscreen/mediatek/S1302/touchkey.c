/********************************************************************************/
/*                                                                              */
/* Copyright (c) 2000-2010  YULONG Company             ��������������           */
/*         ���������ͨ�ſƼ������ڣ����޹�˾  ��Ȩ���� 2000-2010               */
/*                                                                              */
/* PROPRIETARY RIGHTS of YULONG Company are involved in the                     */
/* subject matter of this material.  All manufacturing, reproduction, use,      */
/* and sales rights pertaining to this subject matter are governed by the       */
/* license agreement.  The recipient of this software implicitly accepts        */
/* the terms of the license.                                                    */
/* ������ĵ�������������˾���ʲ�,�κ���ʿ�Ķ���ʹ�ñ����ϱ�����              */
/* ��Ӧ��������Ȩ,�е��������κͽ�����Ӧ�ķ���Լ��.                             */
/*                                                                              */
/********************************************************************************/

/**************************************************************************
**  Copyright (C), 2000-2010, Yulong Tech. Co., Ltd.
**  FileName:          touchkey.c
**  Author:            �봺��
**  Version :          2.00
**  Date:                2011/11/19
**  Description:       �����������м��
**
**  History:
**  <author>      <time>      <version >      <desc>
**   �봺�� 2012/01/16     2.20           �޸Ĵ������ڵ����ԣ���ͬ���޸�init.rc����chown system
**   �봺�� 2012/01/11     2.10           ���Ӵ���������mutex lock����
**   �봺�� 2012/01/04     2.00           �޸�ֻ��ѡ���Ĵ������в���
**   �봺�� 2011/11/19     1.00           ����
**
**************************************************************************/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include "touchkey.h"

touchkey_ops_tpye *touchkey_ops[2];

#define TOUCH_IN_ACTIVE(num) (touchkey_ops[num] && touchkey_ops[num]->active && touchkey_ops[num]->active())
static DEFINE_MUTEX(touchkey_mutex);

/**********************************************************************
* �������ƣ�touchkey_set_ops

* �������������ô������ڵ��������

* ���������touchkey_ops_tpye

* ���������NONE

* ����ֵ      ��0---�ɹ���-EBUSY---ʧ��

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
int touchkey_set_ops(touchkey_ops_tpye *ops)
{
    if(ops==NULL || ops->touch_id>1 )
    {
        printk("BJ_BSP_Driver:CP_touchkey:ops error!\n");
        return -EBUSY;
    }
    mutex_lock(&touchkey_mutex);
    if(touchkey_ops[ops->touch_id]!=NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:ops has been used!\n");
        mutex_unlock(&touchkey_mutex);
        return -EBUSY;
    }
       touchkey_ops[ops->touch_id] = ops;
    mutex_unlock(&touchkey_mutex);
    printk("BJ_BSP_Driver:CP_touchkey:ops add success!\n");
    return 0;
}

/**********************************************************************
* �������ƣ�touchkey_type_show

* ��������������ǰ����������

* ���������buf

* ���������buf: 1---��������2---������

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_type_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        ret = touchkey_ops[0]->touch_type;
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        ret = touchkey_ops[1]->touch_type;
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%d\n",ret);

}

/**********************************************************************
* �������ƣ�touchkey_active_show

* ��������������ǰ������״̬

* ���������buf

* ���������buf: 1--��ǰʹ��״̬��0--����״̬

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_active_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;
    int ret1=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if (touchkey_ops[0] && touchkey_ops[0]->active)
    {
        ret = touchkey_ops[0]->active();
    }

    if (touchkey_ops[1] && touchkey_ops[1]->active)
    {
        ret1 = touchkey_ops[1]->active();
    }
    mutex_unlock(&touchkey_mutex);

    printk("BJ_BSP_Driver:CP_touchkey:%d,%d in %s\n",ret,ret1,__FUNCTION__);
    return sprintf(buf, "%d,%d\n",ret,ret1);
}

/**********************************************************************
* �������ƣ�touchkey_firmware_update_show

* ������������ѯ�������̼��Ƿ���Ҫ����

* ���������buf

* ���������buf: 1--��Ҫ�����̼���0--�̼��Ѿ�������

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t touchkey_firmware_update_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->firmware_need_update)
            ret = touchkey_ops[0]->firmware_need_update();
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->firmware_need_update)
            ret = touchkey_ops[1]->firmware_need_update();
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%d\n",ret);
}


/**********************************************************************
* �������ƣ�touchkey_firmware_update_store

* �������������¹̼�

* ���������buf:ϵͳд"update"

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_firmware_update_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    if(strncmp(buf, "update",count-1))
    {
        printk("BJ_BSP_Driver:CP_touchkey:string is %s,count=%d not update!\n",buf,(int)count);
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
         if(touchkey_ops[0]->firmware_need_update && touchkey_ops[0]->firmware_need_update() && touchkey_ops[0]->firmware_do_update)
        {
                   ret = touchkey_ops[0]->firmware_do_update();
                }
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
         if(touchkey_ops[1]->firmware_need_update && touchkey_ops[1]->firmware_need_update() && touchkey_ops[1]->firmware_do_update)
        {
                   ret = touchkey_ops[1]->firmware_do_update();
                }
    }
    mutex_unlock(&touchkey_mutex);
    return ret;
}


/**********************************************************************
* �������ƣ�touchkey_calibrate_store

* ����������������У׼

* ���������buf:ϵͳд"calibrate"

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t touchkey_calibrate_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->need_calibrate)
            ret = touchkey_ops[0]->need_calibrate();
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->need_calibrate)
            ret = touchkey_ops[1]->need_calibrate();
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%d\n",ret);
}

/**********************************************************************
* �������ƣ�touchkey_calibrate_store

* ����������������У׼

* ���������buf:ϵͳд"calibrate"

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_calibrate_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    if(strncmp(buf, "calibrate",count-1))
    {
        printk("BJ_BSP_Driver:CP_touchkey:string is %s,not calibrate!\n",buf);
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->calibrate)
            ret = touchkey_ops[0]->calibrate();
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->calibrate)
            ret = touchkey_ops[1]->calibrate();
    }
    mutex_unlock(&touchkey_mutex);
    return ret;

}


/**********************************************************************
* �������ƣ�touchkey_firmware_version_show

* ����������������дģʽ

* ���������buf

* ���������buf ���������ҡ�ic�ͺš��̼��汾

* ����ֵ      ��size ���س���

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_firmware_version_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    char version[64]={0};

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->get_firmware_version)
            touchkey_ops[0]->get_firmware_version(version);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->get_firmware_version)
            touchkey_ops[1]->get_firmware_version(version);
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%s\n",version);
}


/**********************************************************************
* �������ƣ�touchkey_reset_store

* ����������������дģʽ

* ���������buf:ͨ��д"reset"

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_reset_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;


    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }


    if(strncmp(buf, "reset",count-1))
    {
        printk("BJ_BSP_Driver:CP_touchkey:string is %s,not reset!\n",buf);
        return -EINVAL;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->reset_touchkey)
            ret = touchkey_ops[0]->reset_touchkey();
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->reset_touchkey)
            ret = touchkey_ops[1]->reset_touchkey();
    }
    mutex_unlock(&touchkey_mutex);

    return ret;

}

/**********************************************************************
* �������ƣ�touchkey_mode_show

* ��������������ǰ����������ģʽ

* ���������buf

* ���������buf: 1--��д��0--����

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_mode_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->get_mode)
        ret = touchkey_ops[0]->get_mode();
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->get_mode)
        ret = touchkey_ops[1]->get_mode();
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%d\n",ret);
}

/**********************************************************************
* �������ƣ�touchkey_work_mode_store

* �������������ù���ģʽ

* ���������buf:���뷨"handwrite" "normal"

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_mode_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;
    touch_mode_type mode = MODE_NORMAL;
    printk("touchkey_mode_store begin!\n");
    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }
    if(strncmp(buf, "normal",count-1)==0)
        mode = MODE_NORMAL;
    else if(strncmp(buf, "glove",count-1)==0)
        mode = MODE_GLOVE;
    else if(strncmp(buf, "window",count-1)==0)
        mode = MODE_WINDOW;
    else
    {
        printk("BJ_BSP_Driver:CP_touchkey:Don't support %s mode!\n",buf);
        return -EINVAL;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->set_mode)
            ret = touchkey_ops[0]->set_mode(mode);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->set_mode)
            ret = touchkey_ops[1]->set_mode(mode);
    }
    mutex_unlock(&touchkey_mutex);
    printk("store count is %d!\n", (int)count);
    return count;
}

/**********************************************************************
* �������ƣ�touchkey_oreitation_show

* ������������ȡ��������ǰ��λ

* ���������buf

* ���������buf:������Ӧ��"X"    X:1,2,3,4--> 0,90,180,270

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_oreitation_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->get_oreitation)
            ret = touchkey_ops[0]->get_oreitation();
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->get_oreitation)
            ret = touchkey_ops[1]->get_oreitation();
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%d\n",ret);
}


/**********************************************************************
* �������ƣ�touchkey_oreitate_store

* �������������ô�������ǰ��λ

* ���������buf:������Ӧ��"oreitation:X"    X:1,2,3,4--> 0,90,180,270

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_oreitation_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    ssize_t ret=0;
    int oreitation=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }


    if(strncmp(buf, "oreitation",count-2))
    {
        printk("BJ_BSP_Driver:CP_touchkey:string is %s,not oreitation\n",buf);
        return -EINVAL;
    }

    oreitation=buf[count-2]-'0';
    printk("BJ_BSP_Driver:CP_touchkey:oreitation=%d",oreitation);
    if(oreitation<0 || oreitation>3)
    {
        printk("BJ_BSP_Driver:CP_touchkey:oreitation[%d] is invalid\n",oreitation);
        return -EINVAL;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->set_oreitation)
            ret = touchkey_ops[0]->set_oreitation(oreitation);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->set_oreitation)
            ret = touchkey_ops[1]->set_oreitation(oreitation);
    }
    mutex_unlock(&touchkey_mutex);

    return ret;

}


/**********************************************************************
* �������ƣ�touchkey_read_regs_show

* ������������������ic�Ĵ���

* ���������NONE

* ���������buf[256]: ef:ab [�Ĵ���]��ֵ

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_regs_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret=0;
    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0] ->read_regs)
            ret=touchkey_ops[0] ->read_regs(buf);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1] ->read_regs)
            ret=touchkey_ops[1] ->read_regs(buf);
    }
    mutex_unlock(&touchkey_mutex);

    return ret;
}


/**********************************************************************
* �������ƣ�touchkey_write_regs_store

* ����������д�������Ĵ���ֵ

* ���������buf[256]: ef:ab [�Ĵ���]��ֵ

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_regs_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;
    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0] ->write_regs)
            ret=touchkey_ops[0] ->write_regs(buf);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1] ->write_regs)
            ret=touchkey_ops[1] ->write_regs(buf);
    }
    mutex_unlock(&touchkey_mutex);
    return ret;
}


/**********************************************************************
* �������ƣ�touchkey_debug_store

* �������������õ���ģʽ

* ���������buf:"on" "off"

* ���������buf

* ����ֵ      ��size

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2011/11/19       �봺��                  �� ��
**********************************************************************/
static ssize_t  touchkey_debug_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;
    int on=0;

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    if(strncmp(buf, "on",count-1)==0)
        on=1;

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0] ->debug)
            ret= touchkey_ops[0] ->debug(on);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1] ->debug)
            ret= touchkey_ops[1] ->debug(on);
    }
    mutex_unlock(&touchkey_mutex);
    return count;
}


/**********************************************************************
* �������ƣ�touchkey_vendor_show

* ������������ѯ����IC����

* ���������buf

* ���������buf ������IC ����

* ����ֵ      ��size ���س���

* ����˵����

* �޸�����         �޸���                 �޸�����
* --------------------------------------------------------------------
* 2013/7/10    �ƻ���                 �� ��
**********************************************************************/
static ssize_t  touchkey_vendor_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    char vendor[64]={0};

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->get_vendor)
            touchkey_ops[0]->get_vendor(vendor);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->get_vendor)
            touchkey_ops[1]->get_vendor(vendor);
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%s\n",vendor);
}


/*added by yewenliang for gesture wakeup*/
static ssize_t  touchkey_gesture_wakeup_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    char gesture[64]={0};

    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0]->get_wakeup_gesture)
            touchkey_ops[0]->get_wakeup_gesture(gesture);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1]->get_wakeup_gesture)
            touchkey_ops[1]->get_wakeup_gesture(gesture);
    }
    mutex_unlock(&touchkey_mutex);

    return sprintf(buf, "%s\n",gesture);
}

static ssize_t  touchkey_gesture_ctrl_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;
    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }

    printk("%s: count = %d.\n", __func__, (int)count);
    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0] ->gesture_ctrl)
            ret= touchkey_ops[0] ->gesture_ctrl(buf);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1] ->gesture_ctrl)
            ret= touchkey_ops[1] ->gesture_ctrl(buf);
    }
    mutex_unlock(&touchkey_mutex);
    return count;
}
/*
static ssize_t  touchkey_smarthull_ctrl_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret=0;
    int smarthull_switch = 0;
    if(buf==NULL)
    {
        printk("BJ_BSP_Driver:CP_touchkey:buf is NULL!\n");
        return -ENOMEM;
    }
    if(strncmp(buf, "smarthull",count-1)==0)
        smarthull_switch = 1;
    else
        smarthull_switch = 0;

    mutex_lock(&touchkey_mutex);
    if(TOUCH_IN_ACTIVE(0))
    {
        if(touchkey_ops[0] ->smarthull_ctrl)
            ret= touchkey_ops[0] ->smarthull_ctrl(smarthull_switch);
    }
    else if(TOUCH_IN_ACTIVE(1))
    {
        if(touchkey_ops[1] ->smarthull_ctrl)
            ret= touchkey_ops[1] ->smarthull_ctrl(smarthull_switch);
    }
    mutex_unlock(&touchkey_mutex);
    return count;
}
*/
static DEVICE_ATTR(type, 0444, touchkey_type_show, NULL);
static DEVICE_ATTR(active, 0444, touchkey_active_show, NULL);
static DEVICE_ATTR(firmware_update, 0666, touchkey_firmware_update_show, touchkey_firmware_update_store);
static DEVICE_ATTR(calibrate, 0664, touchkey_calibrate_show, touchkey_calibrate_store);
static DEVICE_ATTR(firmware_version, 0444, touchkey_firmware_version_show,NULL);
static DEVICE_ATTR(reset, 0224, NULL, touchkey_reset_store);
static DEVICE_ATTR(mode, 0664, touchkey_mode_show, touchkey_mode_store);
static DEVICE_ATTR(oreitation, 0664, touchkey_oreitation_show, touchkey_oreitation_store);
static DEVICE_ATTR(regs, 0664, touchkey_regs_show, touchkey_regs_store);
static DEVICE_ATTR(debug, 0224, NULL, touchkey_debug_store);
static DEVICE_ATTR(vendor, 0444, touchkey_vendor_show, NULL);
static DEVICE_ATTR(gesture_wakeup, 0444, touchkey_gesture_wakeup_show, NULL);
static DEVICE_ATTR(gesture_ctrl, 0222, NULL, touchkey_gesture_ctrl_store);
//static DEVICE_ATTR(smarthull_ctrl, 0222, NULL, touchkey_smarthull_ctrl_store);

static const struct attribute *touchkey_attrs[] = {
    &dev_attr_type.attr,
    &dev_attr_active.attr,
    &dev_attr_firmware_update.attr,
    &dev_attr_calibrate.attr,
    &dev_attr_firmware_version.attr,
    &dev_attr_reset.attr,
    &dev_attr_mode.attr,
    &dev_attr_oreitation.attr,
    &dev_attr_regs.attr,
    &dev_attr_debug.attr,
    &dev_attr_vendor.attr,
    &dev_attr_gesture_wakeup.attr,
    &dev_attr_gesture_ctrl.attr,
    //&dev_attr_smarthull_ctrl.attr,
    NULL,
};

static const struct attribute_group touchkey_attr_group = {
    .attrs = (struct attribute **) touchkey_attrs,
};

static ssize_t export_store(struct class *class, struct class_attribute *attr, const char *buf, size_t len)
{
    return 1;
}

static ssize_t unexport_store(struct class *class, struct class_attribute *attr,const char *buf, size_t len)
{
    return 1;
}

static struct class_attribute uart_class_attrs[] = {
    __ATTR(export, 0200, NULL, export_store),
    __ATTR(unexport, 0200, NULL, unexport_store),
    __ATTR_NULL,
};

static struct class touchkey_class = {
    .name =     "touchkey",
    .owner =    THIS_MODULE,

    .class_attrs =  uart_class_attrs,
};

static struct device *touchkey_dev;
struct device *touchkey_get_dev(void)
{
    return touchkey_dev;
}
EXPORT_SYMBOL(touchkey_get_dev);

static int touchkey_export(void)
{
    int status = 0;
    struct device   *dev = NULL;

    dev = device_create(&touchkey_class, NULL, MKDEV(0, 0), NULL, "touchkey_dev");
    if (dev)
    {
        status = sysfs_create_group(&dev->kobj, &touchkey_attr_group);
        touchkey_dev = dev;
    }
    else
    {
        printk(KERN_ERR"BJ_BSP_Driver:CP_touchkey:uart switch sysfs_create_group fail\r\n");
        status = -ENODEV;
    }

    return status;
}

static int __init touchkey_sysfs_init(void)
{
    int status = 0;
    touchkey_ops[0]=NULL;
    touchkey_ops[1]=NULL;
    status = class_register(&touchkey_class);
    if (status < 0)
    {
        printk(KERN_ERR"BJ_BSP_Driver:CP_touchkey:uart switch class_register fail\r\n");
        return status;
    }

    status = touchkey_export();

    return status;
}

arch_initcall(touchkey_sysfs_init);

