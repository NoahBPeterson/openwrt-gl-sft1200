#ifndef _SIWIFI_FACTORY_INFO_H_
#define _SIWIFI_FACTORY_INFO_H_ 
#include "siwifi_defs.h"
#ifdef CONFIG_SFAX8_FACTORY_READ
#include <sfax8_factory_read.h>
#endif
#define LB_ONE_CHAN_GAIN_NUM 28
#define HB_ONE_CHAN_GAIN_NUM 53
#define LB_CHANNEL_COUNT 13
#define HB_CHANNEL_COUNT 25
#define LB_LIST_LEN (LB_ONE_CHAN_GAIN_NUM * LB_CHANNEL_COUNT)
#define HB_LIST_LEN (HB_ONE_CHAN_GAIN_NUM * HB_CHANNEL_COUNT)
#define LB_V2_MAGIC_INFO_SIZE 1
#define HB_V2_MAGIC_INFO_SIZE 1
#define LB_V2_MORE_INFO_SIZE (LB_V2_MAGIC_INFO_SIZE + LB_ONE_CHAN_GAIN_NUM)
#define HB_V2_MORE_INFO_SIZE (HB_V2_MAGIC_INFO_SIZE + HB_ONE_CHAN_GAIN_NUM)
#define SIWIFI_TXPOWER_SLEEPMODE_LOW_NAME "txp_offset_sleepmode_low.ini"
#define SIWIFI_TXPOWER_SLEEPMODE_LOW_SECOND_NAME "txp_offset_sleepmode_low_second.ini"
struct siwifi_factory_info {
    int version;
    int xo_value;
 char *sleepmode_txpower_list;
 char *low_txpower_list;
 char *normal_txpower_list;
    char *high_txpower_list;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
 char *sleepmode_txpower_list_second_antenna;
 char *low_txpower_list_second_antenna;
 char *normal_txpower_list_second_antenna;
#endif
    int list_len;
};
int sf_wifi_init_wifi_factory_info(struct siwifi_hw *siwifi_hw);
void sf_wifi_deinit_wifi_factory_info(struct siwifi_hw *siwifi_hw);
#endif
