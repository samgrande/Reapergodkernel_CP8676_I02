/*
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "DW97182AF.h"
#include "../camera/kd_camera_hw.h"
#include <linux/xlog.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

// in K2, main=3, sub=main2=1
#define LENS_I2C_BUSNUM 0
static struct i2c_board_info kd_lens_dev __initdata = { I2C_BOARD_INFO("DW97182AF", 0x17) };


#define DW97182AF_DRVNAME "DW97182AF"
#define DW97182AF_VCM_WRITE_ID           0x18
#define PLATFORM_DRIVER_NAME "lens_actuator_dw97182af"

#define DW97182AF_DEBUG
#ifdef DW97182AF_DEBUG
#define DW97182AFDB pr_debug
#else
#define DW97182AFDB(x, ...)
#endif

static spinlock_t g_DW97182AF_SpinLock;

static struct i2c_client *g_pstDW97182AF_I2Cclient;

static dev_t g_DW97182AF_devno;
static struct cdev *g_pDW97182AF_CharDrv;
static struct class *actuator_class;

static int g_s4DW97182AF_Opened;
static long g_i4MotorStatus;
static long g_i4Dir;
static unsigned long g_u4DW97182AF_INF;
static unsigned long g_u4DW97182AF_MACRO = 1023;
static unsigned long g_u4TargetPosition;
static unsigned long g_u4CurrPosition;

static int g_sr = 3;

static int i2c_read(u8 a_u2Addr, u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[1] = { (char)(a_u2Addr) };
	i4RetValue = i2c_master_send(g_pstDW97182AF_I2Cclient, puReadCmd, 1);
	if (i4RetValue != 2) {
		DW97182AFDB(" I2C write failed!!\n");
		return -1;
	}
	/*  */
	i4RetValue = i2c_master_recv(g_pstDW97182AF_I2Cclient, (char *)a_puBuff, 1);
	if (i4RetValue != 1) {
		DW97182AFDB(" I2C read failed!!\n");
		return -1;
	}

	return 0;
}

static u8 read_data(u8 addr)
{
	u8 get_byte = 0;
	i2c_read(addr, &get_byte);
	DW97182AFDB("[DW97182AF]  get_byte %d\n", get_byte);
	return get_byte;
}

static int s4DW97182AF_ReadReg(unsigned short *a_pu2Result)
{
	/* int  i4RetValue = 0; */
	/* char pBuff[2]; */

	*a_pu2Result = (read_data(0x02) << 8) + (read_data(0x03) & 0xff);

	DW97182AFDB("[DW97182AF]  s4DW97182AF_ReadReg %d\n", *a_pu2Result);
	return 0;
}

static int s4DW97182AF_WriteReg(u16 a_u2Data)
{
	int i4RetValue = 0;

	char puSendCmd[3] = { 0x02, (char)(a_u2Data >> 8), (char)(a_u2Data & 0xFF) };

	DW97182AFDB("[DW97182AF]  write %d\n", a_u2Data);

	g_pstDW97182AF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG;
	i4RetValue = i2c_master_send(g_pstDW97182AF_I2Cclient, puSendCmd, 3);

	if (i4RetValue < 0) {
		DW97182AFDB("[DW97182AF] I2C send failed!!\n");
		return -1;
	}

	return 0;
}

inline static int getDW97182AFInfo(__user stDW97182AF_MotorInfo * pstMotorInfo)
{
	stDW97182AF_MotorInfo stMotorInfo;
	stMotorInfo.u4MacroPosition = g_u4DW97182AF_MACRO;
	stMotorInfo.u4InfPosition = g_u4DW97182AF_INF;
	stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	stMotorInfo.bIsSupportSR = TRUE;

	if (g_i4MotorStatus == 1) {
		stMotorInfo.bIsMotorMoving = 1;
	} else {
		stMotorInfo.bIsMotorMoving = 0;
	}

	if (g_s4DW97182AF_Opened >= 1) {
		stMotorInfo.bIsMotorOpen = 1;
	} else {
		stMotorInfo.bIsMotorOpen = 0;
	}

	if (copy_to_user(pstMotorInfo, &stMotorInfo, sizeof(stDW97182AF_MotorInfo))) {
		DW97182AFDB("[DW97182AF] copy to user failed when getting motor information\n");
	}

	return 0;
}
static void initdrv()
{
	char puSendCmd2[2] = { 0x01, 0x39 };
	char puSendCmd3[2] = { 0x05, 0x65 };
	i2c_master_send(g_pstDW97182AF_I2Cclient, puSendCmd2, 2);
	i2c_master_send(g_pstDW97182AF_I2Cclient, puSendCmd3, 2);
}

