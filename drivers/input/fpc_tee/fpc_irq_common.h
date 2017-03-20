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

#ifndef __FPC_IRQ_COMMON_H
#define __FPC_IRQ_COMMON_H

#include <linux/completion.h>
#include <linux/input.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/version.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif


/* -------------------------------------------------------------------------- */
/* platform compatibility                                                     */
/* -------------------------------------------------------------------------- */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
	#define SLEEP_US(delay) {usleep_range((delay), (delay)); }
#else
	#define SLEEP_US(delay) {usleep((delay)); }
#endif

/* -------------------------------------------------------------------------- */
/* fpc_irq driver constants                                                   */
/* -------------------------------------------------------------------------- */
#define FPC_IRQ_DEV_NAME	"fpc_irq"

/* NOTE: driver and HAL must share same definition for SIGNAL and STATE */
enum {
	FPC_IRQ_SIGNAL_TEST          = 1,
	FPC_IRQ_SIGNAL_INTERRUPT_REQ = 2,
	FPC_IRQ_SIGNAL_SUSPEND_REQ   = 3,
	FPC_IRQ_SIGNAL_RESUME_REQ    = 4,
#ifdef CONFIG_HAS_EARLYSUSPEND
	FPC_IRQ_SIGNAL_SUSPEND_EARLY_REQ = 5,
	FPC_IRQ_SIGNAL_RESUME_LATE_REQ   = 6,
#endif
};

enum {
	FPC_IRQ_STATE_ACTIVE        = 1,
	FPC_IRQ_STATE_SUSPENDED     = 2,
#ifdef CONFIG_HAS_EARLYSUSPEND
	FPC_IRQ_STATE_EARLY_SUSPEND = 3,
	FPC_IRQ_STATE_LATE_RESUME   = 4,
#endif
};

/* -------------------------------------------------------------------------- */
/* fpc data types                                                             */
/* -------------------------------------------------------------------------- */
struct fpc_irq_setup {
	pid_t dst_pid;
	int   dst_signo;
	int   enabled;
	int   test_trigger; // Todo: remove ?
    const char *module_id; /*add for getting module ID by qingixnjun@yulong.com in 20150522*/
};

struct fpc_irq_pm {
	int  state;
	bool supply_on;
	bool hw_reset;
	bool notify_enabled;
	bool notify_ack;
	bool wakeup_req;
    bool wakeup_flagX;/*add for virtual event node by qinxinjun@yulong.com in 20150602*/
    bool spi_clk;/*add for spi clock node by qinxinjun@yulong.com in 20150618*/
};

typedef struct {
	int irq_gpio;
	int irq_no;
	int rst_gpio;
} fpc_irq_pdata_t;

typedef struct {
	struct platform_device  *plat_dev;
	struct device	  	*dev;
	struct input_dev	*input_dev;
	struct class		*class;
	struct cdev            cdev;	
	dev_t                  devno;	
	struct device          *cdevice;
	fpc_irq_pdata_t		pdata;
	struct fpc_irq_setup	setup;
	struct semaphore	mutex;
	wait_queue_head_t	wq_irq_return;
	bool			interrupt_done;
    int irq_state;
} fpc_irq_data_t;


/* -------------------------------------------------------------------------- */
/* function prototypes                                                        */
/* -------------------------------------------------------------------------- */

/***************************************************/
#define FPC_HW_PREPARE			_IOW('K', 0, int)
#define FPC_HW_UNPREPARE		_IOW('K', 1, int)
#define FPC_GET_INTERRUPT 		_IOR('K', 2, int)
#define FPC_GET_MODULE_NAME		_IOR('K', 3, int)
#define FPC_MASK_INTERRUPT		_IOW('K', 4, int)
#define FPC_HW_RESET			_IOW('K', 5, int)
#define FPC_GET_SERIAL_NUM		_IOR('K', 6, int)
/***************************************************/

#endif /* __FPC_IRQ_COMMON_H */

