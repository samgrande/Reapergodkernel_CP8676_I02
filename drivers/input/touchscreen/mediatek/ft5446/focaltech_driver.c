/*--------------------------------------------------------------------------
*
*Focaltech touchscreen code
*
*-------------------------------------------------------------------------*/
#include "tpd.h"

#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/atomic.h>

#include "tpd_custom_fts.h"

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_boot_common.h>

#include <linux/input.h>
#include <linux/input/mt.h>

#include "cust_gpio_usage.h"

#include <linux/dma-mapping.h>  //dma
#include <mach/ext_wd_drv.h>

#ifdef FTS_CTL_IIC
#include "focaltech_ctl.h"
#endif
#ifdef TPD_SYSFS_DEBUG
#include "focaltech_ex_fun.h"
#endif

//shihaobin@yulong.com add ESD protect begin 20150729
#if FT_ESD_PROTECT
extern int apk_debug_flag;
#endif
//shihaobin@yulong.com add ESD protect begin 20150729

/*--------------------------------------------------------------------------
*
*UPGRADE INFO DEFINE
*
*-------------------------------------------------------------------------*/
struct Upgrade_Info fts_updateinfo[] =
{
    {0x54,"FT5x46",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,2, 2, 0x54, 0x2c, 10, 2000},
    {0x55,"FT5x06",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000},
    {0x08,"FT5606",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 10, 0x79, 0x06, 100, 2000},
    {0x0a,"FT5x16",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x07, 10, 1500},
    {0x06,"FT6x06",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,100, 30, 0x79, 0x08, 10, 2000},
    {0x36,"FT6x36",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x18, 10, 2000},
    {0x55,"FT5x06i",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000},
    {0x14,"FT5336",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
    {0x13,"FT3316",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
    {0x12,"FT5436i",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
    {0x11,"FT5336i",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
};

struct Upgrade_Info fts_updateinfo_curr;
unsigned char vendorID = 0x0;

//[BEGIN] liushilong@yulong.com save color info for ft5446 2015-7-6
bool real_isGolden = FALSE; //the real color for current TW true--golden false--white
unsigned char currentColor; //save current kernel TW color info 1--golden.0--white
//[END]

/*--------------------------------------------------------------------------
*
*TPD PROXIMITY DEFINE
*
*-------------------------------------------------------------------------*/

#ifdef TPD_PROXIMITY
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>

#define APS_ERR(fmt,arg...)             printk("<<proximity>> "fmt"\n",##arg)
#define TPD_PROXIMITY_DEBUG(fmt,arg...) printk("<<proximity>> "fmt"\n",##arg)
#define TPD_PROXIMITY_DMESG(fmt,arg...) printk("<<proximity>> "fmt"\n",##arg)

static u8 tpd_proximity_flag        = 0;
static u8 tpd_proximity_flag_one    = 0; //add for tpd_proximity by wangdongfang
static u8 tpd_proximity_detect      = 1; //0-->close ; 1--> far away
#endif

//shihaobin@yulong.com modify for EDS protect begin 20170729
#if FT_ESD_PROTECT
    #define TPD_ESD_CHECK_CIRCLE                                200
    static struct delayed_work gtp_esd_check_work;
    static struct workqueue_struct *gtp_esd_check_workqueue     = NULL;
    static int count_irq                                        = 0;
    static u8 run_check_91_register                             = 0;
    static unsigned long esd_check_circle                       = TPD_ESD_CHECK_CIRCLE;
    static void gtp_esd_check_func(struct work_struct *);
#endif
//shihaobin@yulong.com modify for EDS protect end 20170729

/*--------------------------------------------------------------------------
*
*TPD GESTURE DEFINE
*
*-------------------------------------------------------------------------*/

#ifdef FTS_GESTRUE
//#include "ft_gesture_lib.h"
short pointnum = 0;
unsigned long time_stamp = 0;

#define FTS_GESTURE_POINTS 255
#define FTS_GESTURE_POINTS_ONETIME  62
#define FTS_GESTURE_POINTS_HEADER 8
#define FTS_GESTURE_OUTPUT_ADRESS 0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 4
int gestrue_id = 0;

unsigned short coordinate_x[150] = {0};
unsigned short coordinate_y[150] = {0};

typedef enum
{
    DOZE_DISABLED = 0,
    DOZE_ENABLED = 1,
    DOZE_WAKEUP = 2,
}DOZE_T;
static DOZE_T doze_status = DOZE_DISABLED;

enum support_gesture_e {
    TW_SUPPORT_NONE_SLIDE_WAKEUP = 0x0,
    TW_SUPPORT_UP_SLIDE_WAKEUP = 0x1,
    TW_SUPPORT_DOWN_SLIDE_WAKEUP = 0x2,
    TW_SUPPORT_LEFT_SLIDE_WAKEUP = 0x4,
    TW_SUPPORT_RIGHT_SLIDE_WAKEUP = 0x8,
    TW_SUPPORT_E_SLIDE_WAKEUP = 0x10,
    TW_SUPPORT_O_SLIDE_WAKEUP = 0x20,
    TW_SUPPORT_W_SLIDE_WAKEUP = 0x40,
    TW_SUPPORT_C_SLIDE_WAKEUP = 0x80,
    TW_SUPPORT_M_SLIDE_WAKEUP = 0x100,
    TW_SUPPORT_DOUBLE_CLICK_WAKEUP = 0x200,
    TW_SUPPORT_L_SLIDE_WAKEUP = 0x400,
    TW_SUPPORT_S_SLIDE_WAKEUP = 0x800,
    TW_SUPPORT_V_SLIDE_WAKEUP = 0x1000,

    TW_SUPPORT_GESTURE_IN_ALL = (TW_SUPPORT_UP_SLIDE_WAKEUP |
                                 TW_SUPPORT_DOWN_SLIDE_WAKEUP |
                                 TW_SUPPORT_LEFT_SLIDE_WAKEUP |
                                 TW_SUPPORT_RIGHT_SLIDE_WAKEUP |
                                 TW_SUPPORT_E_SLIDE_WAKEUP |
                                 TW_SUPPORT_O_SLIDE_WAKEUP |
                                 TW_SUPPORT_W_SLIDE_WAKEUP |
                                 TW_SUPPORT_C_SLIDE_WAKEUP |
                                 TW_SUPPORT_M_SLIDE_WAKEUP |
                                 TW_SUPPORT_DOUBLE_CLICK_WAKEUP |
                                 TW_SUPPORT_L_SLIDE_WAKEUP |
                                 TW_SUPPORT_S_SLIDE_WAKEUP |
                                 TW_SUPPORT_V_SLIDE_WAKEUP)
};

u32 support_gesture = TW_SUPPORT_NONE_SLIDE_WAKEUP;
char wakeup_slide[32];

static int SCREEN_MAX_X = 1080;
static int SCREEN_MAX_Y = 1920;
#endif

#define TPD_GESTURE
#ifdef TPD_GESTURE
#define TPD_GESTURE_DBG(fmt, args...)         printk("<3>""[TPD_GESTURE]" fmt, ## args)
#else
#define TPD_GESTURE_DBG(fmt, args...)         do{}while(0)
#endif
/*--------------------------------------------------------------------------
*
*window and glove
*
*-------------------------------------------------------------------------*/

#ifdef CONFIG_COVER_WINDOW_SIZE

struct cover_window_info {
     unsigned int win_x_min;
     unsigned int win_x_max;

     unsigned int win_y_min;
     unsigned int win_y_max;

     atomic_t windows_switch;     //on/off
};

struct cover_window_info fts_cover_window;
#endif

unsigned char YL_I2C_STATUS = 0;

u8 current_mode=1;      // current is normal
u8 glove_switch=0;      // it default close:1--open..0--close

#define ID_G_GLOVE_MODE_EN 0xc0     //glove mode: 1--open  0--close
#define ID_G_COVER_MODE_EN 0xc1     //cover mode: 1--open  0--close

#define ID_G_COVER_WINDOW_LEFT      0xc4
#define ID_G_COVER_WINDOW_RIGHT     0xc5
#define ID_G_COVER_WINDOW_UP        0xc6
#define ID_G_COVER_WINDOW_DOWN      0xc7

/*--------------------------------------------------------------------------
*
*FUNCTION DECLARATION
*
*-------------------------------------------------------------------------*/

u8 focaltech_type=0;
u8 tp_number[4];

extern struct tpd_device *tpd;

struct i2c_client *i2c_client = NULL;
struct task_struct *thread = NULL;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
static DEFINE_MUTEX(i2c_access);

unsigned char read_tp_bootloader_version(void);
static void tpd_eint_interrupt_handler(void);

static int __init tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);

//zengfl add start
//static void tpd_shutdown(struct i2c_client *client);
#ifdef USB_CHARGE_DETECT
static void tpd_usb_plugin(int plugin);
static struct delayed_work Judge_tp_delayed_work;
static int b_usb_plugin = 0;
static int b_tpd_suspend = 0;
#endif
//zengfl add end

static int tpd_flag           = 0;
static int tpd_halt           = 0;
static int point_num          = 0;
static int p_point_num        = 0;

#define TPD_CLOSE_POWER_IN_SLEEP

#define TPD_RESET_ISSUE_WORKAROUND


//extern int tpd_mstar_status ;  // compatible mstar and ft6306 chenzhecong
/*add by sunxuebin begin for charging filter*/
extern kal_bool upmu_is_chr_det(void);
unsigned int chr_status_before = 0;
/*add by sunxuebin end for charging filter*/

extern int fts_i2c_Read(struct i2c_client *client, char *writebuf,
        int writelen, char *readbuf, int readlen);


#ifdef TPD_HAVE_BUTTON
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

#define VELOCITY_CUSTOM_FT5336
#ifdef VELOCITY_CUSTOM_FT5336
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

/*--------------------------------------------------------------------------
*
*MAGNIFY VELOCITY
*
*-------------------------------------------------------------------------*/

#ifndef TPD_VELOCITY_CUSTOM_X
#define TPD_VELOCITY_CUSTOM_X       10
#endif
#ifndef TPD_VELOCITY_CUSTOM_Y
#define TPD_VELOCITY_CUSTOM_Y       10
#endif

#define TOUCH_IOC_MAGIC         'A'

#define TPD_GET_VELOCITY_CUSTOM_X _IO(TOUCH_IOC_MAGIC,0)
#define TPD_GET_VELOCITY_CUSTOM_Y _IO(TOUCH_IOC_MAGIC,1)

int g_v_magnify_x =TPD_VELOCITY_CUSTOM_X;
int g_v_magnify_y =TPD_VELOCITY_CUSTOM_Y;
#include "touchscreen.h"
static int cpd_debug = 0;
#define CPD_TAG  "TouchScreen:"
#define CPD_DBG_MODE
#ifdef CPD_DBG_MODE
#define CPD_DBG(fmt, ...) \
if(cpd_debug)printk(KERN_ERR CPD_TAG pr_fmt(fmt), ##__VA_ARGS__)
#else
#define CPD_DBG \
do{}while(0)
#endif

typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

static int calibrate_flag = 0 ; /**means that don't calibrate**/
static unsigned char tp_type_from_boot=0xff;

#if FTS_HAVE_TOUCH_KEY
const u16 touch_key_array[] = {KEY_APPSELECT , KEY_HOMEPAGE, KEY_BACK};
#define FTS_MAX_KEY_NUM ( sizeof( touch_key_array )/sizeof( touch_key_array[0] ) )
#endif

static DEFINE_MUTEX(tw_mutex);

void delay_qt_ms(unsigned long  w_ms)
{
    unsigned long i;
    unsigned long j;

    for (i = 0; i < w_ms; i++)
    {
        for (j = 0; j < 1000; j++)
        {
            udelay(1);
        }
    }
}

/*--------------------------------------------------------------------------
*
*DMA READ
*
*-------------------------------------------------------------------------*/

//[BEGIN] liumingsheng@focaltech-systems.com changes dma read, add mutex lock for read and write 2015-5-28
#define __MSG_DMA_MODE__  //
#ifdef __MSG_DMA_MODE__

static u8 *I2CDMABuf_va = NULL;
static u64 I2CDMABuf_pa = 0;

int fts_dma_buffer_init(void)
{
    I2CDMABuf_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, FTS_DMA_BUF_SIZE, &I2CDMABuf_pa, GFP_KERNEL);

    if (!I2CDMABuf_va)
    {
        FTS_DBG("[FTS] %s Allocate DMA I2C Buffer failed!\n", __func__);
        return -EIO;
    }
    return 0;
}

int fts_dma_buffer_deinit(void)
{
    if (I2CDMABuf_va)
    {
        dma_free_coherent(&tpd->dev->dev, FTS_DMA_BUF_SIZE, I2CDMABuf_va, I2CDMABuf_pa);
        I2CDMABuf_va = NULL;
        I2CDMABuf_pa = 0;
        FTS_DBG("[FTS] %s free DMA I2C Buffer...\r\n", __func__);
    }
    return 0;
}


static DEFINE_MUTEX(i2c_rw_access);

int fts_i2c_Read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
    int ret,i;

    mutex_lock(&i2c_access);

    if(writelen > 0)
    {
        //DMA Write
        {
            for(i = 0 ; i < writelen; i++)
            {
                I2CDMABuf_va[i] = writebuf[i];
            }
            client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;

            if((ret=i2c_master_send(client, (unsigned char *)I2CDMABuf_pa, writelen))!=writelen)
            {
                FTS_DBG( "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,I2CDMABuf_pa);
            }
            client->addr = client->addr & I2C_MASK_FLAG &(~ I2C_DMA_FLAG);
        }
    }
    //DMA Read
    if(readlen > 0)
    {
        {
            client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
            ret = i2c_master_recv(client, (unsigned char *)I2CDMABuf_pa, readlen);
            for(i = 0; i < readlen; i++)
            {
                readbuf[i] = I2CDMABuf_va[i];
            }
            //memcpy(readbuf, I2CDMABuf_va, readlen);
            client->addr = client->addr & I2C_MASK_FLAG &(~ I2C_DMA_FLAG);
        }
    }

    mutex_unlock(&i2c_access);
    
    return ret;
}
/*write data by i2c*/

int fts_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
    int ret;
    int i = 0;

    client->addr = client->addr & I2C_MASK_FLAG;

    mutex_lock(&i2c_access);

    if (writelen > 0)
    {
        for(i = 0 ; i < writelen; i++)
        {
            I2CDMABuf_va[i] = writebuf[i];
        }
        client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;

        if((ret=i2c_master_send(client, (unsigned char *)I2CDMABuf_pa, writelen))!=writelen)
        {
            FTS_DBG( "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,I2CDMABuf_pa);
        }
        client->addr = client->addr & I2C_MASK_FLAG &(~ I2C_DMA_FLAG);
    }

    mutex_unlock(&i2c_access);
    
    return ret;
}
//[END]

int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
    unsigned char buf[2] = {0};
    buf[0] = regaddr;
    buf[1] = regvalue;

    return fts_i2c_Write(client, buf, sizeof(buf));
}


int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{
    return fts_i2c_Read(client, &regaddr, 1, regvalue, 1);
}

#endif

/***********************************************************************************************
*
*
* GET FTS UPDATE INFO
*
*
***********************************************************************************************/

void focaltech_get_upgrade_array(void)
{
    u8 chip_id;
    u32 i;

    i2c_smbus_read_i2c_block_data(i2c_client,FTS_REG_CHIP_ID,1,&chip_id);

    printk("%s chip_id = %x\n", __func__, chip_id);

    for(i=0;i<sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info);i++)
    {
        if(chip_id==fts_updateinfo[i].CHIP_ID)
        {
            memcpy(&fts_updateinfo_curr, &fts_updateinfo[i], sizeof(struct Upgrade_Info));
            break;
        }
    }

    if(i >= sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info))
    {
        memcpy(&fts_updateinfo_curr, &fts_updateinfo[0], sizeof(struct Upgrade_Info));
    }
}

