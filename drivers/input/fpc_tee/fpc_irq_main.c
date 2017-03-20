/* Fingerprint Cards, Hybrid Touch sensor driver
 *
 * Copyright (c) 2014,2015 Fingerprint Cards AB <tech@fingerprints.com>
 *
 *
 * Software license : "Dual BSD/GPL"
 * see <linux/module.h> and ./Documentation
 * for  details.
 *
*/

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <asm/siginfo.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/rcupdate.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/version.h>

#include "fpc_irq_common.h"
#include "fpc_irq_ctrl.h"
#include "fpc_irq_supply.h"
#include "fpc_irq_pm.h"


#include <mach/mt_gpio.h>
#include <mach/mt_spi.h>
#include <mach/eint.h>
#include <cust_eint.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <cust_vibrator.h>
#include <vibrator_hal.h>

#ifndef CONFIG_OF
// #include <linux/xxxx/fpc_irq.h> // todo
#else
#include <linux/of.h>
#include "fpc_irq.h"
#endif

MODULE_AUTHOR("Fingerprint Cards AB <tech@fingerprints.com>");
MODULE_DESCRIPTION("FPC IRQ driver.");

MODULE_LICENSE("Dual BSD/GPL");
#define MAX_NAME_LENGTH 20
/*start to add for getting module ID by qinxinjun@yulong.com in 20150522*/
extern int IMM_auxadc_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
/*end to add for getting module ID by qinxinjun@yulong.com in 20150522*/


/*start to add for spi clock node by qinxinjun@yulong.com in 20150618*/
#include <mach/mt_spi.h>
#include <mach/mt_clkmgr.h>
static int spi_clock_enable = 0;
/*end to add for spi clock node by qinxinjun@yulong.com in 20150618*/
int fpc1020_module_name(char *buffer);
static int  fpc_create_device(fpc_irq_data_t *fpc_irq_data);
static int fpc_irq_create_class(fpc_irq_data_t *fpc_irq_data);
/* -------------------------------------------------------------------------- */
/* platform compatibility                                                     */
/* -------------------------------------------------------------------------- */
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
	#include <linux/interrupt.h>
	#include <linux/irqreturn.h>
	#include <linux/of_gpio.h>
//#endif


/* -------------------------------------------------------------------------- */
/* global variables                                                           */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/* fpc data types                                                             */
/* -------------------------------------------------------------------------- */
struct fpc_irq_attribute {
	struct device_attribute attr;
	size_t offset;
};


/* -------------------------------------------------------------------------- */
/* fpc_irq driver constants                                                   */
/* -------------------------------------------------------------------------- */
#define FPC_IRQ_CLASS_NAME	"fpsensor_irq"

#define FPC1020_DEV_NAME                        "fpc1020"

/* set '0' for dynamic assignment, or '> 0' for static assignment */
#define FPC1020_MAJOR				0

#define FPC_IRQ_WORKER_NAME	"fpc_irq_worker"
#define FPC_IRQ_EVENT_NAME	"fpc_irq_event"
fpc_irq_data_t *fpc_irq = NULL;

#define CUST_EINT_EDGE_SENSITIVE 0
#define EINT1               11
#define FPC_INT_IRQNO       EINT1
#define FPC_IRQ_GPIO       FPC_INT_IRQNO
 
#define GPIO_FPC_SPI_CS             (GPIO65 | 0x80000000)
#define GPIO_FPC_SPI_CLK            (GPIO66 | 0x80000000)
#define GPIO_FPC_SPI_MISO           (GPIO67 | 0x80000000)
#define GPIO_FPC_SPI_MOSI           (GPIO68 | 0x80000000)
#define GPIO_FPC1020_IRQ            (GPIO11 | 0x80000000)
#define FPC_RESET_GPIO              (GPIO59 | 0x80000000)
#define GPIO_FPC1020_ID             (GPIO12 | 0x80000000)


/* -------------------------------------------------------------------------- */
/* function prototypes                                                        */
/* -------------------------------------------------------------------------- */
static int fpc_irq_init(void);

static void fpc_irq_exit(void);

static int fpc_irq_probe(struct platform_device *plat_dev);

static int fpc_irq_remove(struct platform_device *plat_dev);


//static int fpc_irq_platform_init(fpc_irq_data_t *fpc_irq_data,
	//			fpc_irq_pdata_t *pdata);

