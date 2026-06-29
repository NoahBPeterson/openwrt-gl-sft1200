#include <linux/slab.h>
#include "siwifi_bfmer.h"
#include "siwifi_mem.h"
int siwifi_bfmer_report_add(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta,
                          unsigned int length)
{
    gfp_t flags;
    struct siwifi_bfmer_report *bfm_report ;
    if (in_softirq())
        flags = GFP_ATOMIC;
    else
        flags = GFP_KERNEL;
    bfm_report = siwifi_kmalloc(sizeof(*bfm_report) + length, flags);
    if (!bfm_report) {
        return -1;
    }
    bfm_report->length = length;
    bfm_report->dma_addr = dma_map_single(siwifi_hw->dev, &bfm_report->report[0],
                                          length, DMA_FROM_DEVICE);
    if (dma_mapping_error(siwifi_hw->dev, bfm_report->dma_addr)) {
        siwifi_kfree(bfm_report);
        return -1;
    }
    siwifi_sta->bfm_report = bfm_report;
    return 0;
}
void siwifi_bfmer_report_del(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta)
{
    if (siwifi_sta->bfm_report) {
        struct siwifi_bfmer_report *bfm_report = siwifi_sta->bfm_report;
        dma_unmap_single(siwifi_hw->dev, bfm_report->dma_addr,
                         bfm_report->length, DMA_BIDIRECTIONAL);
        siwifi_kfree(bfm_report);
        siwifi_sta->bfm_report = NULL;
    }
}
u8 siwifi_bfmer_get_rx_nss(const struct ieee80211_vht_cap *vht_capa)
{
    int i;
    u8 rx_nss = 0;
    u16 rx_mcs_map = le16_to_cpu(vht_capa->supp_mcs.rx_mcs_map);
    for (i = 7; i >= 0; i--) {
        u8 mcs = (rx_mcs_map >> (2 * i)) & 3;
        if (mcs != IEEE80211_VHT_MCS_NOT_SUPPORTED) {
            rx_nss = i + 1;
            break;
        }
    }
    return rx_nss;
}