void focalech_get_upgrade_specific_array(void)
{
    u8 chip_id;
    chip_id = 0x54; // change it by ic type
    memcpy(&fts_updateinfo_curr, &fts_updateinfo[0], sizeof(struct Upgrade_Info)); //fts_updateinfo[0] means the first chid id
}

static int tpd_misc_open(struct inode *inode, struct file *file)
{
/*
    file->private_data = adxl345_i2c_client;

    if(file->private_data == NULL)
    {
        printk("tpd: null pointer!!\n");
        return -EINVAL;
    }
*/
  return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int tpd_misc_release(struct inode *inode, struct file *file)
{
  //file->private_data = NULL;
  return 0;
}
/*----------------------------------------------------------------------------*/
//static int adxl345_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
//       unsigned long arg)
static long tpd_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
    //struct i2c_client *client = (struct i2c_client*)file->private_data;
    //struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);
    //char strbuf[256];
    void __user *data;

    long err = 0;

    if(_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if(err)
    {
        printk("tpd: access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }

    switch(cmd)
    {
    case TPD_GET_VELOCITY_CUSTOM_X:
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;
        }

        if(copy_to_user(data, &g_v_magnify_x, sizeof(g_v_magnify_x)))
        {
            err = -EFAULT;
            break;
        }
        break;

    case TPD_GET_VELOCITY_CUSTOM_Y:
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;
        }

        if(copy_to_user(data, &g_v_magnify_y, sizeof(g_v_magnify_y)))
        {
            err = -EFAULT;
            break;
        }
        break;

    default:
        printk("tpd: unknown IOCTL: 0x%08x\n", cmd);
        err = -ENOIOCTLCMD;
        break;
    }

    return err;
}


static struct file_operations tpd_fops = {
    //.owner = THIS_MODULE,
    .open = tpd_misc_open,
    .release = tpd_misc_release,
    .unlocked_ioctl = tpd_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice tpd_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "touch_ft5336",
    .fops = &tpd_fops,
};

//**********************************************
#endif

struct touch_info {
    int y[TPD_MAX_POINTS_5];
    int x[TPD_MAX_POINTS_5];
    int p[TPD_MAX_POINTS_5];
    int id[TPD_MAX_POINTS_5];
};


//unsigned short force[] = {0,0x70,I2C_CLIENT_END,I2C_CLIENT_END};
//static const unsigned short * const forces[] = { force, NULL };
//static struct i2c_client_address_data addr_data = { .forces = forces, };
static const struct i2c_device_id ft5336_tpd_id[] = {{TPD_DEVICE,0},{}};
static struct i2c_board_info __initdata ft5336_i2c_tpd={ I2C_BOARD_INFO("mtk-tpd", (0x70>>1))};


static struct i2c_driver tpd_i2c_driver = {
    .driver = {
        .name = TPD_DEVICE,
        .owner = THIS_MODULE,
    },

    .probe = tpd_probe,
    .remove = tpd_remove,
    .id_table = ft5336_tpd_id,
    .detect = tpd_detect,
    //  .address_data = &addr_data,
};

#ifdef USB_CHARGE_DETECT
static void tpd_power_reset(void)
{
}

static void judge_tp_normal_delayed_work(struct work_struct *work)
{
    int ret = 0;
    char data[2] = {0};

    ret = i2c_smbus_read_i2c_block_data(i2c_client, 0xA6, 1, &(data[0]));
    if ( ret < 0 || data[0] == 0xA6 || data[0] == 0 )
    {
        printk("%s read i2c error ret = %d data[0] = %d", __FUNTION__,ret,data[0]);
        tpd_power_reset();
    }
    printk("tpd:%s plugin %d", __func__,b_usb_plugin);
    i2c_smbus_write_i2c_block_data(i2c_client, 0x90, 1, &b_usb_plugin);
    b_tpd_suspend = 0;
}

static void tpd_usb_plugin(int plugin)
{
    b_usb_plugin = plugin;

    printk("tpd: %s %d.\n",__func__,b_usb_plugin);

    if ( b_tpd_suspend ) return;

    i2c_smbus_write_i2c_block_data(i2c_client, 0x90, 1, &b_usb_plugin);
}
#endif

static void tpd_down(int x, int y, int p)
{
    if(x > TPD_RES_X)
    {
        TPD_DEBUG("warning: IC have sampled wrong value.\n");;
        return;
    }

    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    //printk("D[%4d %4d %4d] ", x, y, p);
    /* track id Start 0 */
    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p);//shihaobin@yulong.com add for resolve connect a line between two point 20150801
    input_mt_sync(tpd->dev);
#ifndef MT6572
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
#endif
    {
        tpd_button(x, y, 1);
    }

    if(y > TPD_RES_Y) //virtual key debounce to avoid android ANR issue
    {
        //msleep(50);
        printk("D virtual key \n");
    }

    TPD_EM_PRINT(x, y, x, y, p-1, 1);
}

static void tpd_up(int x, int y,int *count)
{
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    //printk("U[%4d %4d %4d] ", x, y, 0);
    input_mt_sync(tpd->dev);
    TPD_EM_PRINT(x, y, x, y, 0, 0);

    //(*count)--;
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 0);
    }
}

//[BEGIN] liumingsheng@focaltech-systems mask mutex lock for avoiding nest 2015-5-27
static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
    int i = 0;
    //add by sunxuebin start 2013-12-23 for CP82971 tiao ping Bug.
    int ret = 0;
    //add by sunxuebin start 2013-12-23 for CP82971 tiao ping Bug.
    int auc_i2c_write_buf[10] = {0};

#if (TPD_MAX_POINTS==2)
    unsigned char data[35] = {0};
#else
    unsigned char data[70] = {0};
    //char data[16] = {0};
#endif

    u16 high_byte,low_byte;
    u8 report_rate =0;

    p_point_num = point_num;
    if (tpd_halt)
    {
        TPD_DMESG( "tpd_touchinfo return ..\n");
        return false;
    }
    //mutex_lock(&i2c_access);

    auc_i2c_write_buf[0] = 0x00;
    auc_i2c_write_buf[1] = 0x08;
    auc_i2c_write_buf[2] = 0x10;
    auc_i2c_write_buf[3] = 0x18;
    auc_i2c_write_buf[4] = 0x88;

    ret = fts_i2c_Read(i2c_client, &auc_i2c_write_buf[0], 1, &(data[0]), 8);
    //ret = i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 8, &(data[0]));
    if (ret < 0)
    {
        //printk("%s read data 0~7 fails\n", __func__);
        //mutex_unlock(&i2c_access);
        return false;
    }

    ret = fts_i2c_Read(i2c_client, &auc_i2c_write_buf[1], 1, &(data[8]), 8);
    //ret = i2c_smbus_read_i2c_block_data(i2c_client, 0x08, 8, &(data[8]));
    if (ret < 0)
    {
        //printk("%s read data 8~15 fails\n", __func__);
       // mutex_unlock(&i2c_access);
        return false;
    }

#if (TPD_MAX_POINTS!=2)
    ret = fts_i2c_Read(i2c_client, &auc_i2c_write_buf[2], 1, &(data[16]), 8);
    //ret = i2c_smbus_read_i2c_block_data(i2c_client, 0x10, 8, &(data[16]));
    if (ret < 0)
    {
        //printk("%s read data 16~23 fails\n", __func__);
        //mutex_unlock(&i2c_access);
        return false;
    }

    ret = fts_i2c_Read(i2c_client, &auc_i2c_write_buf[3], 1, &(data[24]), 8);
    //ret = i2c_smbus_read_i2c_block_data(i2c_client, 0x18, 8, &(data[24]));
    if (ret < 0)
    {
        //printk("%s read data 24~31 fails\n", __func__);
        //mutex_unlock(&i2c_access);
        return false;
    }

    ret = fts_i2c_Read(i2c_client, &auc_i2c_write_buf[4], 1, &report_rate, 1);
    //i2c_smbus_read_i2c_block_data(i2c_client, 0x88, 1, &report_rate);
#endif

    //mutex_unlock(&i2c_access);

    /* Device Mode[2:0] == 0 :Normal operating Mode*/
    if((data[0] & 0x70) != 0)
        return false;
    /*get the number of the touch points*/
    point_num = data[2] & 0x0f;

    //printk("point_num =%d\n",point_num);

    for(i = 0; i < point_num; i++)
    {
        cinfo->p[i] = data[3+6*i] >> 6; //event flag
        cinfo->id[i] = data[3+6*i+2]>>4; //touch id
        /*get the X coordinate, 2 bytes*/

        //printk("touch point[%d] X is 0x%x:0x%x\n", i, data[3+6*i], data[3+6*i + 1]);
        high_byte = data[3+6*i];
        high_byte <<= 8;
        high_byte &= 0x0f00;
        low_byte = data[3+6*i + 1];
        cinfo->x[i] = high_byte |low_byte;

        //cinfo->x[i] =  cinfo->x[i] * 480 >> 11; //calibra

        /*get the Y coordinate, 2 bytes*/
        //printk("touch point[%d] Y is 0x%x:0x%x\n", i, data[3+6*i+2], data[3+6*i+3]);
        high_byte = data[3+6*i+2];
        high_byte <<= 8;
        high_byte &= 0x0f00;
        low_byte = data[3+6*i+3];
        cinfo->y[i] = high_byte |low_byte;

        //cinfo->y[i]=  cinfo->y[i] * 800 >> 11;
    }
    TPD_DEBUG(" cinfo->x[0] = %d, cinfo->y[0] = %d, cinfo->p[0] = %d\n", cinfo->x[0], cinfo->y[0], cinfo->p[0]);
    //TPD_DEBUG(" cinfo->x[1] = %d, cinfo->y[1] = %d, cinfo->p[1] = %d\n", cinfo->x[1], cinfo->y[1], cinfo->p[1]);
    //TPD_DEBUG(" cinfo->x[2]= %d, cinfo->y[2]= %d, cinfo->p[2] = %d\n", cinfo->x[2], cinfo->y[2], cinfo->p[2]);

    return true;
};
//[END]