static int fpc_irq_platform_init();

static int fpc_irq_platform_destroy(fpc_irq_data_t *fpc_irq_data);

static int fpc_irq_create_class(fpc_irq_data_t *fpc_irq_data);



//irqreturn_t fpc_irq_interrupt(int irq, void *_fpc_irq_data);
void fpc_irq_interrupt(void);




/* -------------------------------------------------------------------------- */
/* External interface                                                         */
/* -------------------------------------------------------------------------- */
module_init(fpc_irq_init);
module_exit(fpc_irq_exit);

//#ifdef CONFIG_OF
#if 0
static struct of_device_id fpc_irq_of_match[] __devinitdata = {
	{.compatible = "fpc,fpc_irq", },
	{}
};
MODULE_DEVICE_TABLE(of, fpc_irq_of_match);
#endif

static struct platform_device *fpc_irq_platform_device;

static struct platform_driver fpc_irq_driver = {
	.driver	 = {
		.name		= FPC_IRQ_DEV_NAME,
		.owner		= THIS_MODULE,
//#ifdef CONFIG_OF
#if 0
		.of_match_table	= fpc_irq_of_match,
#endif
	},
	.probe   = fpc_irq_probe,
	.remove  = fpc_irq_remove,
};

extern void spi_dump_sysclk_reg(void );
/* -------------------------------------------------------------------------- */
/* function definitions                                                       */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int fpc1020_hw_prepare(fpc_irq_data_t *fpc_irq_data)
{
    printk(KERN_INFO "%s\n", __func__);
    enable_clock(MT_CG_PERI_SPI0, "spi");
    //spi_dump_sysclk_reg();
    return 0;
}

int fpc1020_hw_unprepare(fpc_irq_data_t *fpc_irq_data)
{
    printk(KERN_INFO "%s\n", __func__);
    disable_clock(MT_CG_PERI_SPI0, "spi");
    //spi_dump_sysclk_reg();
    return 0;
}

static int fpc1020_open(struct inode *inode, struct file *file)

{
	fpc_irq_data_t *fpc_irq_data;

	printk(KERN_INFO "%s\n", __func__);

	fpc_irq_data = container_of(inode->i_cdev, fpc_irq_data_t, cdev);

	if (down_interruptible(&fpc_irq_data->mutex))
		return -ERESTARTSYS;

	file->private_data = fpc_irq_data;

	up(&fpc_irq_data->mutex);

	return 0;
}



/* -------------------------------------------------------------------- */
static int fpc1020_release(struct inode *inode, struct file *file)
{
	fpc_irq_data_t *fpc_irq_data = file->private_data;
	int status = 0;

	printk(KERN_INFO "%s\n", __func__);

	if (down_interruptible(&fpc_irq_data->mutex))
		return -ERESTARTSYS;

	up(&fpc_irq_data->mutex);

	return status;
}


static unsigned int fpc1020_poll(struct file *file, poll_table *wait)
{
	fpc_irq_data_t *fpc_irq_data = file->private_data;
	poll_wait(file, &fpc_irq_data->wq_irq_return, wait);
	if (mt_get_gpio_in(GPIO_FPC1020_IRQ)) {
	//if(fpc1020->interrupt_done == true) {
	//	fpc1020->interrupt_done = false;
		return POLLIN;
	}
	else return 0;
}