inline static int moveDW97182AF(unsigned long a_u4Position)
{
	int ret = 0;

	if ((a_u4Position > g_u4DW97182AF_MACRO) || (a_u4Position < g_u4DW97182AF_INF)) {
		DW97182AFDB("[DW97182AF] out of range\n");
		return -EINVAL;
	}

	if (g_s4DW97182AF_Opened == 1) {
		unsigned short InitPos;
		initdrv();
		ret = s4DW97182AF_ReadReg(&InitPos);

		if (ret == 0) {
			DW97182AFDB("[DW97182AF] Init Pos %6d\n", InitPos);

			spin_lock(&g_DW97182AF_SpinLock);
			g_u4CurrPosition = (unsigned long)InitPos;
			spin_unlock(&g_DW97182AF_SpinLock);
		} else {
			spin_lock(&g_DW97182AF_SpinLock);
			g_u4CurrPosition = 0;
			spin_unlock(&g_DW97182AF_SpinLock);
		}

		spin_lock(&g_DW97182AF_SpinLock);
		g_s4DW97182AF_Opened = 2;
		spin_unlock(&g_DW97182AF_SpinLock);

	}

	if (g_u4CurrPosition < a_u4Position) {
		spin_lock(&g_DW97182AF_SpinLock);
		g_i4Dir = 1;
		spin_unlock(&g_DW97182AF_SpinLock);
	} else if (g_u4CurrPosition > a_u4Position) {
		spin_lock(&g_DW97182AF_SpinLock);
		g_i4Dir = -1;
		spin_unlock(&g_DW97182AF_SpinLock);
	} else {
		return 0;
	}

	spin_lock(&g_DW97182AF_SpinLock);
	g_u4TargetPosition = a_u4Position;
	spin_unlock(&g_DW97182AF_SpinLock);

	/* DW97182AFDB("[DW97182AF] move [curr] %d [target] %d\n", g_u4CurrPosition, g_u4TargetPosition); */

	spin_lock(&g_DW97182AF_SpinLock);
	g_sr = 3;
	g_i4MotorStatus = 0;
	spin_unlock(&g_DW97182AF_SpinLock);

	if (s4DW97182AF_WriteReg((unsigned short)g_u4TargetPosition) == 0) {
		spin_lock(&g_DW97182AF_SpinLock);
		g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
		spin_unlock(&g_DW97182AF_SpinLock);
	} else {
		DW97182AFDB("[DW97182AF] set I2C failed when moving the motor\n");
		spin_lock(&g_DW97182AF_SpinLock);
		g_i4MotorStatus = -1;
		spin_unlock(&g_DW97182AF_SpinLock);
	}

	return 0;
}

inline static int setDW97182AFInf(unsigned long a_u4Position)
{
	spin_lock(&g_DW97182AF_SpinLock);
	g_u4DW97182AF_INF = a_u4Position;
	spin_unlock(&g_DW97182AF_SpinLock);
	return 0;
}

inline static int setDW97182AFMacro(unsigned long a_u4Position)
{
	spin_lock(&g_DW97182AF_SpinLock);
	g_u4DW97182AF_MACRO = a_u4Position;
	spin_unlock(&g_DW97182AF_SpinLock);
	return 0;
}