#ifdef TPD_PROXIMITY
int tpd_read_ps(void)
{
    tpd_proximity_detect;
    return 0;
}

static int tpd_get_ps_value(void)
{
    return tpd_proximity_detect;
}

static int tpd_enable_ps(int enable)
{
    u8 state;
    int ret = -1;

    i2c_smbus_read_i2c_block_data(i2c_client, 0xB0, 1, &state);
    printk("[proxi_5206]read: 999 0xb0's value is 0x%02X\n", state);
    if (enable)
    {
        state |= 0x01;
        tpd_proximity_flag = 1;
        TPD_PROXIMITY_DEBUG("[proxi_5206]ps function is on\n");
    }
    else
    {
        state &= 0x00;
        tpd_proximity_flag = 0;
        TPD_PROXIMITY_DEBUG("[proxi_5206]ps function is off\n");
    }

    ret = i2c_smbus_write_i2c_block_data(i2c_client, 0xB0, 1, &state);
    TPD_PROXIMITY_DEBUG("[proxi_5206]write: 0xB0's value is 0x%02X\n", state);
    return 0;
}

int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
    void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;

    hwm_sensor_data *sensor_data;
    TPD_DEBUG("[proxi_5206]command = 0x%02X\n", command);
    switch (command)
    {
    case SENSOR_DELAY:
        if((buff_in == NULL) || (size_in < sizeof(int)))
        {
            APS_ERR("Set delay parameter error!\n");
            err = -EINVAL;
        }
        // Do nothing
        break;

    case SENSOR_ENABLE:
        if((buff_in == NULL) || (size_in < sizeof(int)))
        {
            APS_ERR("Enable sensor parameter error!\n");
            err = -EINVAL;
        }
        else
        {
            value = *(int *)buff_in;
            if(value)
            {
                if((tpd_enable_ps(1) != 0))
                {
                    APS_ERR("enable ps fail: %d\n", err);
                    return -1;
                }
            }
            else
            {
                if((tpd_enable_ps(0) != 0))
                {
                    APS_ERR("disable ps fail: %d\n", err);
                    return -1;
                }
            }
        }
        break;

    case SENSOR_GET_DATA:
        if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
        {
            APS_ERR("get sensor data parameter error!\n");
            err = -EINVAL;
        }
        else
        {
            sensor_data = (hwm_sensor_data *)buff_out;

            if((err = tpd_read_ps()))
            {
                err = -1;;
            }
            else
            {
                sensor_data->values[0] = tpd_get_ps_value();
                TPD_PROXIMITY_DEBUG("huang sensor_data->values[0] 1082 = %d\n", sensor_data->values[0]);
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }
        }
        break;
    default:
        APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
        err = -1;
        break;
    }

    return err;
}
#endif

/*add by sunxuebin begin for charging filter*/
void tpd_charging_filter(void)
{
    unsigned char buf[2];
    unsigned int chr_status = 0;

    chr_status = upmu_is_chr_det();
    if (chr_status != chr_status_before)
    {
        if (chr_status)     // charger plugged in
        {
            buf[0] = 0x8b;
            buf[1] = 0x01;
            fts_i2c_Write(i2c_client, buf, 2); //for charging filtering on
            //printk("tpd_charging_filter [charging filtering on]....\n");
            chr_status_before = chr_status;
        }
        else            // charger plugged out
        {
            buf[0] = 0x8b;
            buf[1] = 0x00;
            fts_i2c_Write(i2c_client, buf, 2); //for charging filtering off
            //printk("tpd_charging_filter [charging filtering off]....\n");
            chr_status_before = chr_status;
        }
    }
}
/*add by sunxuebin end for charging filter*/

#ifdef FTS_GESTRUE
//read but not write addr
static int fts_i2c_read_only(char *rxdata, int length)
{
    int ret;

    struct i2c_msg msgs[] = {
      {
        .addr = i2c_client->addr,
        .flags  = I2C_M_RD,
        .len  = length,
        .buf  = rxdata,
        .timing =I2C_MASTER_CLOCK
      },
    };

    //msleep(1);
    ret = i2c_transfer(i2c_client->adapter, msgs, 1);
    if (ret < 0)
        pr_err("msg %s i2c read error: %d\n", __func__, ret);

    return ret;
}

static int fts_read_Gesturedata(void)
{
    unsigned char buf[FTS_GESTURE_POINTS * 3] = { 0 };
    int ret = -1;
    int i = 0;
    buf[0] = 0xd3;
    short pointnum = 0;

    ret = fts_i2c_Read(i2c_client, buf, 1, buf, FTS_GESTURE_POINTS_HEADER);

    //printk( "tpd read FTS_GESTURE_POINTS_HEADER.\n");
    if (ret < 0)
    {
        printk( "%s read touchdata failed %d.\n", __func__,__LINE__);
        return ret;
    }

    /* FW */
    gestrue_id = buf[0];
    pointnum = (short)(buf[1]) & 0xff;
    buf[0] = 0xd3;
    printk("gesture gesture_id = %d pointnum = %d\n", gestrue_id , pointnum);//shihaobin@yulong.com modify for debug 20150731

    //[BEGIN] liushilong@yulong.com modify read gesture data for avoiding invalid 2015-7-7
    if((pointnum * 4 + 8)<255)
    {
        ret = fts_i2c_Read(i2c_client, buf, 1, buf, (pointnum * 4 + 8));
    }
    else
    {
        ret = fts_i2c_Read(i2c_client, buf, 1, buf, 255);
        ret = fts_i2c_Read(i2c_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
    }
    //[END]

    for(i=1;i<(pointnum + 2);i++)
    {
        ret=fts_i2c_read_only((buf + (4 * i)), 4);
    }

    if (ret < 0)
    {
        printk( "%s read touchdata failed %d.\n", __func__,__LINE__);
        return ret;
    }

    //check_gesture(gestrue_id);

    for(i = 0;i < pointnum;i++)
    {
        coordinate_x[i] = (((s16) buf[0 + (4 * i)]) & 0x0F) << 8 | (((s16) buf[1 + (4 * i)]) & 0xFF);
        coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) << 8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
    }
    return -1;
}
#endif

/*--------------------------------------------------------------------------
*
*Touch_event_handler
*
*-------------------------------------------------------------------------*/
extern struct device *touchscreen_get_dev(void);
static int touch_event_handler(void *unused)
{
    struct touch_info cinfo, pinfo;
    static u8 pre_touch = 0;
    int i=0;

    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);

    do
    {
        //shihaobin@yulong.com delete less important log begin 20150806
        //printk("touch event handler start!\n");
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter,tpd_flag!=0);

        tpd_flag = 0;

        set_current_state(TASK_RUNNING);

        //TPD_GESTURE_DBG("enter %s ! %d\n",__func__, __LINE__);
#ifdef FTS_GESTRUE
        int ret = 1;
        char *none_wakeup[2] = {"GESTURE=NONE",NULL};
        char *left_wakeup[2] = {"GESTURE=LEFT",NULL};
        char *right_wakeup[2] = {"GESTURE=RIGHT",NULL};
        char *up_wakeup[2] = {"GESTURE=UP",NULL};
        char *down_wakeup[2] = {"GESTURE=DOWN",NULL};
        char *doubleClick_wakeup[2] = {"GESTURE=DOUBLE_CLICK",NULL};
        char *c_wakeup[2] = {"GESTURE=C",NULL};
        char *e_wakeup[2] = {"GESTURE=E",NULL};
        char *m_wakeup[2] = {"GESTURE=M",NULL};
        char *o_wakeup[2] = {"GESTURE=O",NULL};
        char *w_wakeup[2] = {"GESTURE=W",NULL};
        char *l_wakeup[2] = {"GESTURE=L",NULL};
        char *s_wakeup[2] = {"GESTURE=S",NULL};
        char *v_wakeup[2] = {"GESTURE=V",NULL};
        char **envp;
        envp = none_wakeup;

        if (DOZE_ENABLED == doze_status)
        {
            TPD_GESTURE_DBG("enter doze_status !%s , %d\n",__func__, __LINE__);
            ret = fts_read_Gesturedata();
            if (ret == -1)
            {
                if ((0x21 == gestrue_id) && (support_gesture & TW_SUPPORT_RIGHT_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide right\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"right");
                    envp = right_wakeup;
                }
                else if ((0x20 == gestrue_id) && (support_gesture & TW_SUPPORT_LEFT_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide left\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"left");
                    envp = left_wakeup;
                }
                else if ((0x24 == gestrue_id) && (support_gesture & TW_SUPPORT_DOUBLE_CLICK_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide double_click\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"double_click");
                    envp = doubleClick_wakeup;
                }
                else if ((0x22 == gestrue_id) && (support_gesture & TW_SUPPORT_UP_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide up\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"up");
                    envp = up_wakeup;
                }
                else if ((0x23 == gestrue_id) && (support_gesture & TW_SUPPORT_DOWN_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide down\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"down");
                    envp = down_wakeup;
                }
                else if ((0x34 == gestrue_id) && (support_gesture & TW_SUPPORT_C_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide c\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"c");
                    envp = c_wakeup;
                }
                else if ((0x33 == gestrue_id) && (support_gesture & TW_SUPPORT_E_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide e\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"e");
                    envp = e_wakeup;
                }
                else if ((0x32 == gestrue_id) && (support_gesture & TW_SUPPORT_M_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide m\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"m");
                    envp = m_wakeup;
                }
                else if ((0x30 == gestrue_id) && (support_gesture & TW_SUPPORT_O_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide o\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"o");
                    envp = o_wakeup;
                }
                else if ((0x31 == gestrue_id) && (support_gesture & TW_SUPPORT_W_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide w\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"w");
                    envp = w_wakeup;
                }
                else if ((0x44 == gestrue_id) && (support_gesture & TW_SUPPORT_L_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide l\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"l");
                    envp = l_wakeup;
                }
                else if ((0x46 == gestrue_id) && (support_gesture & TW_SUPPORT_S_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide s\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"s");
                    envp = s_wakeup;
                }
                else if ((0x54 == gestrue_id) && (support_gesture & TW_SUPPORT_V_SLIDE_WAKEUP))
                {
                    TPD_GESTURE_DBG("slide v\n");
                    doze_status = DOZE_WAKEUP;
                    sprintf(wakeup_slide,"v");
                    envp = v_wakeup;
                }
            }

            if (doze_status == DOZE_WAKEUP)
            {
                struct device *touchscreen_dev;
                touchscreen_dev = touchscreen_get_dev();

                //input_report_key(tpd->dev, KEY_POWER, 1);
                //input_sync(tpd->dev);
                //input_report_key(tpd->dev, KEY_POWER, 0);
                //input_sync(tpd->dev);

                kobject_uevent_env(&touchscreen_dev->kobj, KOBJ_CHANGE, envp);
                TPD_GESTURE_DBG("send kobject uevent!\n");
                doze_status = DOZE_ENABLED;
            }
        }
        else
