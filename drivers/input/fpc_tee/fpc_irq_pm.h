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

#ifndef __FPC_IRQ_PM_H__
#define __FPC_IRQ_PM_H__

#include "fpc_irq_common.h"

extern int wakeup_flag; /*start to add for virtual event node by qinxinjun@yulong.com in 20150602*/
extern int fpc_irq_worker_functionX(void);//qinxinjun

extern int fpc_irq_pm_suspend(struct platform_device *plat_dev, pm_message_t state);

extern int fpc_irq_pm_resume(struct platform_device *plat_dev);

extern int fpc_irq_pm_init(fpc_irq_data_t *fpc_irq_data);

extern int fpc_irq_pm_destroy(fpc_irq_data_t *fpc_irq_data);

extern int fpc_irq_pm_notify_enable(fpc_irq_data_t *fpc_irq_data,
				int req_state);

extern int fpc_irq_pm_wakeup_req(fpc_irq_data_t *fpc_irq_data);

extern int fpc_irq_pm_notify_ack(fpc_irq_data_t *fpc_irq_data, int val);

#endif /* __FPC_IRQ_PM_H__ */



