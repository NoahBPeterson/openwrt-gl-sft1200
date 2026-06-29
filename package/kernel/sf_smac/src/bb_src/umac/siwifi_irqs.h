#ifndef _SIWIFI_IRQS_H_
#define _SIWIFI_IRQS_H_ 
#include <linux/interrupt.h>
irqreturn_t siwifi_irq_hdlr(int irq, void *dev_id);
void siwifi_task(unsigned long data);
#endif