#endif //FTS_GESTRUE
        {
            //shihaobin@yulong.com modify for EDS protect begin 20170729
            #if FT_ESD_PROTECT
                esd_switch(0);apk_debug_flag = 1;
            #endif
            //shihaobin@yulong.com modify for EDS protect end 20170729
            if (tpd_touchinfo(&cinfo, &pinfo))
            {
                //TPD_DEBUG("point_num = %d\n",point_num);
                TPD_DEBUG_SET_TIME;

                //shihaobin@yulong.com modify for priority of VA interface virture key begin 20150716
                if (point_num)
                {
                    for(i =0; i<point_num && i<5; i++)//only support 5 point
                    {
                        //printk("button: cinfo.x[0]=%d cinfo.y[0]=%d\n",cinfo.x[0],cinfo.y[0]);
                        if(cinfo.y[0] == 2000)
                        {
                            if((cinfo.x[0]>=235)&&(cinfo.x[0]<=305))//270 +-35
                            {
                                printk("[fts] key menu\n");
                                input_report_key(tpd->dev, touch_key_array[0], 1);
                            }
                            if((cinfo.x[0]>=505)&&(cinfo.x[0]<=575))//540 +-35
                            {
                                printk("[fts] key home\n");
                                input_report_key(tpd->dev, touch_key_array[1], 1);
                            }
                            if((cinfo.x[0]>=775)&&(cinfo.x[0]<=845))//810 +-35
                            {
                                printk("[fts] key back\n");
                                input_report_key(tpd->dev, touch_key_array[2], 1);
                            }
                            point_num=0;
                            pre_touch=0;
                            input_report_key(tpd->dev, BTN_TOUCH, 1);
                            input_sync(tpd->dev);
                            printk("touch event handler touch key out!\n");
                            goto key_out;
                        }
                        
                         //[BEGIN] liushilong@yulong.com when double or more finger, it improve the VA priority 2015-7-15
                        if ((cinfo.x[i] >= 0 && cinfo.x[i] <= 1080) && (cinfo.y[i] >= 0 && cinfo.y[i] <= 1920))
                        {
                            //[BEGIN] liushilong@yulong.com modify edge unvalid reported data 2015-6-30
                            int x = cinfo.x[i];
                            int y = cinfo.y[i];

                            if(x < 20)
                            {
                                x = 20;
                            }
                            else if(x > 1061)
                            {
                                x = 1061;
                            }

                            if(y < 20)
                            {
                                y = 20;
                            }
                            else if(y > 1901)
                            {
                                y = 1901;
                            }

                            tpd_down(x, y, cinfo.id[i]);
                            //[END]

                            //printk("touch point[%d] X is 0x%x, Y is 0x%x, ID is 0x%x\n", i, x, y, cinfo.id[i]);
                            //printk("touch event handler touch down out!\n");
                            //shihaobin@yulong.com delete less important log end 20150806
                        }
                        //[END]
                    }
                }
                //shihaobin@yulong.com modify for priority of VA interface virture key begin 20150716
                else
                {
#if FTS_HAVE_TOUCH_KEY
                    input_report_key(tpd->dev, touch_key_array[0], 0);
                    input_report_key(tpd->dev, touch_key_array[1], 0);
                    input_report_key(tpd->dev, touch_key_array[2], 0);
#endif
                    //printk("touch event handler touch up out!\n");
                    tpd_up(0, 0, 0);
                }
            }

            pre_touch = point_num;

            if (tpd != NULL && tpd->dev != NULL)
            {
                input_sync(tpd->dev);
            }
key_out:

            //[BEGIN] liushilong@yulong.com change TW charging filter when handler data 2015-7-15
            tpd_charging_filter();
            //[END]
            if(tpd_mode==12)
            {
                //power down for desence debug
                //power off, need confirm with SA
                #ifdef TPD_POWER_SOURCE_CUSTOM
                    hwPowerDown(TPD_POWER_SOURCE_CUSTOM, "TP");
                #else
                    hwPowerDown(MT65XX_POWER_LDO_VGP2, "TP");
                #endif

                #ifdef TPD_POWER_SOURCE_1800
                    hwPowerDown(TPD_POWER_SOURCE_1800, "TP");
                #endif
                msleep(20);
            }
        }

        //printk("touch event handler end!\n");

    } while(!kthread_should_stop());
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(1);apk_debug_flag = 0;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    return 0;
}

static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);
    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
    printk("TPD interrupt has been triggered\n");
    TPD_DEBUG_PRINT_INT;
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        count_irq ++;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
}

/***********************************************************************************************
Name  :  fts_read_fw_ver
Input :  void
Output  :  firmware version
function  :  read TP firmware version

***********************************************************************************************/
static unsigned char fts_read_fw_ver(void)
{
    unsigned char ver = 0;
    unsigned char i = 5;
    while(i--)
    {
        //FTS_REG_FIRMID: 0xa6  fts Firmware Version Register
        i2c_smbus_read_i2c_block_data(i2c_client, FTS_REG_FW_VER, 1, &ver);

        printk("fts_read_fw_ver(0xa6) =0x%x\n",ver);
        if(ver)
            break;//read out correct fw version
        msleep(10);
    }

    return(ver);
}

 /*******************************************************
   ********  1--active  0--not active***********
*******************************************************/
int ft5x46_active(void)
{
    printk("Enter %s\n",__FUNCTION__);
    return 1;
}

/***********************************************************************************************
Name  : fts_read_tp_type

Input :  void

Output  :  TP type

function  :  read TP type

***********************************************************************************************/
unsigned char fts_read_tp_type(void)
{
    unsigned char type = 0;
    i2c_smbus_read_i2c_block_data(i2c_client, FTS_REG_VENDORID, 1, &type);// FTS_REG_VENDORID: 0xa8 TP type register
    printk("fts_read_tp_type type =%x\n",type);
    return (type);
}

/***********************************************************************************************
Name  : fts_read_chipid
Input :  void
Output  :  chip type
function  :  read chip type
***********************************************************************************************/
unsigned char fts_read_chipid(void)
{
    unsigned char chipid = 0;
    i2c_smbus_read_i2c_block_data(i2c_client, FTS_REG_CHIP_ID, 1, &chipid);// FTS_REG_CHIP_ID: 0xa3 TP type register
    printk("fts_read_chipid =%x\n",chipid);
    return (chipid);
}

//[BEGIN] liushilong@yulong.com read tp module color on 2015-7-3
static unsigned char fts_isGolden(void)
{
    unsigned char color = 0;
    unsigned char isGolden = 0;
    //0xBC register mean what color that current firmware is.  0x47 means golden, 0x31 means white
    i2c_smbus_read_i2c_block_data(i2c_client, 0xBC, 1, &color);
    printk("fts_read_color =%x\n",color);

    if(0x47 == color) {
        isGolden = 1;
    }
    else if(0x31 == color) {
        isGolden = 0;
    }

    return (isGolden);
}

static unsigned char fts_getColorANISC(void)
{
    unsigned char color = 0;
    //0xBC register mean what color that current firmware is.  0x47 means golden, 0x31 means white
    i2c_smbus_read_i2c_block_data(i2c_client, 0xBC, 1, &color);
    printk("fts_read_color =%x\n",color);
    return (color);
}
//[END]

/***********************************************************
************Get Upgrade Version from *.i File***************
************************************************************/
unsigned char fts_get_upg_ver(void)
{
    unsigned int ui_sz=0;
    int retry_c=30;
    unsigned char tp_type=0xff;
    unsigned char *pbuf = NULL;
    //printk("TP-cyd:fts_get_upg_ver:focaltech_type=%d\n",focaltech_type);
    while(retry_c > 0)
    {
        //[BEGIN] liushilong@yulong.com read boot area for get right vender id 2015-4-21
        //tp_type = fts_read_tp_type();
        tp_type = vendorID;
        //[END]

        //CPD_DBG(KERN_ERR "[FTS] tp_id = 0x%x\n", tp_type);
        printk("[FST] tp_id = 0x%x\n", tp_type);
        retry_c--;
        if(tp_type!=0xff)
        {
            break;
        }
        else
        {
            printk(KERN_ERR "Can't get TP type from the register, so bad!!!!!\n");
        }
    }

    //[BEGIN] liushilong@yulong.com diff golden and white module 2015-7-3
    printk("tp-cyd, tp_type = 0x%x\n", tp_type);
    printk("tp-cyd, read_isGolden = 0x%x\n", real_isGolden);
    if(tp_type == TW_OFILM_ID) {

        if(FALSE == real_isGolden) {
            pbuf = OFILM_CTPM_FW000;
            ui_sz = sizeof(OFILM_CTPM_FW000);
        } else {
            pbuf = OFILM_CTPM_FW001;
            ui_sz = sizeof(OFILM_CTPM_FW001);
        }

    }
    else if(tp_type == TW_YEJI_ID) {

        if(FALSE == real_isGolden) {
            pbuf = YEJI_CTPM_FW000;
            ui_sz = sizeof(YEJI_CTPM_FW000);
        } else {
            pbuf = YEJI_CTPM_FW001;
            ui_sz = sizeof(YEJI_CTPM_FW001);
        }

    }
    else if(tp_type == TW_BOEN_ID) {

        if(FALSE == real_isGolden) {
            pbuf = BOEN_CTPM_FW000;
            ui_sz = sizeof(BOEN_CTPM_FW000);
        } else {
            pbuf = BOEN_CTPM_FW001;
            ui_sz = sizeof(BOEN_CTPM_FW001);
        }

    }//[END]
    else
    {
        CPD_DBG(KERN_ERR "Can't get TP type from the register, 0x%02x!!!!!\n", tp_type);
        //printk( "Can't get TP type from the register, 0x%02x!!!!!\n", tp_type);
        return 0xff; //default value
    }

    if (ui_sz > 2)
    {
        //printk("pbuf[ui_sz - 2]  =0x%x\n", pbuf[ui_sz - 2]);
        return pbuf[ui_sz - 2];
    }
    else
    {
        CPD_DBG(KERN_ERR "Can't get TP type from the register: 0x%02x, so bad!!!!!\n", tp_type);
        return 0xff;
    }
}

extern int get_device_info(char* buf);
int fts_get_firmware_info(void)
{
    unsigned char firmware_version;
    unsigned char tp_type;
    //char firmware_version_buf[0xff];
    int retry_c=30;
    unsigned char buf[50]={0};
    CPD_DBG("Enter %s\n",__FUNCTION__);

    mutex_lock(&tw_mutex);
    while(retry_c > 0)/** reading firmware version has 30 times chances**/
    {
        firmware_version = fts_read_fw_ver();
        CPD_DBG("...Firmware version = 0x%x\n",firmware_version);
        retry_c--;
        if(firmware_version != 0xff)
        {
            break;
        }
    }

    if(retry_c <= 0)
    {
        CPD_DBG("..func:%s read version error!!!\n",__func__);
        mutex_unlock(&tw_mutex);
        return -1 ;
    }


tw_retry_id:
    retry_c=30;

    //[BEGIN] liushilong@yulong.com read boot area for get right vender id 2015-4-21
    //tp_type = fts_read_tp_type();//read tp type: OFILM or YEJI or BOEN
    tp_type = vendorID;
    //[END]

    //[BEGIN] liushilong@yulong.com display tp color info 2015-7-6
    tp_number[0] = fts_getColorANISC();
    tp_number[1] = tp_number[2] = tp_number[3] = 0;
    //[END]

    CPD_DBG(KERN_ERR "[FST] tp_id = 0x%x\n", tp_type);
    retry_c--;
    if(tp_type==0xff)
    {
        if(retry_c>0)
        goto tw_retry_id;
        //printk(KERN_ERR "Can't get TP type, so bad!!!!!\n");
        //return -1;
    }
    if(retry_c <= 0)
    {
        CPD_DBG("..func:%s read tp type error!!!\n",__func__);
        mutex_unlock(&tw_mutex);
        return -1 ;
    }

    //shihaobin@yulong.com modify firmware info begin 20150710
    if(tp_type == TW_OFILM_ID)
    {
        mutex_unlock(&tw_mutex);
        sprintf(buf, "%s:%s:%s%x\n","OFILM:FT5446",((tp_number[0]==0x31) ? "White" : "Gold"),"0x",firmware_version); //Display the right firmware version
    }
    else if(tp_type == TW_YEJI_ID)
    {
        mutex_unlock(&tw_mutex);
        sprintf(buf, "%s:%s:%s%x\n","YEJI:FT5446",((tp_number[0]==0x31) ? "White" : "Gold"),"0x",firmware_version); //Display the right firmware version
    }
    else if(tp_type == TW_BOEN_ID)
    {
        mutex_unlock(&tw_mutex);
        sprintf(buf, "%s:%s:%s%x\n","BOEN:FT5446",((tp_number[0]==0x31) ? "White" : "Gold"),"0x",firmware_version); //Display the right firmware version
    }
    else
    {
        CPD_DBG("Don't support kinds of TW chip : 0x%02x!!!!\n", tp_type);
        sprintf(buf, "TW: unknown\n");
        mutex_unlock(&tw_mutex);
    }
    //shihaobin@yulong.com modify firmware info end 20150710

    //[Begin] liushilong@yulong.com open touchscreen device info 2015-5-22
    get_device_info(buf);
    //[End]

    return 0;
}




