#ifndef _WIFI_RF_SYSFS_H_
#define _WIFI_RF_SYSFS_H_ 
#include <linux/platform_device.h>
int sf_wifi_rf_sysfs_register(struct platform_device *pdev, char *parent);
int sf_wifi_rf_sysfs_unregister(struct platform_device *pdev);
#endif