static long fpc1020_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
    fpc_irq_data_t *fpc_irq_data = file->private_data;
	char buffer[MAX_NAME_LENGTH];
	void __user *up = (void __user *)arg;
	void *param = (char *)arg;
	//void __iomem *addr;
	int rc = 0;
	//int sn = 0;
	int mask = 0;
	switch (cmd) {
		case FPC_HW_RESET:
			 /* hardare reset */
    		mt_set_gpio_mode(FPC_RESET_GPIO, GPIO_MODE_00);
    		mt_set_gpio_out(FPC_RESET_GPIO,0);
    		mdelay(10);

    		mt_set_gpio_out(FPC_RESET_GPIO,1);
    		mdelay(10);
			break;
		case FPC_HW_PREPARE:
			fpc1020_hw_prepare(fpc_irq_data);
			break;

		case FPC_HW_UNPREPARE:
			fpc1020_hw_unprepare(fpc_irq_data);
			break;

		case FPC_GET_INTERRUPT:
			if (!up)
			{
				//dev_err(&fpc1020->spi->dev, "%s up is NULL \n", __func__);
                printk(KERN_ERR "%s up is NULL \n", __func__);
			}
			
			//rc = gpio_get_value(fpc1020->irq_gpio);
			rc = mt_get_gpio_in(GPIO_FPC1020_IRQ);
			if (put_user(rc,(int *)up))
			{
				//dev_dbg(&fpc1020->spi->dev, "fpc put_user (%d) failed " , *(int *)up);
                printk(KERN_ERR  "fpc put_user (%d) failed " , *(int *)up);
				rc = -EFAULT;
			}
			break;

		case FPC_MASK_INTERRUPT:
			if (__get_user(mask, (int __user *)arg)) {
				printk("arg is inval");
				return -EINVAL;
			}
			if(fpc_irq_data->irq_state == mask){
				printk("already in %d",mask);
				break;
			}
			fpc_irq_data->irq_state=mask;
			if(mask == 1)
			{
				mt_eint_mask(fpc_irq_data->pdata.irq_no);  
				fpc_irq_data->interrupt_done = false;
			}else {
				mt_eint_unmask(fpc_irq_data->pdata.irq_no); 
				fpc_irq_data->interrupt_done = false;
			}
			break;

		case FPC_GET_MODULE_NAME:			
			rc = fpc1020_module_name(buffer);
			rc = copy_to_user(param,buffer, rc+1);
			if (rc ) {
				rc = -EFAULT;
				dev_err(fpc_irq_data->dev, "get module name fail,rc=%d", rc);
			}			 
			break;

	default:
		dev_dbg(fpc_irq_data->dev, "ENOIOCTLCMD: cmd=%d ", cmd);
		return -ENOIOCTLCMD;
	}

	return rc;
}
/**************************************************************/
/* -------------------------------------------------------------------------- */
int fpc1020_module_name(char *buffer)
{
    /*start to add for getting module ID by qinxinjun@yulong.com in 20150522*/
    int data[4];
    int adcVol;
    int res;
    char *module_name = NULL;
    res = IMM_auxadc_GetOneChannelValue(12, data, NULL);
    adcVol = data[0]*1000 + data[1]*10;

    if((adcVol > 50) && (adcVol <= 365))
        module_name = "ofilm";
    /*start to add for dreamtech module ID by qinxinjun@yulong.com in 20150601*/
    else if((adcVol > 365) && (adcVol <= 775))
        module_name = "guangbao";
    else if((adcVol > 775) && (adcVol <= 1100))
        module_name = "dreamtech";
    /*end to add for dreamtech module ID by qinxinjun@yulong.com in 20150601*/
    else
        module_name = "unknown";

    printk("fingerprint module adcVol = %d , module name is %s\n",adcVol,module_name);

	return sprintf(buffer, module_name);


}
static int fpc_irq_init(void)
{
	printk(KERN_INFO "%s\n", __func__);

	fpc_irq_platform_device = platform_device_register_simple(
							FPC_IRQ_DEV_NAME,
							0,
							NULL,
							0);

	if (IS_ERR(fpc_irq_platform_device))
		return PTR_ERR(fpc_irq_platform_device);
	
	return (platform_driver_register(&fpc_irq_driver) != 0)? EINVAL : 0;
}


/* -------------------------------------------------------------------------- */
static void fpc_irq_exit(void)
{
	printk(KERN_INFO "%s\n", __func__);

	platform_driver_unregister(&fpc_irq_driver);

	platform_device_unregister(fpc_irq_platform_device);
}

static const struct file_operations fpc_irq_fops = {
	.owner          = THIS_MODULE,
	.open           = fpc1020_open,
	.release        = fpc1020_release,
	.poll           = fpc1020_poll,
	.unlocked_ioctl = fpc1020_ioctl,
};