//Add FTS HAL Function Entry for Factory Mode Test
/**************************************************************************
 ******************add another function get firmware version **************
**************************************************************************/
int ft5x46_get_firmware_version(char *buf)
{
    unsigned char firmware_version;
    unsigned char tp_type;
    //char firmware_version_buf[0xff];
    int retry_c=30;
    CPD_DBG("Enter %s\n",__FUNCTION__);

    mutex_lock(&tw_mutex);
    while(retry_c > 0)/** reading firmware version has 30 times chances**/
    {
        firmware_version = fts_read_fw_ver();
        //CPD_DBG("...Firmware version = 0x%x\n",firmware_version);
        printk("[FST]...Firmware version = 0x%x\n",firmware_version);
        retry_c--;
        if(firmware_version != 0xff)
        {
            break;
        }
    }
    if(retry_c <= 0)
    {
        CPD_DBG("..func:%s read version error!!!\n",__func__);
        mutex_unlock(&tw_mutex);
        return -1 ;
    }

tw_retry_id:
    retry_c=30;

    //[BEGIN] liushilong@yulong.com read boot area for get right vender id 2015-4-21
    //tp_type = fts_read_tp_type();//read tp type: OFILM or YEJI or BOEN
    tp_type = vendorID;
    //[END]

    //[BEGIN] liushilong@yulong.com display tw color info 2015-7-6
    tp_number[0] = fts_getColorANISC();
    tp_number[1] = tp_number[2] = tp_number[3] = 0;
    //[END]

    //CPD_DBG(KERN_ERR "[FST] tp_id = 0x%x\n", tp_type);
    printk("[FST] tp_id = 0x%x\n", tp_type);
    retry_c--;

    if(tp_type==0xff)
    {
        if(retry_c>0)
        goto tw_retry_id;
        //printk(KERN_ERR "Can't get TP type, so bad!!!!!\n");
        //return -1;
    }

    if(retry_c <= 0)
    {
        CPD_DBG("..func:%s read tp type error!!!\n",__func__);
        mutex_unlock(&tw_mutex);
        return -1 ;
    }

    //shihaobin@yulong.com modify firmware info begin 20150710
    if(tp_type == TW_OFILM_ID)
    {
        mutex_unlock(&tw_mutex);
        return sprintf(buf, "%s:%s:%s%x","OFILM:FT5446",((tp_number[0]==0x31) ? "White" : "Gold"),"0x",firmware_version); //Display the right firmware version
    }
    else if(tp_type == TW_YEJI_ID)
    {
        mutex_unlock(&tw_mutex);
        return sprintf(buf, "%s:%s:%s%x","YEJI:FT5446",((tp_number[0]==0x31) ? "White" : "Gold"),"0x",firmware_version); //Display the right firmware version
    }
    else if(tp_type == TW_BOEN_ID)
    {
        mutex_unlock(&tw_mutex);
        return sprintf(buf, "%s:%s:%s%x","BOEN:FT5446",((tp_number[0]==0x31) ? "White" : "Gold"),"0x",firmware_version); //Display the right firmware version
    }
    else
    {
        CPD_DBG("Don't support the kinds of TW chip : 0x%02x!!!!\n", tp_type);
        mutex_unlock(&tw_mutex);
    }
    //shihaobin@yulong.com modify firmware info end 20150710


    return -1; //the AP read the inode failed!!!!!
 }


/***********************************************************************
***********add another function check if firmware need update***********
***********************************************************************/
int ft5x46_firmware_need_update()
{

    unsigned char firmware_version;
    unsigned char firmware_version_from_i_file;
    char firmware_version_buf[0xff];
    int ret;

    CPD_DBG("enter %s\n",__FUNCTION__);
    if((ret=ft5x46_get_firmware_version(firmware_version_buf))<0)
    {
        CPD_DBG("the AP open the inode failed!!!!\n");
        return -1;
    }
    else
    {
        firmware_version = fts_read_fw_ver();
    }

    CPD_DBG("[FT]:firmware version = 0x%x\n", firmware_version);
    firmware_version_from_i_file = fts_get_upg_ver();

    if(firmware_version_from_i_file==0xFF)
    {
        CPD_DBG("Can't get rightly the firmware version from i file, so bad!!!!!\n");
        return -1;
    }
    CPD_DBG("[FT]:firmware_version_from_i_file = 0x%x\n",firmware_version_from_i_file);

    if( firmware_version < firmware_version_from_i_file)
    {
        CPD_DBG("[FT]: fts updates FW right now, please!!!!!\n");
        return 1;
    }
    else
    {
        CPD_DBG("[FT]: fts doesn't need to update FW now!!!!!\n");
        return 0;
    }
}

extern int fts_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
extern int fts_5x46_ctpm_fw_upgrade(struct i2c_client *client, u8* pbt_buf, u32 dw_lenth);

int fts_ctpm_fw_upgrade_with_i_file(int tp)
{
    u8* pbt_buf = FTS_NULL;
    int i_ret = -1;
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(0);apk_debug_flag = 1;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729
    //[BEGIN] liushilog@yulong.com when firmware update, it shoule close watchdog for avoiding reboot 2015-5-12
    mtk_wdt_enable(WK_WDT_DIS);
    //=========FW upgrade========================*/
    //[BEGIN] liushilong@yulong.com diff golden and white module for TW firmware update 2015-7-3
    if(TW_OFILM_ID == tp)
    {
        printk("inter OFILM TW update\n");

        if(FALSE == real_isGolden)
        {
            pbt_buf = OFILM_CTPM_FW000;
            i_ret = fts_5x46_ctpm_fw_upgrade(i2c_client,pbt_buf,sizeof(OFILM_CTPM_FW000));
        } else {
            pbt_buf = OFILM_CTPM_FW001;
            i_ret = fts_5x46_ctpm_fw_upgrade(i2c_client,pbt_buf,sizeof(OFILM_CTPM_FW001));
        }

        if (i_ret != 0)
        {
            printk(KERN_ERR "upgrade OFILM firmware failed\n");
        }
    }
    else if(TW_YEJI_ID == tp)
    {
        printk("inter yeji TW update\n");

        if(FALSE == real_isGolden)
        {
            pbt_buf = YEJI_CTPM_FW000;
            i_ret =  fts_5x46_ctpm_fw_upgrade(i2c_client,pbt_buf,sizeof(YEJI_CTPM_FW000));
        } else {
            pbt_buf = YEJI_CTPM_FW001;
            i_ret =  fts_5x46_ctpm_fw_upgrade(i2c_client,pbt_buf,sizeof(YEJI_CTPM_FW001));
        }

        if (i_ret != 0)
        {
            printk(KERN_ERR "upgrade YEJI firmware failed\n");
        }
    }
    else if(TW_BOEN_ID == tp)
    {
        printk("inter boen TW update\n");

        if(FALSE == real_isGolden)
        {
            pbt_buf = BOEN_CTPM_FW000;
            i_ret = fts_5x46_ctpm_fw_upgrade(i2c_client,pbt_buf,sizeof(BOEN_CTPM_FW000));
        } else {
            pbt_buf = BOEN_CTPM_FW001;
            i_ret = fts_5x46_ctpm_fw_upgrade(i2c_client,pbt_buf,sizeof(BOEN_CTPM_FW001));
        }

        if (i_ret != 0)
        {
            printk(KERN_ERR "upgrade BOEN firmware failed\n");
        }
    }//[END]
    else
    {
        printk(KERN_ERR "unknow tw id: 0x%02x\n", tp);
        return -1;
    }
    mtk_wdt_enable(WK_WDT_EN);
    //[END]
    CPD_DBG(KERN_ERR "FT UPDATA OK-----\n");
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        esd_switch(1);apk_debug_flag = 0;
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729
    return i_ret;
}

extern int fts_ctpm_auto_clb(struct i2c_client *client);
static int fts_firmware_upgrade_bill(void)
{
    unsigned char tp_type = TW_OFILM_ID; /*default is OFILM*/
    int ret = -1 ;
    int upgrade_times = 4 ;
    int i;

    //[BEGIN] liushilong@yulong.com modify BOEN ID for calibrate 2015-4-22
    for(i=0;i<10;i++)
    {
        //[BEGIN] liushilong@yulong.com read boot area for get right vender id 2015-4-21
        //tp_type = fts_read_tp_type();//read tp type: OFILM or YEJI or BOEN
        tp_type = vendorID;
        //[END]

        if((0x60 == tp_type)||(0xa6 == tp_type)||(0xff == tp_type))
            break;

        mdelay(20);
    }

    if( TW_OFILM_ID == tp_type || TW_YEJI_ID == tp_type || TW_BOEN_ID == tp_type)
    {
        while(upgrade_times > 0)
        {
            if(!fts_ctpm_fw_upgrade_with_i_file(tp_type))
            {
                ret = 0 ;
                //zouxin add begin 120119
                fts_ctpm_auto_clb(i2c_client); //auto calibration after upgrade succeedfully
                //zouxin add end 120119
                break ;
            }
            upgrade_times--;
        }

        if(!ret)
        {
            calibrate_flag = 0 ;
            return 0 ;
        }
        else
        {
            calibrate_flag = 1 ;
            return -1 ;
        }
    }
    else
    {
        CPD_DBG("read tw vendor chip id failed: 0x%02x !!!!\n", tp_type);
        calibrate_flag = 1 ;
        return -1 ;
    }
//[END]
}


ssize_t ft5x46_store_tw_upgrade_bill()
{
    int ret = -1 ;

    ret = fts_firmware_upgrade_bill();
    if(!ret)
    {
        CPD_DBG("..func:%s..firmware upgrade successful..\n",__FUNCTION__);
        mutex_unlock(&tw_mutex);
        //return count ;
        return 0; // zouxin changed 120119 upgrade successfully
    }
    else
    {
        CPD_DBG("..func:%s execute firmware_upgrade function failed!!\n",__func__);
        mutex_unlock(&tw_mutex);
        return -1;
    }
}

ssize_t ft5x46_show_tw_calibrate_bill()
{
    mutex_lock(&tw_mutex);
    CPD_DBG("Enter %s\n function:", __FUNCTION__);
    CPD_DBG("calibrate_flag=%d\n", calibrate_flag);
    if(!calibrate_flag)
    {
        mutex_unlock(&tw_mutex);
        //return sprintf(buf, "%s\n", "1");/**need to calibrate,write 0**/
        return 1; //zouxin modify 120119
    }
    else
    {
        mutex_unlock(&tw_mutex);
        //return sprintf(buf, "%d\n", "0");/**don't need to calibrate,write 1**/
        return 0; //zouxin  modify 120119
    }
    //return 0 ;
}

ssize_t ft5x46_store_tw_calibrate_bill()
{
    fts_ctpm_auto_clb(i2c_client); //auto calibration start
    mutex_unlock(&tw_mutex);
    CPD_DBG("..func:%s..calibrate successful....\n",__FUNCTION__);
    //return count ;
    return 0;//zouxin changed calibrtaion successfully
}


/*******************************************************
  ******************system write "reset"***************
*******************************************************/
int ft5x46_reset_touchscreen(void)
{
    CPD_DBG(KERN_ERR "enter %s\n",__FUNCTION__);

    /*write 0xaa to register 0xfc*/
    fts_write_reg(i2c_client,0xfc,0xaa);
    delay_qt_ms(50);
    /*write 0x55 to register 0xfc*/
    fts_write_reg(i2c_client,0xfc,0x55);
    CPD_DBG(KERN_ERR "[FTS]: Reset CTPM ok!\n");

    return 1;
}


touch_mode_type ft5x46_get_mode(void)
{
    return current_mode;
}

int ft5x46_set_mode(touch_mode_type work_mode)
{
    u8 ret=0;

    if (i2c_client == NULL)
    {
        return -1;
    }

    if(work_mode == MODE_GLOVE)
    {
        printk(KERN_ERR"[FTS]:switch to glove mode.\n");
        #if TW_GLOVE_SWITCH
        current_mode = 3;
        //To do
        fts_write_reg(i2c_client, ID_G_GLOVE_MODE_EN , 0x01);   //open glove switch
        glove_switch = 1;   //set glove status is open
        #endif
        //shihaobin@yulong.com add for close window mode when hall state is zero begin 20150731
        #if TW_WINDOW_SWITCH
            fts_write_reg(i2c_client, ID_G_COVER_MODE_EN, 0x00);
            atomic_set(&fts_cover_window.windows_switch, 0);
        #endif
        //shihaobin@yulong.com add for close window mode when hall state is zero end 20150731
    }
    else if( work_mode == MODE_WINDOW )
    {
        printk(KERN_ERR"[FTS]:switch to window mode.\n");
#if TW_WINDOW_SWITCH
        //To do
        current_mode = 4;

        //open cover mode
        #if TW_GLOVE_SWITCH
        if (1 == glove_switch)
        {
            fts_write_reg(i2c_client, ID_G_GLOVE_MODE_EN, 0x01);
            fts_write_reg(i2c_client, ID_G_COVER_MODE_EN, 0x01);
            atomic_set(&fts_cover_window.windows_switch, 1);
            printk("[FTS]:switch to window mode and glove mode.\n");
        }
        else
        {
            fts_write_reg(i2c_client, ID_G_COVER_MODE_EN, 0x01);
            atomic_set(&fts_cover_window.windows_switch, 1);
            printk("[FTS]:1/only switch to window mode.\n");
        }
        #else
        fts_write_reg(i2c_client, ID_G_COVER_MODE_EN, 0x01);
        atomic_set(&fts_cover_window.windows_switch, 1);
        printk("[FTS]:2/only switch to window mode.\n");
        #endif

        //set window size
        //shihaobin@yulong.com delete for close window in driver begin 20150729
        #if 0
        {
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_LEFT, &fts_cover_window.win_x_min);
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_RIGHT, &fts_cover_window.win_x_max);
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_UP,  &fts_cover_window.win_y_min);
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_DOWN, &fts_cover_window.win_y_max);
        }
        #endif
        //shihaobin@yulong.com delete for close window in driver end 20150729