/* ////////////////////////////////////////////////////////////// */
static long DW97182AF_Ioctl(struct file *a_pstFile,
			   unsigned int a_u4Command, unsigned long a_u4Param)
{
	long i4RetValue = 0;

	switch (a_u4Command) {
	case DW97182AFIOC_G_MOTORINFO:
		i4RetValue = getDW97182AFInfo((__user stDW97182AF_MotorInfo *) (a_u4Param));
		break;

	case DW97182AFIOC_T_MOVETO:
		i4RetValue = moveDW97182AF(a_u4Param);
		break;

	case DW97182AFIOC_T_SETINFPOS:
		i4RetValue = setDW97182AFInf(a_u4Param);
		break;

	case DW97182AFIOC_T_SETMACROPOS:
		i4RetValue = setDW97182AFMacro(a_u4Param);
		break;

	default:
		DW97182AFDB("[DW97182AF] No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}


/* Main jobs: */
/* 1.check for device-specified errors, device not ready. */
/* 2.Initialize the device if it is opened for the first time. */
/* 3.Update f_op pointer. */
/* 4.Fill data structures into private_data */
/* CAM_RESET */
static int DW97182AF_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	DW97182AFDB("[DW97182AF] DW97182AF_Open - Start\n");
	if (g_s4DW97182AF_Opened) {
		DW97182AFDB("[DW97182AF] the device is opened\n");
		return -EBUSY;
	}
	spin_lock(&g_DW97182AF_SpinLock);
	g_s4DW97182AF_Opened = 1;
	spin_unlock(&g_DW97182AF_SpinLock);
	DW97182AFDB("[DW97182AF] DW97182AF_Open - End\n");
	return 0;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
static int DW97182AF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	DW97182AFDB("[DW97182AF] DW97182AF_Release - Start\n");

    if (g_s4DW97182AF_Opened == 2)
    {
		g_sr = 5;
    }
	if (g_s4DW97182AF_Opened) {
		DW97182AFDB("[DW97182AF] feee\n");

		spin_lock(&g_DW97182AF_SpinLock);
		g_s4DW97182AF_Opened = 0;
		spin_unlock(&g_DW97182AF_SpinLock);

	}
	DW97182AFDB("[DW97182AF] DW97182AF_Release - End\n");

	return 0;
}

static const struct file_operations g_stDW97182AF_fops = {
	.owner = THIS_MODULE,
	.open = DW97182AF_Open,
	.release = DW97182AF_Release,
	.unlocked_ioctl = DW97182AF_Ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = DW97182AF_Ioctl,
#endif
};

inline static int Register_DW97182AF_CharDrv(void)
{
	struct device *vcm_device = NULL;

	DW97182AFDB("[DW97182AF] Register_DW97182AF_CharDrv - Start\n");

	/* Allocate char driver no. */
	if (alloc_chrdev_region(&g_DW97182AF_devno, 0, 1, DW97182AF_DRVNAME)) {
		DW97182AFDB("[DW97182AF] Allocate device no failed\n");

		return -EAGAIN;
	}
	/* Allocate driver */
	g_pDW97182AF_CharDrv = cdev_alloc();

	if (NULL == g_pDW97182AF_CharDrv) {
		unregister_chrdev_region(g_DW97182AF_devno, 1);

		DW97182AFDB("[DW97182AF] Allocate mem for kobject failed\n");

		return -ENOMEM;
	}
	/* Attatch file operation. */
	cdev_init(g_pDW97182AF_CharDrv, &g_stDW97182AF_fops);

	g_pDW97182AF_CharDrv->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(g_pDW97182AF_CharDrv, g_DW97182AF_devno, 1)) {
		DW97182AFDB("[DW97182AF] Attatch file operation failed\n");

		unregister_chrdev_region(g_DW97182AF_devno, 1);

		return -EAGAIN;
	}

	actuator_class = class_create(THIS_MODULE, "actuatordrvDW97182AF");
	if (IS_ERR(actuator_class)) {
		int ret = PTR_ERR(actuator_class);
		DW97182AFDB("Unable to create class, err = %d\n", ret);
		return ret;
	}

	vcm_device = device_create(actuator_class, NULL, g_DW97182AF_devno, NULL, DW97182AF_DRVNAME);

	if (NULL == vcm_device) {
		return -EIO;
	}

	DW97182AFDB("[DW97182AF] Register_DW97182AF_CharDrv - End\n");
	return 0;
}

inline static void Unregister_DW97182AF_CharDrv(void)
{
	DW97182AFDB("[DW97182AF] Unregister_DW97182AF_CharDrv - Start\n");

	/* Release char driver */
	cdev_del(g_pDW97182AF_CharDrv);

	unregister_chrdev_region(g_DW97182AF_devno, 1);

	device_destroy(actuator_class, g_DW97182AF_devno);

	class_destroy(actuator_class);

	DW97182AFDB("[DW97182AF] Unregister_DW97182AF_CharDrv - End\n");
}

/* //////////////////////////////////////////////////////////////////// */

static int DW97182AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int DW97182AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id DW97182AF_i2c_id[] = { {DW97182AF_DRVNAME, 0}, {} };

struct i2c_driver DW97182AF_i2c_driver = {
	.probe = DW97182AF_i2c_probe,
	.remove = DW97182AF_i2c_remove,
	.driver.name = DW97182AF_DRVNAME,
	.id_table = DW97182AF_i2c_id,
};

#if 0
static int DW97182AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strcpy(info->type, DW97182AF_DRVNAME);
	return 0;
}
#endif
static int DW97182AF_i2c_remove(struct i2c_client *client)
{
	return 0;
}