/* -------------------------------------------------------------------------- */
static int fpc_irq_probe(struct platform_device *plat_dev)
{
	int error = 0;
	fpc_irq_data_t *fpc_irq_data = NULL;

	fpc_irq_pdata_t *pdata_ptr;
	fpc_irq_pdata_t pdata_of;

	dev_info(&plat_dev->dev, "%s\n", __func__);

    printk(KERN_ERR "enter %s\n", __func__);
     /*Set SPI PORT add by lucky.wang@yulong.com 2015-03-31 */
    mt_set_gpio_mode(GPIO_FPC_SPI_CS, GPIO_MODE_01);
    mt_set_gpio_mode(GPIO_FPC_SPI_CLK, GPIO_MODE_01);
    mt_set_gpio_mode(GPIO_FPC_SPI_MISO, GPIO_MODE_01);
    mt_set_gpio_mode(GPIO_FPC_SPI_MOSI, GPIO_MODE_01);
   
    mt_set_gpio_mode(FPC_RESET_GPIO, GPIO_MODE_00);
    mdelay(10);

    /* hardare reset */
    mt_set_gpio_out(FPC_RESET_GPIO,1);
    mdelay(10);

    mt_set_gpio_out(FPC_RESET_GPIO,0);
    mdelay(10);

    mt_set_gpio_out(FPC_RESET_GPIO,1);
    mdelay(10);    

	fpc_irq_data = kzalloc(sizeof(*fpc_irq_data), GFP_KERNEL);
	if (!fpc_irq_data) {
		dev_err(&plat_dev->dev, "failed to allocate memory for struct fpc_irq_data\n");

		return -ENOMEM;
	}

	platform_set_drvdata(plat_dev, fpc_irq_data);

	fpc_irq_data->plat_dev = plat_dev;
	fpc_irq_data->dev = &plat_dev->dev;

	fpc_irq_data->pdata.irq_gpio = FPC_IRQ_GPIO;
	fpc_irq_data->pdata.irq_no   = FPC_INT_IRQNO ;
	fpc_irq_data->pdata.rst_gpio = FPC_RESET_GPIO;
    fpc_irq =  fpc_irq_data;

	init_waitqueue_head(&fpc_irq_data->wq_irq_return);

	pdata_ptr = plat_dev->dev.platform_data;
	
    printk(KERN_ERR "enter =========%s\n", __func__);

	//error = fpc_irq_platform_init(fpc_irq_data, pdata_ptr);
    error = fpc_irq_platform_init();
	if (error)
		goto err_1;

	error = fpc_irq_create_class(fpc_irq_data);
	if (error)
		goto err_2;
	
	fpc_create_device(fpc_irq_data);
	sema_init(&fpc_irq_data->mutex, 0);

	cdev_init(&fpc_irq_data->cdev, &fpc_irq_fops);
	fpc_irq_data->cdev.owner = THIS_MODULE;

	error = cdev_add(&fpc_irq_data->cdev, fpc_irq_data->devno, 1);
	if (error) {
		dev_err(&fpc_irq_data->dev, "cdev_add failed.\n");
	}

	up(&fpc_irq_data->mutex);

	return 0;

err_3:
	class_destroy(fpc_irq_data->class);
err_2:
	fpc_irq_platform_destroy(fpc_irq_data);
err_1:
	platform_set_drvdata(plat_dev, NULL);

	kfree(fpc_irq_data);

	return error;
}


/* -------------------------------------------------------------------------- */
static int fpc_irq_remove(struct platform_device *plat_dev)
{
	fpc_irq_data_t *fpc_irq_data;


	fpc_irq_data = platform_get_drvdata(plat_dev);

    //destroy_workqueue(uevent_report_workqueue);

    printk("queue_delayed_work : LINE = %d : %s\n",__LINE__,__func__);

	class_destroy(fpc_irq_data->class);
//err_2:
	fpc_irq_platform_destroy(fpc_irq_data);
//err_1:
	platform_set_drvdata(plat_dev, NULL);

	kfree(fpc_irq_data);

	return 0;
}


