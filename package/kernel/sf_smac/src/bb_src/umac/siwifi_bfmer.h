#ifndef _SIWIFI_BFMER_H_
#define _SIWIFI_BFMER_H_ 
#include "siwifi_defs.h"
#define SIWIFI_BFMER_REPORT_MAX_LEN 2048
#define SIWIFI_BFMER_REPORT_SPACE_SIZE (SIWIFI_BFMER_REPORT_MAX_LEN * 2)
struct siwifi_bfmer_report {
    dma_addr_t dma_addr;
    unsigned int length;
    u8 report[1];
};
int siwifi_bfmer_report_add(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta,
                          unsigned int length);
void siwifi_bfmer_report_del(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta);
u8 siwifi_bfmer_get_rx_nss(const struct ieee80211_vht_cap *vht_capa);
#endif
