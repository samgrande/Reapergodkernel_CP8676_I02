/* drivers/i2c/chips/ms5607.c - MS5607 motion sensor driver
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
//#include <math.h>
#include <cust_eint.h>

//#include <cust_baro.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include <mach/mt_gpio.h>
#include <linux/switch.h>
#include <mach/eint.h>
#include <linux/kpd.h>

static struct switch_dev hall_data;
//struct timer_list HALL_timer;
//#define HALL_RATE_TIMER   (1 *HZ)         // 1 seconds
static struct work_struct hall_work;
static struct workqueue_struct * hall_workqueue = NULL;
struct early_suspend    early_hall_drv;
atomic_t                hall_suspend;

extern struct input_dev *kpd_input_dev;

/******************************************************************************
 * extern functions
*******************************************************************************/
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);
extern void hall_state_notify_touch(int state);   // yulong tanyang add //chenbing3@yulong.com unmark for complix err

static void hall_poing_callback(struct work_struct *work)
{
         int hall_value;

     hall_value= mt_get_gpio_in(GPIO_QWERTYSLIDE_EINT_PIN);


        if(hall_value)
        {
            mt_eint_set_polarity(CUST_EINT_KPD_SLIDE_NUM,  MT_EINT_POL_NEG );
                mt_eint_set_sens(CUST_EINT_KPD_SLIDE_NUM, MT_LEVEL_SENSITIVE);
            switch_set_state((struct switch_dev *)&hall_data, 0);
            //hall_state_notify_touch(0);  // yulong tanyang add //chenbing3@yulong.com mark for complix err

                    #if 0
            if(atomic_read(&hall_suspend))
                {
                    input_report_key(kpd_input_dev,KPD_PWRKEY_MAP,1);
                    input_sync(kpd_input_dev);
                    printk("hall_suspend HALL_value far away!\n");
                    input_report_key(kpd_input_dev,KPD_PWRKEY_MAP,0);
                    input_sync(kpd_input_dev);
                }
                    #endif

            printk("HALL_value far away!\n");
        }
        else
        {
                mt_eint_set_polarity(CUST_EINT_KPD_SLIDE_NUM, MT_EINT_POL_POS);
                mt_eint_set_sens(CUST_EINT_KPD_SLIDE_NUM, MT_LEVEL_SENSITIVE);
            switch_set_state((struct switch_dev *)&hall_data, 1);
                //hall_state_notify_touch(1);  // yulong tanyang add //chenbing3@yulong.com unmark for complix err

                    #if 0
            if(atomic_read(&hall_suspend) == 0)
                {
                    input_report_key(kpd_input_dev,KPD_PWRKEY_MAP,1);
                    input_sync(kpd_input_dev);
                    printk("hall_suspend HALL_value far away!\n");
                    input_report_key(kpd_input_dev,KPD_PWRKEY_MAP,0);
                    input_sync(kpd_input_dev);
                }
                    #endif

            printk("HALL_value close!\n");
        }

    mt_eint_unmask(CUST_EINT_KPD_SLIDE_NUM);

}

static irqreturn_t detect_hall_handler(int irq, void *dev_id)
{
    int ret = 0, pol = 1;
    int retry_limit = 10;

    queue_work(hall_workqueue, &hall_work);
        printk("detect_hall_handler!\n");
    return IRQ_HANDLED;
}

static void hall_early_suspend(struct early_suspend *h)
{
    printk("hall_early_suspend!!!!!!\n");
    atomic_set(&hall_suspend, 1);
}

/*----------------------------------------------------------------------------*/
static void hall_late_resume(struct early_suspend *h)
{
    printk("hall_late_resume!!!!!!\n");
    atomic_set(&hall_suspend, 0);
}

/*----------------------------------------------------------------------------*/
static int hall_probe(struct platform_device *pdev)
{
       int ret;

       printk("HALL_probe1111111111!!!!\n");

    mt_set_gpio_mode(GPIO_QWERTYSLIDE_EINT_PIN, GPIO_QWERTYSLIDE_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_QWERTYSLIDE_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_QWERTYSLIDE_EINT_PIN, TRUE);
    mt_set_gpio_pull_select(GPIO_QWERTYSLIDE_EINT_PIN, GPIO_PULL_UP);

    msleep(50);

    hall_data.name = "hall";
    hall_data.index = 0;
    hall_data.state = 0;
       hall_workqueue = create_singlethread_workqueue("hall_polling");

    if (hall_workqueue == NULL) {
        ret = -ENOMEM;
        goto err_create_hall_queue;
    }

       INIT_WORK(&hall_work, hall_poing_callback);

    ret = switch_dev_register(&hall_data);
    if (ret < 0)
    {
        goto err_switch_hall_register;
    }

    #ifdef CONFIG_HAS_EARLYSUSPEND
    early_hall_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    early_hall_drv.suspend  = hall_early_suspend,
    early_hall_drv.resume   = hall_late_resume,
    register_early_suspend(&early_hall_drv);
    #endif
    atomic_set(&hall_suspend, 0);

        mt_eint_set_hw_debounce(CUST_EINT_KPD_SLIDE_NUM, 256);
    ret = mt_get_gpio_in(GPIO_QWERTYSLIDE_EINT_PIN);
        if(ret)
    {
               mt_eint_registration(CUST_EINT_KPD_SLIDE_NUM, CUST_EINTF_TRIGGER_LOW, detect_hall_handler, 0);
               irq_set_irq_wake(CUST_EINT_KPD_SLIDE_NUM,1);  //yulong tanyang add for 8680
    }
        else
    {
             mt_eint_registration(CUST_EINT_KPD_SLIDE_NUM, CUST_EINTF_TRIGGER_HIGH, detect_hall_handler, 0);
               irq_set_irq_wake(CUST_EINT_KPD_SLIDE_NUM,1);  //yulong tanyang add for 8680
    }

        mt_eint_unmask(CUST_EINT_KPD_SLIDE_NUM);
    if(ret == 1)
    {
        switch_set_state((struct switch_dev *)&hall_data, 0);
    }
    else
    {
        switch_set_state((struct switch_dev *)&hall_data, 1);
    }

    return 0;

err_switch_hall_register:
     destroy_workqueue(hall_workqueue);

err_create_hall_queue:
    printk(KERN_ERR "hall failed to create hall_queue!\n");
    return ret;

}
/*----------------------------------------------------------------------------*/
static int hall_remove(struct platform_device *pdev)
{
        destroy_workqueue(hall_workqueue);
    switch_dev_unregister(&hall_data);
        return 0;
}
/*----------------------------------------------------------------------------*/


static struct platform_device hall_device =
{
    .name = "bu52031_hall",
    .id   = -1,
};

static struct platform_driver hall_driver = {
    .probe      = hall_probe,
    .remove     = hall_remove,
    .driver     = {
        .name  = "bu52031_hall",
        .owner = THIS_MODULE,
    }
};

/*----------------------------------------------------------------------------*/
static int __init hall_init(void)
{
/*-begin- chenbing3@yulong.com add for hall_sensor debug 2014/11/20*/
    printk("hall_init!!!!\n");
/*-end- chenbing3@yulong.com add for hall_sensor debug 2014/11/20*/
    platform_device_register(&hall_device);

    if(platform_driver_register(&hall_driver))
    {
        printk("failed to register  hall driver");
        return -ENODEV;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/

static void __exit hall_exit(void)
{
    platform_driver_unregister(&hall_driver);
}
/*----------------------------------------------------------------------------*/
module_init(hall_init);
module_exit(hall_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hall driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