/* -------------------------------------------------------------------------- */
// #ifdef CONFIG_OF
#if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int fpc_irq_get_of_pdata(struct platform_device *dev,
				fpc_irq_pdata_t *pdata)
{
	int num_io;
	struct device_node *node = dev->dev.of_node;

	if (node == NULL) {
		dev_err(&dev->dev, "%s: Could not find OF device node\n", __func__);
		goto of_err;
	}

	num_io = of_gpio_count(node);

	// Required, IRQ GPIO ...
	pdata->irq_gpio = (num_io >= 1) ? of_get_gpio(node, 0) : -EINVAL;

	if (pdata->irq_gpio < 0) {
		dev_err(&dev->dev,
			"%s: req. OF property : 'irq_gpio' missing or error (%d)\n",
			__func__,
			pdata->irq_gpio);
		goto of_err;
	} else {
		dev_dbg(&dev->dev, "%s: found irq_gpio = %d\n", __func__, pdata->irq_gpio);
	}

	// Optional hardware reset GPIO ...
	pdata->rst_gpio = (num_io >= 2) ? of_get_gpio(node, 1) : -EINVAL;

	if (pdata->rst_gpio < 0)
		dev_dbg(&dev->dev, "%s: rst_gpio, not found\n", __func__);
	else
		dev_dbg(&dev->dev, "%s: found rst_gpio = %d\n", __func__, pdata->rst_gpio);

	return 0;

of_err:
	pdata->irq_gpio = -EINVAL;
	pdata->irq_no   = -EINVAL;
	pdata->rst_gpio = -EINVAL;

	return -ENODEV;
}
#else
static int fpc_irq_get_of_pdata(struct platform_device *dev,
				fpc_irq_pdata_t *pdata)
{
	const struct device_node *node = dev->dev.of_node;

	const void *irq_prop = of_get_property(node, "fpc,gpio_irq", NULL); // Required
	const void *rst_prop = of_get_property(node, "fpc,gpio_rst",  NULL); // Optional

	if (node == NULL) {
		dev_err(&dev->dev, "%s: Could not find OF device node\n", __func__);
		goto of_err;
	}

	if (!irq_prop) {
		dev_err(&dev->dev, "%s: Missing OF property\n", __func__);
		goto of_err;
	}

	pdata->irq_gpio = (irq_prop != NULL) ? be32_to_cpup(irq_prop) : -EINVAL;
	pdata->rst_gpio = (rst_prop != NULL) ? be32_to_cpup(rst_prop) : -EINVAL;

	return 0;

of_err:
	pdata->irq_gpio = -EINVAL;
	pdata->irq_no   = -EINVAL;
	pdata->rst_gpio = -EINVAL;

	return -ENODEV;
}

#endif // KERNEL_VERSION >= 3.8.0

#else
static int fpc_irq_get_of_pdata(struct platform_device *dev,
				fpc_irq_pdata_t *pdata)
{

//	pdata->irq_gpio = -EINVAL;
//	pdata->irq_no   = -EINVAL;
//	pdata->rst_gpio = -EINVAL;

//	return -ENODEV;
    return 0;
}
#endif // CONFIG_OF


/* -------------------------------------------------------------------------- */
//static int fpc_irq_platform_init(fpc_irq_data_t *fpc_irq_data,
	//			fpc_irq_pdata_t *pdata)
static int fpc_irq_platform_init()
{
	int error = 0;

    printk(KERN_ERR "enter======%s \n", __func__);	
    // set irq gpio

  	error = mt_set_gpio_mode(GPIO_FPC1020_IRQ, GPIO_MODE_01);
	if (error != 0) {
		printk("mt_set_gpio_mode (eint) failed.error=%d\n",error);
	}

  	error = mt_set_gpio_dir(GPIO_FPC1020_IRQ, GPIO_DIR_IN);
	if (error != 0) {
		printk("mt_set_gpio_dir (eint) failed.error=%d\n",error);
	}

  	error = mt_set_gpio_pull_enable(GPIO_FPC1020_IRQ, GPIO_PULL_DISABLE);
	if (error != 0) {
		printk("mt_set_gpio_pull_enable (eint) failed.error=%d\n",error);
	}
	
	//FPC_INT_IRQNO = gpio_to_irq(GPIO_FPC1020_IRQ);
    printk(KERN_ERR "enter======FPC_INT_IRQNO = %d %s \n", FPC_INT_IRQNO, __func__);	
	mt_eint_registration(FPC_INT_IRQNO, EINTF_TRIGGER_RISING, fpc_irq_interrupt, 1);
	mt_eint_unmask(FPC_INT_IRQNO);


#if 0
	if (gpio_is_valid(pdata->irq_gpio)) {

		dev_info(fpc_irq_data->dev,
			"Assign IRQ -> GPIO%d\n",
			pdata->irq_gpio);

		error = gpio_request(GPIO_FPC1020_IRQ, "fpc_irq");

		if (error) {
			dev_err(fpc_irq_data->dev, "gpio_request failed.\n");
			return error;
		}

		fpc_irq_data->pdata.irq_gpio = GPIO_FPC1020_IRQ;

		error = gpio_direction_input(GPIO_FPC1020_IRQ);

		if (error) {
			dev_err(fpc_irq_data->dev, "gpio_direction_input (irq) failed.\n");
			return error;
		}
	} else {
		dev_err(fpc_irq_data->dev, "IRQ gpio not valid.\n");
		return -EINVAL;
	}

	fpc_irq_data->pdata.irq_no = gpio_to_irq(GPIO_FPC1020_IRQ);

	if (fpc_irq_data->pdata.irq_no < 0) {
		dev_err(fpc_irq_data->dev, "gpio_to_irq failed.\n");
		error = fpc_irq_data->pdata.irq_no;
		return error;
	}

	error = request_irq(fpc_irq_data->pdata.irq_no, fpc_irq_interrupt,
			IRQF_TRIGGER_RISING, "fpc_irq", fpc_irq_data);

	disable_irq(fpc_irq_data->pdata.irq_no);

	if (error) {
		dev_err(fpc_irq_data->dev,
			"request_irq %i failed.\n",
			fpc_irq_data->pdata.irq_no);

		fpc_irq_data->pdata.irq_no = -EINVAL;
	}

#endif
	return error;
}