#endif
    }
    else
    {
        printk(KERN_ERR"[FTS]:switch to normal mode.\n");
        //To do
        current_mode = 1;
        //open cover mode
        fts_write_reg(i2c_client, ID_G_COVER_MODE_EN, 0x00);    //close window mode
        fts_write_reg(i2c_client, ID_G_GLOVE_MODE_EN, 0x00);    //close glove mode
        atomic_set(&fts_cover_window.windows_switch, 0);
        glove_switch = 0;       //current glove is closed
    }

    printk(KERN_ERR"%s: mode  = %d\n", __func__,work_mode);
    return 1;
}

#ifdef TW_WINDOW_SWITCH
void window_mode(bool on)
{
    if(on)
    {
        ft5x46_set_mode(MODE_WINDOW);
    }
    else
    {
        if(1 == glove_switch)
        {
            ft5x46_set_mode(MODE_GLOVE);   //if glove switch is open ,open glove mode
        }
        else if(0 == glove_switch)
        {
            ft5x46_set_mode(MODE_NORMAL);  //if glove switch is closed, open normal mode
        }
    }
}

int hall_state_notify_touch(int in_hall)
{
    if(1 == YL_I2C_STATUS)
    {
        if(in_hall)
        {
            window_mode(true);
        }
        else
        {
            window_mode(false);
        }
        return 0;
    }
    else
    {
        return -1;
    }
}
#else
void window_mode(bool on)
{
    CPD_DBG("Not support window mode.\n");
}

int hall_state_notify_touch(int in_hall)
{
    CPD_DBG("Not support window switch.\n");
    return 0;
}
#endif
EXPORT_SYMBOL_GPL(hall_state_notify_touch);


touch_oreitation_type ft5x46_get_oreitation(void)
{
    CPD_DBG("enter %s\n",__FUNCTION__);
    return 1;
}


int ft5x46_set_oreitation(touch_oreitation_type oreitate)
{
    CPD_DBG("enter %s\n",__FUNCTION__);
    return 1;
}


/*******************************************************
  **********write buf[256]******************************
*******************************************************/
int ft5x46_write_regs(const char * buf)
{
    char *start = (char *)buf;
    unsigned long reg,reg1;
    unsigned long value;

    CPD_DBG("enter %s\n",__FUNCTION__);

    if(NULL == buf)
    {
        CPD_DBG(KERN_ERR "ft5446_tw:ft5446_Write: Invalid Parameter\r\n");
        return -1;
    }

    reg = simple_strtoul((const char *)start, &start, 16);

    while (*start == ' ')
        start++;

    reg1=strict_strtoul((const char *)start, 16, &value);
    CPD_DBG("0x%2x-->0x%2x\n",(u8)reg,(u8)value);
    i2c_smbus_write_byte_data(i2c_client,(u8)reg,(u8)value);

    return 1;
}

/*******************************************************
  *************** debug on or off *************
*******************************************************/
int ft5x46_debug(int val)
{
    printk("Enter %s\n",__FUNCTION__);
    cpd_debug=val;
    return 1;
}

//[Begin] liushilong@yulong.com modify vendor name for match tw shorttest and fastmmi test 2015-5-22
int focaltech_vendor(char* vendor)
{
    return sprintf(vendor, "%s", "focaltech");
}
//[End]


#ifdef FTS_GESTRUE
int ft5x46_get_wakeup_gesture(char*  gesture)
{
    return sprintf(gesture, "%s", (char *)wakeup_slide);
}