/* Kirby: add new-style driver {*/
static int DW97182AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i4RetValue = 0;

	DW97182AFDB("[DW97182AF] DW97182AF_i2c_probe\n");

	/* Kirby: add new-style driver { */
	g_pstDW97182AF_I2Cclient = client;
    g_pstDW97182AF_I2Cclient->addr = DW97182AF_VCM_WRITE_ID;//gaoatao add
	g_pstDW97182AF_I2Cclient->addr = g_pstDW97182AF_I2Cclient->addr >> 1;

	/* Register char driver */
	i4RetValue = Register_DW97182AF_CharDrv();

	if (i4RetValue) {

		DW97182AFDB("[DW97182AF] register char device failed!\n");

		return i4RetValue;
	}

	spin_lock_init(&g_DW97182AF_SpinLock);

	DW97182AFDB("[DW97182AF] Attached!!\n");

	return 0;
}

static int DW97182AF_probe(struct platform_device *pdev)
{
	return i2c_add_driver(&DW97182AF_i2c_driver);
}

static int DW97182AF_remove(struct platform_device *pdev)
{
	i2c_del_driver(&DW97182AF_i2c_driver);
	return 0;
}

static int DW97182AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int DW97182AF_resume(struct platform_device *pdev)
{
	return 0;
}

/* platform structure */
static struct platform_driver g_stDW97182AF_Driver = {
	.probe = DW97182AF_probe,
	.remove = DW97182AF_remove,
	.suspend = DW97182AF_suspend,
	.resume = DW97182AF_resume,
	.driver = {
		   .name = PLATFORM_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   }
};
static struct platform_device g_stDW97182AF_device = {
    .name = PLATFORM_DRIVER_NAME,
    .id = 0,
    .dev = {}
};
static int __init DW97182AF_i2C_init(void)
{
	i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
  if(platform_device_register(&g_stDW97182AF_device)){
    DW97182AFDB("failed to register AF driver\n");
    return -ENODEV;
  }
	if (platform_driver_register(&g_stDW97182AF_Driver)) {
		DW97182AFDB("failed to register DW97182AF driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit DW97182AF_i2C_exit(void)
{
	platform_driver_unregister(&g_stDW97182AF_Driver);
}
module_init(DW97182AF_i2C_init);
module_exit(DW97182AF_i2C_exit);

MODULE_DESCRIPTION("DW97182AF lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");