/* -------------------------------------------------------------------------- */
static int fpc_irq_platform_destroy(fpc_irq_data_t *fpc_irq_data)
{
	dev_dbg(fpc_irq_data->dev, "%s\n", __func__);

#if 0
	if (fpc_irq_data->pdata.irq_no >= 0)
		free_irq(fpc_irq_data->pdata.irq_no, fpc_irq_data);

	if (gpio_is_valid(fpc_irq_data->pdata.irq_gpio))
		gpio_free(fpc_irq_data->pdata.irq_gpio);
#endif
	return 0;
}


/* -------------------------------------------------------------------------- */
static int fpc_irq_create_class(fpc_irq_data_t *fpc_irq_data)
{
	int error = 0;

	dev_dbg(fpc_irq_data->dev, "%s\n", __func__);

	fpc_irq_data->class = class_create(THIS_MODULE, FPC_IRQ_CLASS_NAME);

	if (IS_ERR(fpc_irq_data->class)) {
		dev_err(fpc_irq_data->dev, "failed to create class.\n");
		error = PTR_ERR(fpc_irq_data->class);
	}

	return error;
}

static int  fpc_create_device(fpc_irq_data_t *fpc_irq_data)
{
	int error = 0;

	dev_dbg(fpc_irq_data->dev, "%s\n", __func__);

	if (FPC1020_MAJOR > 0) {
		fpc_irq_data->devno = MKDEV(FPC1020_MAJOR, 0);

		error = register_chrdev_region(fpc_irq_data->devno,
						1,
						FPC1020_DEV_NAME);
	} else {
		error = alloc_chrdev_region(&fpc_irq_data->devno,0,1,FPC1020_DEV_NAME);
	}

	if (error < 0) {
		dev_err(fpc_irq_data->dev,
				"%s: FAILED %d.\n", __func__, error);
		goto out;

	} else {
		dev_info(fpc_irq_data->dev, "%s: major=%d, minor=%d\n",
						__func__,
						MAJOR(fpc_irq_data->devno),
						MINOR(fpc_irq_data->devno));
	}

	fpc_irq_data->cdevice = device_create(fpc_irq_data->class, NULL, fpc_irq_data->devno,
						NULL, "%s", FPC1020_DEV_NAME);

	if (IS_ERR(fpc_irq_data->cdevice)) {
		dev_err(fpc_irq_data->dev, "device_create failed.\n");
		error = PTR_ERR(fpc_irq_data->cdevice);
	}

out:
	return error;
}

void  fpc_irq_interrupt(void)
{
	fpc_irq_data_t *fpc_irq_data = fpc_irq;

    printk(KERN_ERR "test fpc_irq info  %s  irq_gpio = %d \n", __func__, mt_get_gpio_in(GPIO_FPC1020_IRQ));
    
    mt_eint_mask(fpc_irq_data->pdata.irq_no);    
	if (mt_get_gpio_in(GPIO_FPC1020_IRQ)) {
		fpc_irq_data->interrupt_done = true;
		wake_up_interruptible(&fpc_irq_data->wq_irq_return);
        mt_eint_unmask(fpc_irq_data->pdata.irq_no); 
		return;
	}
    mt_eint_unmask(fpc_irq_data->pdata.irq_no); 

	return;
}
/**************************************************************/