int ft5x46_gesture_ctrl(const char*  gesture_buf)
{
    char *gesture;
    int buf_len;
    char *gesture_p;
    char *temp_p;


    buf_len = strlen(gesture_buf);


    CPD_DBG("%s buf_len = %d.\n", __func__, buf_len);
    CPD_DBG("%s gesture_buf:%s.\n", __func__, gesture_buf);

    gesture_p = kzalloc(buf_len + 1, GFP_KERNEL);
    if(gesture_p == NULL)
    {
        CPD_DBG("%s: alloc mem error.\n", __func__);
        return -1;
    }
    temp_p = gesture_p;
    strlcpy(gesture_p, gesture_buf, buf_len + 1);

    while(gesture_p)
    {
        gesture = strsep(&gesture_p, ",");
        CPD_DBG("%s gesture:%s.\n", __func__, gesture);

        if (!strncmp(gesture, "up=",3))
        {
            if(!strncmp(gesture+3, "true", 4))
            {
                CPD_DBG("%s: enable up slide wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_UP_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+3, "false", 5))
            {
                CPD_DBG("%s: disable up slide wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_UP_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "down=",5))
        {
            if(!strncmp(gesture+5, "true", 4))
            {
                CPD_DBG("%s: enable down slide wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_DOWN_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+5, "false", 5))
            {
                CPD_DBG("%s: disable down slide wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_DOWN_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "left=",5))
        {
            if(!strncmp(gesture+5, "true", 4))
            {
                CPD_DBG("%s: enable left slide wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_LEFT_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+5, "false", 5))
            {
                CPD_DBG("%s: disable left slide wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_LEFT_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "right=",6))
        {
            if(!strncmp(gesture+6, "true", 4))
            {
                CPD_DBG("%s: enable right slide wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_RIGHT_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+6, "false", 5))
            {
                CPD_DBG("%s: disable right slide wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_RIGHT_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "double_click=",13))
        {
            if(!strncmp(gesture+13, "true", 4))
            {
                CPD_DBG("%s: enable double click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_DOUBLE_CLICK_WAKEUP;
            }
            else if(!strncmp(gesture+13, "false", 5))
            {
                CPD_DBG("%s: disable double click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_DOUBLE_CLICK_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "e=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable e click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_E_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable e click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_E_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "o=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable o click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_O_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable o click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_O_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "w=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable w click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_W_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable w click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_W_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "c=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable c click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_C_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable c click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_C_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "m=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable m click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_M_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable m click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_M_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "l=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable l click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_L_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable l click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_L_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "s=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable s click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_S_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable s click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_S_SLIDE_WAKEUP;
            }
            continue;
        }

        if (!strncmp(gesture, "v=",2))
        {
            if(!strncmp(gesture+2, "true", 4))
            {
                CPD_DBG("%s: enable v click wakeup func.\n", __func__);
                support_gesture |= TW_SUPPORT_V_SLIDE_WAKEUP;
            }
            else if(!strncmp(gesture+2, "false", 5))
            {
                CPD_DBG("%s: disable v click wakeup func.\n", __func__);
                support_gesture &= ~TW_SUPPORT_V_SLIDE_WAKEUP;
            }
            continue;
        }
    }
    kfree(temp_p);
    return 1;
}
#endif

touchscreen_ops_tpye ft5x46_ts_ops={
    .touch_id=0,
    .touch_type=1,
    .active=ft5x46_active,
    .firmware_need_update=ft5x46_firmware_need_update,
    .firmware_do_update=ft5x46_store_tw_upgrade_bill,
    .need_calibrate=ft5x46_show_tw_calibrate_bill,
    .calibrate=ft5x46_store_tw_calibrate_bill,
    .get_firmware_version=ft5x46_get_firmware_version,
    .reset_touchscreen=ft5x46_reset_touchscreen,
    .get_mode=ft5x46_get_mode,
    .set_mode=ft5x46_set_mode,
    .get_oreitation=ft5x46_get_oreitation,
    .set_oreitation=ft5x46_set_oreitation,
    .write_regs=ft5x46_write_regs,
    .debug=ft5x46_debug,
    .get_vendor = focaltech_vendor,
#ifdef FTS_GESTRUE   ///add by mxg
    .get_wakeup_gesture = ft5x46_get_wakeup_gesture,
    .gesture_ctrl = ft5x46_gesture_ctrl,
 #endif
};


unsigned char read_tp_bootloader_version(void)
{
    u8  reg_val[50] = {0};
    u8  reg_val_color[10] = {0};//liushilong@yulong.com add for save 0xBA,0xBB,0xBC register for TW color 2015-7-4
    u32 i = 0;
    u32 j = 0;
    u8  auc_i2c_write_buf[10];
    int i_ret;
    unsigned char uc_panel_factory_id[2]= {0};  //liushilong@yulong.com store different vendorID for choosing 2015-5-12
    unsigned char au_delay_timings[11] = {30,33,36,39,42,45,27,24,21,18,15};

UPGR_START:
    /*write 0xaa to register 0xfc*/
    //this_client->timing = 100; //i2c setting 100
    printk(" ********read_tp_bootloader_version start***********\n");

#if 0
    //Hardware Reset
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    mdelay(5);//30);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    delay_qt_ms(au_delay_timings[j]);
#endif

    //[BEGIN] liushilong@yulong.com modify shakehands protocol for ft5446 2015-4-21
    //write 0xaa to register 0xfc
    fts_write_reg(i2c_client, 0xfc, FTS_UPGRADE_AA);
    msleep(fts_updateinfo_curr.delay_aa);

    //write 0x55 to register 0xfc
    fts_write_reg(i2c_client, 0xfc, FTS_UPGRADE_55);
    msleep(200);

    auc_i2c_write_buf[0] = 0xeb;
    auc_i2c_write_buf[1] = 0xaa; 
    auc_i2c_write_buf[2] = 0x09; 
    fts_i2c_Read(i2c_client, auc_i2c_write_buf, 3, reg_val, 3);

    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do
    {
        i++;
        i_ret = fts_i2c_Write(i2c_client, auc_i2c_write_buf, 2);
        delay_qt_ms(5);
        printk("[FTS]i_ret = %d, i = %d\n",i_ret, i);
    } while(i_ret <= 0 && i < 5 );

    mdelay(200);
    auc_i2c_write_buf[0] = 0x90;
    auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;
    fts_i2c_Read(i2c_client, auc_i2c_write_buf, 1, reg_val, 3);
    printk( "[FTS] 0.:reg_val[0] = 0x%x, reg_val[1] = 0x%x\n",reg_val[0], reg_val[1]);

    if ( reg_val[0] == 0x54 && reg_val[1] == 0x2c )
    {
        printk("[FTS] 1.read_tp_bootloader_version reg_val[0] = 0x%x,reg_val[1] = 0x%x\n",reg_val[0],reg_val[1]);
    }
    else
    {
        if (j < 10)
        {
            j++;
            msleep(200);
            printk("[FTS] 2.read_tp_bootloader_version reg_val[0] = 0x%x,reg_val[1] = 0x%x\n",reg_val[0],reg_val[1]);
            printk("j = %d\n",j);
            printk("[FTS]goto UPGR_START!\n");
            goto UPGR_START;
        }
        else
        {
            printk("error read reg_val[0] = 0x%x,reg_val[1] = 0x%x\n",reg_val[0],reg_val[1]);
            return -1;
        }
    }

    if ( reg_val[1] == 0x2c )
    {
        auc_i2c_write_buf[0]=0x03;
        auc_i2c_write_buf[1]=0x00;
        auc_i2c_write_buf[2]=0xD7;
        auc_i2c_write_buf[3]=0x84;

        fts_i2c_Read(i2c_client, auc_i2c_write_buf, 4, reg_val, 5);
        printk( "[FTS] : reg_val[0] = 0x%x,reg_val[1] = 0x%x, reg_val[2] = 0x%x,reg_val[3] = 0x%x,reg_val[4] = 0x%x\n",reg_val[0], reg_val[1],reg_val[2],reg_val[3],reg_val[4]);

        //liushilong@yulong.com when it read error, it should be write 0xcd 2015-5-12
        uc_panel_factory_id[0]=reg_val[4];
    }
    //[END]

    //[BEGIN] liushilong@yulong.com add diff color module ID for golden and white phone
    auc_i2c_write_buf[0]=0x03;
    auc_i2c_write_buf[1]=0x00;
    auc_i2c_write_buf[2]=0xD7;
    auc_i2c_write_buf[3]=0xC2;
    fts_i2c_Read(i2c_client, auc_i2c_write_buf, 4, reg_val_color, 3);
    printk( "[FTS] color : BA = 0x%x, BB = 0x%x, BC = 0x%x\n",reg_val_color[0], reg_val_color[1],reg_val_color[2]);
    if(0x31 == reg_val_color[2])
    {
        real_isGolden = FALSE;
    }
    else if (0x47 == reg_val_color[2])
    {
        real_isGolden = TRUE;
    }

    printk("[FTS] current TW module color is golden = 0x%x\n", real_isGolden);
    //[END]

    if(strcmp(reg_val,"000")==0)focaltech_type=0;
    if(strcmp(reg_val,"829")==0)focaltech_type=1;
    printk("focaltech_type==%d\n",focaltech_type);

    auc_i2c_write_buf[0] = 0xcd;
    fts_i2c_Read(i2c_client, auc_i2c_write_buf, 1, reg_val, 1);
    printk("read_tp_bootloader_version bootloader version = 0x%x\n", reg_val[0]);
    if(0x30 == reg_val[0])
    {
        uc_panel_factory_id[1] = reg_val[3];
    }
    else
    {
        uc_panel_factory_id[1] = reg_val[0];
    }

    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    mdelay(5);//30);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    mdelay(100);

    printk(" ********read_tp_bootloader_version end***********\n");

    return (0xff != uc_panel_factory_id[0]) ? uc_panel_factory_id[0] : uc_panel_factory_id[1];
    //[END]
}


void write_MAX_FRAME_reg(unsigned char val)
{
    unsigned char ver = 0;
    fts_write_reg(i2c_client,FTS_REG_MAX_FRAME,val);
    // i2c_smbus_read_i2c_block_data(i2c_client, FTS_REG_MAX_FRAME, 1, &ver);
    //printk("write_MAX_FRAME_reg =0x%x\n",ver);
}
EXPORT_SYMBOL(write_MAX_FRAME_reg);


static int fts_init_gpio_hw(void)
{

    int ret = 0;
    int i = 0;

    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

    return ret;
}


static int __init tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int  retval = TPD_OK;
    char data;
    u8   report_rate=0;
    int  err=0;
    int  reset_count = 0;
    int  idx;
    u8   chipid = 0;
    int  boot_mode;

    client->timing = 100;
    i2c_client = client;

reset_proc:

    //power on, need confirm with SA
    //shihaobin@yulong.com modify for work time 20150802 begin
    /*#ifdef TPD_POWER_SOURCE_CUSTOM
        hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
    #else
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    #endif

    #ifdef TPD_POWER_SOURCE_1800
        hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
    #endif*/

    //reset tp
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(5);
    
    #ifdef TPD_POWER_SOURCE_CUSTOM
    hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
    #else
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    #endif

    #ifdef TPD_POWER_SOURCE_1800
        hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
    #endif
    msleep(10);
    TPD_DMESG(" ft5446 reset\n");
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    //shihaobin@yulong.com modify for work time 20150802 end

#ifdef FTS_GESTRUE
    input_set_capability(tpd->dev, EV_KEY,KEY_POWER);
#endif

#if FTS_HAVE_TOUCH_KEY
    for (idx = 0; idx < FTS_MAX_KEY_NUM; idx++)
    {
        input_set_capability(tpd->dev, EV_KEY, touch_key_array[idx]);
    }
#endif

    //set interrupt
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

    //mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
    //mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
    mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

    msleep(150);

    if (fts_dma_buffer_init() < 0)
    {
        return -1;
    }

    if((i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &data))< 0)
    {

        YL_I2C_STATUS = 0;

        TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);

        fts_dma_buffer_deinit();

#ifdef TPD_RESET_ISSUE_WORKAROUND
        if ( reset_count < TPD_MAX_RESET_COUNT )
        {
            reset_count++;
            goto reset_proc;
        }
#endif
        return -1;
    }
    else
    {
        YL_I2C_STATUS = 1;
    }

    tpd_load_status = 1;
    // tpd_mstar_status =0 ;  // compatible mstar and ft6306 chenzhecong

    focaltech_get_upgrade_array();

#ifdef FTS_APK_DEBUG
    ft5x0x_create_apk_debug_channel(client);
#endif

#ifdef TPD_SYSFS_DEBUG
    err=fts_create_sysfs(i2c_client);
    printk("fts_create_sysfs(i2c_client)=%d\n",err);
#endif

#ifdef FTS_CTL_IIC
    if (ft_rw_iic_drv_init(i2c_client) < 0)
        TPD_DMESG("%s:[FTS] create fts control iic driver failed\n",__func__);
#endif

#ifdef VELOCITY_CUSTOM_FT5206
    if((err = misc_register(&tpd_misc_device)))
    {
        printk("mtk_tpd: tpd_misc_device register failed\n");
    }
#endif

    //[BEGIN] liushilong@yulong.com forbid load TW driver when off-charge mode and recovery mode 2015-6-16
    boot_mode = get_boot_mode();
    if((KERNEL_POWER_OFF_CHARGING_BOOT == boot_mode) || (RECOVERY_BOOT == boot_mode))
    {
        printk(" Exit ft5446 driver load.\n");
        return 0;
    }
    //[END]

    int firmware_version;
    int firmware_version_from_i_file;
    unsigned char bootloader_ID;
    unsigned char colorMatch;

    firmware_version = fts_read_fw_ver();
    printk("TP-cyd:firmware_version = 0x%x\n",firmware_version);

    //[BEGIN] liushilong@yulong.com get current kernel TW color info 2015-7-6
    currentColor = fts_isGolden();
    printk("TP-cyd current TW color is golden = 0x%x\n", currentColor);
    //[END]

    //[BEGIN] change vender id read method and add boen 0x3B firmware update
    bootloader_ID = read_tp_bootloader_version();
    vendorID = bootloader_ID;

    //bootloader_ID = fts_read_tp_type();
    printk("TP-cyd:bootloader_ID=%x\n",bootloader_ID);

    firmware_version_from_i_file = fts_get_upg_ver();
    printk("TP-cyd:firmware_version_from_i_file = 0x%x\n",firmware_version_from_i_file);

    //[BEGIN] liushilong@yulong.com get current kernel TW color info and whether it match  2015-7-6
    colorMatch = real_isGolden^currentColor;
    printk("TP-cyd:colorMatch = %x\n", colorMatch);
    //[END]

    //[BEGIN]liushilong@yulong.com modify firmware update for color match 2015-7-6
#if 1
    printk("TP-cyd:firmware update start\n");

    if(1 == colorMatch)
    {
        //if kernel TW firmware is not match with TW read color, it should change
        if ((bootloader_ID == 0x51) || (bootloader_ID == TW_OFILM_ID)) {
            printk("TP-sxb: color not match, it should force updating ofilm firmware\n");
            fts_ctpm_fw_upgrade_with_i_file(0x51);
        } else if ((bootloader_ID == 0x80) || (bootloader_ID == TW_YEJI_ID)) {
            printk("TP-sxb: color not match, it should force updating yeji firmware\n");
            fts_ctpm_fw_upgrade_with_i_file(0x80);
        } else if ((bootloader_ID == 0x3B) || (bootloader_ID == TW_BOEN_ID)) {
            printk("TP-sxb: color not match, it should force updating boen firmware\n");
            fts_ctpm_fw_upgrade_with_i_file(0x3B);
        } else {
          printk("TP-sxb: No Bootloader_ID can match for update\n");
        }
    }
    else
    {
        //if if kernel TW firmware is match with TW read color,update directly
        if ((firmware_version < firmware_version_from_i_file) || (0x0 == firmware_version)) {
            if ((bootloader_ID == 0x51) || (bootloader_ID == TW_OFILM_ID)) {
                printk("TP-sxb: enter ofilm tp fw version check for update\n");
                fts_ctpm_fw_upgrade_with_i_file(0x51);
            } else if ((bootloader_ID == 0x80) || (bootloader_ID == TW_YEJI_ID)) {
                printk("TP-sxb: enter yeji tp fw version check for update\n");
                fts_ctpm_fw_upgrade_with_i_file(0x80);
            } else if ((bootloader_ID == 0x3B) || (bootloader_ID == TW_BOEN_ID)) {
                printk("TP-sxb: enter boen tp fw version check for update\n");
                fts_ctpm_fw_upgrade_with_i_file(0x3B);
            } else {
              printk("TP-sxb: No Bootloader_ID can match for update\n");
            }
        }
    }

    printk("TP-cyd:firmware update start end\n");
    tpd_charging_filter();//shihaobin@yulong.com add for judgement if charging or not 20150729
    //[END]

#endif

#ifdef TPD_AUTO_UPGRADE
    printk("********************Enter CTP Auto Upgrade********************\n");
    fts_ctpm_auto_upgrade(i2c_client);
#endif

    #ifdef CONFIG_COVER_WINDOW_SIZE
    atomic_set(&fts_cover_window.windows_switch, 0);

    ///default value
    //shihaobin@yulong.com delete for close window in driver begin 20150729
    #if 0
    {
        fts_cover_window.win_x_min = 0;
        fts_cover_window.win_y_min = 0;
        fts_cover_window.win_x_max = 540;
        fts_cover_window.win_y_max = 300;
    }
    #endif
    //shihaobin@yulong.com delete for close window in driver begin 20150729
    
    #endif

    //reset tp
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(5);
    TPD_DMESG(" ft5446 reset\n");
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    msleep(50);

    touchscreen_set_ops(&ft5x46_ts_ops);
    thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
    if (IS_ERR(thread))
    {
        retval = PTR_ERR(thread);
        TPD_DMESG(" failed to create kernel thread: %d\n", retval);
    }

#ifdef USB_CHARGE_DETECT
    INIT_DELAYED_WORK(&Judge_tp_delayed_work,judge_tp_normal_delayed_work);
#endif

#ifdef TPD_PROXIMITY
    struct hwmsen_object obj_ps;

    obj_ps.polling = 0;//interrupt mode
    obj_ps.sensor_operate = tpd_ps_operate;
    if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
    {
        APS_ERR("proxi_fts attach fail = %d\n", err);
    }
    else
    {
        APS_ERR("proxi_fts attach ok = %d\n", err);
    }
#endif

    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
        gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
        queue_delayed_work(gtp_esd_check_workqueue,&gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    TPD_DMESG("FTS Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

    return 0;
}

static int tpd_remove(struct i2c_client *client)
{
#ifdef FTS_APK_DEBUG
    ft5x0x_release_apk_debug_channel();
#endif

#ifdef FTS_CTL_IIC
    ft_rw_iic_drv_exit();
#endif

#ifdef SYSFS_DEBUG
    fts_release_sysfs(client);
#endif
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        destroy_workqueue(gtp_esd_check_workqueue);
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729

    fts_dma_buffer_deinit();

    TPD_DEBUG("TPD removed\n");
    return 0;
}
//shihaobin@yulong.com modify for EDS protect begin 20170729
#if FT_ESD_PROTECT
void esd_switch(s32 on)
{
    //spin_lock(&esd_lock);
    if (1 == on) // switch on esd
    {
       // if (!esd_running)
       // {
       //     esd_running = 1;
            //spin_unlock(&esd_lock);
            //printk("\n zax Esd started \n");
            queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
        //}
        //else
        //{
         //   spin_unlock(&esd_lock);
        //}
    }
    else // switch off esd
    {
       // if (esd_running)
       // {
         //   esd_running = 0;
            //spin_unlock(&esd_lock);
            //printk("\n zax Esd cancell \n");
            cancel_delayed_work(&gtp_esd_check_work);
       // }
       // else
       // {
        //    spin_unlock(&esd_lock);
        //}
    }
}
/************************************************************************
* Name: force_reset_guitar
* Brief: reset
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static void force_reset_guitar(void)
{
    s32 i;
    s32 ret;

    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(10);
    TPD_DMESG("force_reset_guitar\n");

    #ifdef TPD_POWER_SOURCE_CUSTOM
        hwPowerDown(TPD_POWER_SOURCE_CUSTOM, "TP");
    #else
        hwPowerDown(MT65XX_POWER_LDO_VGP2, "TP");
    #endif
    #ifdef TPD_POWER_SOURCE_1800
        hwPowerDown(TPD_POWER_SOURCE_1800, "TP");
    #endif

    msleep(200);
    #ifdef TPD_POWER_SOURCE_CUSTOM
        hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
    #else
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    #endif

    #ifdef TPD_POWER_SOURCE_1800
        hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
    #endif
    msleep(5);

    msleep(10);
    TPD_DMESG(" fts ic reset\n");
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

    msleep(300);

#ifdef TPD_PROXIMITY
    if (FT_PROXIMITY_ENABLE == tpd_proximity_flag)
    {
        tpd_enable_ps(FT_PROXIMITY_ENABLE);
    }
#endif
}


#define A3_REG_VALUE                                0x54
#define RESET_91_REGVALUE_SAMECOUNT                 5
static u8 g_old_91_Reg_Value = 0x00;
static u8 g_first_read_91 = 0x01;
static u8 g_91value_same_count = 0;
/************************************************************************
* Name: gtp_esd_check_func
* Brief: esd check function
* Input: struct work_struct
* Output: no
* Return: 0
***********************************************************************/
static void gtp_esd_check_func(struct work_struct *work)
{
    int i;
    int ret = -1;
    u8 data, data_old;
    u8 flag_error = 0;
    int reset_flag = 0;
    u8 check_91_reg_flag = 0;

    if (tpd_halt)
    {
        return;
    }
    if(apk_debug_flag)
    {
        //queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
        return;
    }

    run_check_91_register = 0;
    for (i = 0; i < 3; i++)
    {
        //ret = fts_i2c_smbus_read_i2c_block_data(i2c_client, 0xA3, 1, &data);
        ret = fts_read_reg(i2c_client, 0xA3,&data);
        if (ret<0) 
        {
            printk("[Focal][Touch] read value fail");
            //return ret;
        }
        if (ret==1 && A3_REG_VALUE==data)
        {
            break;
        }
    }

    if (i >= 3)
    {
        force_reset_guitar();
        printk("focal--tpd reset. i >= 3  ret = %d  A3_Reg_Value = 0x%02x\n ", ret, data);
        reset_flag = 1;
        goto FOCAL_RESET_A3_REGISTER;
    }

    //esd check for count
    //ret = fts_i2c_smbus_read_i2c_block_data(i2c_client, 0x8F, 1, &data);
    ret = fts_read_reg(i2c_client, 0x8F,&data);
    if (ret<0)
    {
        printk("[Focal][Touch] read value fail");
        //return ret;
    }
    printk("0x8F:%d, count_irq is %d\n", data, count_irq);

    flag_error = 0;
    if((count_irq - data) > 10)
    {
        if((data+200) > (count_irq+10))
        {
            flag_error = 1;
        }
    }

    if((data - count_irq ) > 10)
    {
        flag_error = 1;
    }

    if(1 == flag_error)
    {
        printk("focal--tpd reset.1 == flag_error...data=%d  count_irq=%d\n ", data, count_irq);
        force_reset_guitar();
        reset_flag = 1;
        goto FOCAL_RESET_INT;
    }

    run_check_91_register = 1;
    //ret = fts_i2c_smbus_read_i2c_block_data(i2c_client, 0x91, 1, &data);
    ret = fts_read_reg(i2c_client, 0x91,&data);
    if (ret<0)
    {
        printk("[Focal][Touch] read value fail");
        //return ret;
    }
    printk("focal---------91 register value = 0x%02x    old value = 0x%02x\n",  data, g_old_91_Reg_Value);
    if(0x01 == g_first_read_91)
    {
        g_old_91_Reg_Value = data;
        g_first_read_91 = 0x00;
    }
    else
    {
        if(g_old_91_Reg_Value == data)
        {
            g_91value_same_count++;
            printk("focal 91 value ==============, g_91value_same_count=%d\n", g_91value_same_count);
            if(RESET_91_REGVALUE_SAMECOUNT == g_91value_same_count)
            {
                force_reset_guitar();
                printk("focal--tpd reset. g_91value_same_count = 5\n");
                g_91value_same_count = 0;
                reset_flag = 1;
            }

            //run_check_91_register = 1;
            esd_check_circle = TPD_ESD_CHECK_CIRCLE / 2;
            g_old_91_Reg_Value = data;
        }
        else
        {
            g_old_91_Reg_Value = data;
            g_91value_same_count = 0;
            //run_check_91_register = 0;
            esd_check_circle = TPD_ESD_CHECK_CIRCLE;
        }
    }
FOCAL_RESET_INT:
FOCAL_RESET_A3_REGISTER:
    count_irq=0;
    data=0;
    //fts_i2c_smbus_write_i2c_block_data(i2c_client, 0x8F, 1, &data);
    ret = fts_write_reg(i2c_client, 0x8F,data);
    if (ret<0)
    {
        printk("[Focal][Touch] write value fail");
        //return ret;
    }
    if(0 == run_check_91_register)
    {
        g_91value_same_count = 0;
    }
    #ifdef TPD_PROXIMITY
    if( (1 == reset_flag) && ( FT_PROXIMITY_ENABLE == tpd_proximity_flag) )
    {
        if((tpd_enable_ps(FT_PROXIMITY_ENABLE) != 0))
        {
            APS_ERR("enable ps fail\n");
            return -1;
        }
    }
    #endif
    //end esd check for count

        if (!tpd_halt)
        {
            //queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
            queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
        }

        return;
}
#endif
//shihaobin@yulong.com modify for EDS protect end 20170729

static int tpd_local_init(void)
{
    TPD_DMESG("Focaltech FT5446 I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);

    if(i2c_add_driver(&tpd_i2c_driver)!=0)
    {
        TPD_DMESG("ft5446 unable to add i2c driver.\n");
        return -1;
    }

    if(tpd_load_status == 0)
    {
        TPD_DMESG("ft5446 add error touch panel driver.\n");
        i2c_del_driver(&tpd_i2c_driver);
        return -1;
    }

#ifdef TPD_HAVE_BUTTON
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);
#endif
    TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    //[BEGIN] liushilong@yulong.com mask get ft5446 info when touchscreen info 2015-6-4
    //fts_get_firmware_info();
    //[END]
    return 0;
}

static void tpd_resume( struct early_suspend *h )
{
    //int retval = TPD_OK;
    //char data;
    TPD_DMESG("TPD enter resume\n");
    printk("TPD enter resume\n");

#ifdef FTS_GESTRUE
    //u8 auc_i2c_write_buf[2] = {0};
    printk("resume doze_status = %d\n", doze_status);
    if(support_gesture & TW_SUPPORT_GESTURE_IN_ALL)
    {
        if(doze_status == DOZE_ENABLED)
        {
            TPD_GESTURE_DBG("resume doze_status == DOZE_ENABLED %d \n",__LINE__);
            printk("resume doze_status == DOZE_ENABLED %d \n",__LINE__);

            fts_write_reg(i2c_client,0xD0,0x00);
            doze_status = DOZE_DISABLED;
            TPD_GESTURE_DBG("enter normal\n");
            printk("enter normal\n");
        }
    }
#endif

    TPD_DMESG("TPD wake up\n");
    //shihaobin@yulong.com modify for work time 20150802 begin
/*
#ifdef TPD_CLOSE_POWER_IN_SLEEP
    hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
    msleep(20);
#endif*/

resume_proc:

    //[BEGIN] liushilong@yulong.com modify reset delay for avoiding error 2015-5-29
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(5);

    #ifdef TPD_CLOSE_POWER_IN_SLEEP
        hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
    #endif

    msleep(10);

    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    //[END]

    msleep(150);//shihaobin@yulong.com add for delay after reset tw 20150729

    //shihaobin@yulong.com add begin for judge if resume ture or false 20150802
    u8 work_data_buf;
    int ret = 0;
    int count_flag;
    int count = 0;
    for(count_flag = 0; count_flag < 3; count_flag++)
    {
        ret = fts_read_reg(i2c_client, FTS_REG_PMODE, &work_data_buf);
        if(ret < 0)
        {
            count++;
            if(3 == count)
            {
                printk("TPD : TP is not resume OK!\n");
                goto resume_proc;
            }
        }
        else
        {
            break;
        }
    }

#if TW_WINDOW_SWITCH
    if (1 == atomic_read(&fts_cover_window.windows_switch))
    {
        fts_write_reg(i2c_client, ID_G_COVER_MODE_EN, 0x01);
        //set window size
        //shihaobin@yulong.com delete for close window in driver begin 20150729
        #if 0
        {
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_LEFT, &fts_cover_window.win_x_min);
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_RIGHT, &fts_cover_window.win_x_max);
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_UP,  &fts_cover_window.win_y_min);
            fts_write_reg(i2c_client, ID_G_COVER_WINDOW_DOWN, &fts_cover_window.win_y_max);
        }
        #endif
        //shihaobin@yulong.com delete for close window in driver end 20150729
    }
    else
    {
        printk("[FTS] windows_switch is error\n");
    }
#endif
    //shihaobin@yulong.com modify for work time 20150802 end

    //shihaobin@yulong.com add for open glove mode when resume tw begin 20150729
    #if TW_GLOVE_SWITCH
        if (1 == glove_switch)
        {
            fts_write_reg(i2c_client, ID_G_GLOVE_MODE_EN , 0x01);   //open glove switch
        }
    #endif
    //shihaobin@yulong.com add for open glove mode when resume tw end 20150729

    chr_status_before = 0;//shihaobin@yulong.com add for modify charging status 20150731
    tpd_charging_filter();//shihaobin@yulong.com add for judgement if charging or not 20150729

    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    //msleep(30);/shihaobin@yulong.com delere for minus resume time 20150731
    tpd_halt = 0;

    /* for resume debug
    if((i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &data))< 0)
    {
    TPD_DMESG("resume I2C transfer error, line: %d\n", __LINE__);
    }
    */

    //[BEGIN] liushilong@yulong.com when TP resume, it should eliminate previous 2015-7-15
    tpd_up(0,0,0);
    input_sync(tpd->dev);
    //[END]
    //shihaobin@yulong.com modify for EDS protect begin 20170729
    #if FT_ESD_PROTECT
        count_irq = 0;
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
    #endif
    //shihaobin@yulong.com modify for EDS protect end 20170729
    TPD_DMESG("TPD wake up done\n");
   //return retval;
}

static void tpd_suspend( struct early_suspend *h )
{

    TPD_GESTURE_DBG("enter tpd_suspend %d\n",__LINE__);

#ifdef FTS_GESTRUE
    if(support_gesture & TW_SUPPORT_GESTURE_IN_ALL)
    {
        TPD_GESTURE_DBG("enter tpd_suspend %d\n",__LINE__);
        printk("tpd_suspend gesture %d\n",__LINE__);

        mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

        fts_write_reg(i2c_client, 0xd0, 0x01);

        fts_write_reg(i2c_client, 0xd1, 0xff);
        fts_write_reg(i2c_client, 0xd2, 0xff);
        fts_write_reg(i2c_client, 0xd5, 0xff);
        fts_write_reg(i2c_client, 0xd6, 0xff);
        fts_write_reg(i2c_client, 0xd7, 0xff);
        fts_write_reg(i2c_client, 0xd8, 0xff);

        doze_status = DOZE_ENABLED;
        TPD_GESTURE_DBG("set doze mode done !!!doze_status=%d ; line=%d\n",doze_status,__LINE__);
        printk("tpd_suspend doze_status=%d\n",doze_status);
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    }
    else
#endif //FTS_GESTRUE
    {
        //shihaobin@yulong.com modify for EDS protect begin 20170729
        #if FT_ESD_PROTECT
            cancel_delayed_work_sync(&gtp_esd_check_work);
        #endif
        //shihaobin@yulong.com modify for EDS protect end 20170729
        
        // int retval = TPD_OK;
        static char data = 0x3;
        tpd_halt = 1;
        printk("TPD enter sleep\n");
        mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
        mutex_lock(&i2c_access);
#ifdef TPD_CLOSE_POWER_IN_SLEEP
        hwPowerDown(TPD_POWER_SOURCE_CUSTOM,"TP");
#else
        i2c_smbus_write_i2c_block_data(i2c_client, 0xA5, 1, &data);  //TP enter sleep mode
#endif
        mutex_unlock(&i2c_access);
        TPD_DMESG("TPD enter sleep done\n");
        //return retval;
    }
}

static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = "ft5336",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
     .tpd_have_button = 1,
#else
     .tpd_have_button = 0,
#endif
};

/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{
    printk("MediaTek FT5446 touch panel driver init\n");
    i2c_register_board_info(TPD_I2C_NUMBER, &ft5336_i2c_tpd, 1);

    if(tpd_driver_add(&tpd_device_driver) < 0)
    {
        TPD_DMESG("add FT5446 driver failed\n");
        return -1;
    }
    printk("MediaTek FT5446 touch panel driver init success.\n");
    return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
    TPD_DMESG("MediaTek FT5446 touch panel driver exit\n");
    //input_unregister_device(tpd->dev);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);


