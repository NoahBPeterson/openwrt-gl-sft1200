#ifndef _SIWIFI_MAIN_H_
#define _SIWIFI_MAIN_H_ 
#include "siwifi_defs.h"
int siwifi_cfg80211_init(struct siwifi_plat *siwifi_plat, void **platform_data);
void siwifi_cfg80211_deinit(struct siwifi_hw *siwifi_hw);
#endif
