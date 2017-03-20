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

#ifndef __FPC_IRQ_CTRL_H
#define __FPC_IRQ_CTRL_H

#include "fpc_irq_common.h"
#include <mach/mt_gpio.h>


#define FPC_RESET_GPIO              (GPIO59 | 0x80000000)
#define GPIO_FPC1020_ID             (GPIO12 | 0x80000000)

extern int fpc_irq_ctrl_init();

extern int fpc_irq_ctrl_destroy(fpc_irq_data_t *fpc_irq_data);

extern int fpc_irq_ctrl_hw_reset(fpc_irq_data_t *fpc_irq_data);

#endif /* __FPC_IRQ_CTRL_H */


