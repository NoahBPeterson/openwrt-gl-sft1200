#include <linux/dma-mapping.h>
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>
#include <net/ieee80211_radiotap.h>
#include <linux/vmstat.h>
#ifdef CONFIG_WIFI_RX_THREAD
#include <linux/kthread.h>
#endif
#include "siwifi_defs.h"
#include "siwifi_rx.h"
#include "siwifi_tx.h"
#include "siwifi_prof.h"
#include "ipc_host.h"
#include "siwifi_events.h"
#include "siwifi_compat.h"
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
#include "siwifi_ioctl.h"
#endif
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
#include "sf_hnat.h"
#endif
#ifdef CONFIG_VDR_HW
#include "hw_interface.h"
#endif
#ifdef CONFIG_SF19A28_WIFI_LED
#include "siwifi_led.h"
#endif
#ifdef CONFIG_SIWIFI_IGMP
#include "siwifi_igmp.h"
#endif
#include "siwifi_mem.h"
const u8 legrates_lut[] = {
    0,
    1,
    2,
    3,
    -1,
    -1,
    -1,
    -1,
    10,
    8,
    6,
    4,
    11,
    9,
    7,
    5
};
struct vendor_radiotap_hdr {
    u8 oui[3];
    u8 subns;
    u16 len;
    u8 data[];
};
static bool siwifi_rx_vip_frame(struct sk_buff *skb)
{
    struct ethhdr *eth;
    skb_reset_mac_header(skb);
    eth = (struct ethhdr *)(skb->data);
    if (siwifi_check_skb_is_dhcp(skb)) {
        printk("%s DHCP: from [%02x:%02x:%02x:%02x:%02x:%02x] to [%02x:%02x:%02x:%02x:%02x:%02x]\n", __func__, eth->h_source[0], eth->h_source[1],
                eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5], eth->h_dest[0], eth->h_dest[1],
                eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        return true;
    }
    if(eth->h_proto == cpu_to_be16(ETH_P_PAE)) {
        printk("%s EAPOL: from [%02x:%02x:%02x:%02x:%02x:%02x] to [%02x:%02x:%02x:%02x:%02x:%02x]\n", __func__, eth->h_source[0], eth->h_source[1],
                eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5], eth->h_dest[0], eth->h_dest[1],
                eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        return true;
    }
    return false;
}
void siwifi_rx_sta_stats(struct siwifi_sta *sta, struct hw_rxhdr *rxhdr)
{
 struct rx_vector_1 *rx_vect1 = &rxhdr->hwvect.rx_vect1;
 uint8_t nss = 0;
 uint8_t format_mod = 0;
 uint8_t mcs = rx_vect1->mcs;
 uint8_t sgi = rx_vect1->short_gi;
 uint8_t bw = rx_vect1->ch_bw;
 nss = rx_vect1->stbc ? (rx_vect1->n_sts / 2) : rx_vect1->n_sts;
 format_mod = rx_vect1->format_mod;
 if (!sta->stats.data_rssi_old[0]){
  sta->stats.data_rssi_min = rx_vect1->rssi1;
  sta->stats.data_rssi_max = rx_vect1->rssi1;
  sta->stats.data_rssi_sum = 0;
 }
 if (sta->stats.data_rssi_min > rx_vect1->rssi1)
  sta->stats.data_rssi_min = rx_vect1->rssi1;
 if (sta->stats.data_rssi_max < rx_vect1->rssi1)
  sta->stats.data_rssi_max = rx_vect1->rssi1;
 if (sta->stats.count > 255)
  sta->stats.count = 0;
 if (!sta->stats.data_rssi_old[255])
 {
  sta->stats.data_rssi_sum += rx_vect1->rssi1;
  sta->stats.data_rssi_avg = sta->stats.data_rssi_sum / (sta->stats.count + 1);
 } else {
  sta->stats.data_rssi_sum += (rx_vect1->rssi1 - sta->stats.data_rssi_old[sta->stats.count]);
  sta->stats.data_rssi_avg = sta->stats.data_rssi_sum >> 8;
 }
 sta->stats.leg_rate = rx_vect1->leg_rate;
 sta->stats.data_rssi_old[sta->stats.count] = rx_vect1->rssi1;
 sta->stats.data_rssi = rx_vect1->rssi1;
 sta->stats.data_rx_bw = bw;
 sta->stats.data_rx_mcs = mcs;
 sta->stats.data_rx_nss = nss;
 sta->stats.format_mod = format_mod;
 sta->stats.short_gi = sgi;
 sta->stats.count++;
}
static inline
struct siwifi_vif *siwifi_rx_get_vif(struct siwifi_hw *siwifi_hw, int vif_idx)
{
    struct siwifi_vif *siwifi_vif = NULL;
    if (vif_idx < NX_VIRT_DEV_MAX) {
        siwifi_vif = siwifi_hw->vif_table[vif_idx];
        if (!siwifi_vif || !siwifi_vif->up)
            return NULL;
    }
    return siwifi_vif;
}
static void siwifi_rx_statistic(struct siwifi_hw *siwifi_hw, struct hw_rxhdr *hw_rxhdr,
                              struct siwifi_sta *sta)
{
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    struct siwifi_stats *stats = &siwifi_hw->stats;
    struct siwifi_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
    struct rx_vector_1 *rxvect = &hw_rxhdr->hwvect.rx_vect1;
    int mpdu, ampdu, mpdu_prev, rate_idx;
    sta->stats.last_rx = hw_rxhdr->hwvect;
    mpdu = hw_rxhdr->hwvect.mpdu_cnt;
    ampdu = hw_rxhdr->hwvect.ampdu_cnt;
    mpdu_prev = stats->ampdus_rx_map[ampdu];
    if (mpdu_prev < mpdu) {
        stats->ampdus_rx_miss += mpdu - mpdu_prev - 1;
    } else {
#if DEBUG_ARRAY_CHECK
        BUG_ON(mpdu_prev >= IEEE80211_MAX_AMPDU_BUF_HT);
#endif
        stats->ampdus_rx[mpdu_prev]++;
    }
    stats->ampdus_rx_map[ampdu] = mpdu;
    if (rxvect->fec_coding)
        stats->ldpc_rx++;
    else
        stats->bcc_rx++;
    if (rxvect->stbc)
        stats->stbc_rx++;
    if (!rate_stats->size)
        return;
    if (rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) {
        int mcs;
        int bw = rxvect->ch_bw;
        int sgi;
        int nss;
        switch (rxvect->format_mod) {
            case FORMATMOD_HT_MF:
            case FORMATMOD_HT_GF:
                mcs = rxvect->mcs % 8;
                nss = rxvect->mcs / 8;
                sgi = rxvect->short_gi;
                rate_idx = 16 + nss * 32 + mcs * 4 + bw * 2 + sgi;
                break;
            case FORMATMOD_VHT:
                mcs = rxvect->mcs & 0x0F;
                nss = rxvect->stbc ? rxvect->n_sts/2 : rxvect->n_sts;
                sgi = rxvect->short_gi;
                rate_idx = 144 + nss * 80 + mcs * 8 + bw * 2 + sgi;
                break;
            default:
                return;
                break;
        }
    } else {
        int idx = legrates_lut[rxvect->leg_rate];
        if (idx < 4) {
            rate_idx = idx * 2 + rxvect->pre_type;
        } else {
            rate_idx = 8 + idx - 4;
        }
    }
    if (rate_idx < rate_stats->size) {
        rate_stats->table[rate_idx]++;
        rate_stats->cpt++;
    } else {
        wiphy_err(siwifi_hw->wiphy, "RX: Invalid index conversion => %d/%d\n",
                  rate_idx, rate_stats->size);
    }
#endif
}
const unsigned char rfc1042_header_local[] __aligned(2) =
 { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
const unsigned char bridge_tunnel_header_local[] __aligned(2) =
 { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };
static bool siwifi_single_amsdu_to_8023s(struct sk_buff *frame, struct sk_buff_head *list)
{
    u8 *payload;
    u16 ethertype;
    struct ethhdr eth;
    memcpy(&eth, frame->data, sizeof(eth));
#ifdef CONFIG_FA_PATCH
    if (ether_addr_equal(eth.h_dest, rfc1042_header_local))
        return false;
#endif
    payload = frame->data + ETH_HLEN;
    ethertype = (payload[6] << 8) | payload[7];
    if (likely((ether_addr_equal(payload, rfc1042_header_local) &&
                    ethertype != ETH_P_AARP && ethertype != ETH_P_IPX) ||
                ether_addr_equal(payload, bridge_tunnel_header_local))) {
        eth.h_proto = htons(ethertype);
        skb_pull(frame, ETH_ALEN + 2);
        memcpy(frame->data, &eth, sizeof(eth));
    }
    __skb_queue_tail(list, frame);
    return true;
}
static bool siwifi_rx_get_skb_list(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                                    struct sk_buff *skb, struct hw_rxhdr *rxhdr,
                                    struct sk_buff **amsdu_skb, struct sk_buff_head *list)
{
    bool amsdu = rxhdr->flags_is_amsdu;
    __skb_queue_head_init(list);
    if (amsdu) {
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        int count;
#endif
#ifdef CFG_DEAGG
        int i, len, subframe_len;
        struct ethhdr *eth;
        if (!siwifi_single_amsdu_to_8023s(skb, list))
            return false;
        for (i = 0; i < RX_AMSDU_CNT_MAX - 1; i++) {
            if (!amsdu_skb[i])
                break;
            amsdu_skb[i]->dev = siwifi_vif->ndev;
            amsdu_skb[i]->priority = 256 + rxhdr->flags_user_prio;
            eth = (struct ethhdr *)(amsdu_skb[i]->data);
            len = ntohs(eth->h_proto);
            subframe_len = sizeof(struct ethhdr) + len;
            if (len < 0 || subframe_len > siwifi_hw->ipc_env->rx_bufsz || (amsdu_skb[i]->tail + subframe_len) > amsdu_skb[i]->end) {
                dev_err(siwifi_hw->dev, "Invalid skb len %d for amsdu[%d] tail %p end %p\n",
                        subframe_len, i, amsdu_skb[i]->tail, amsdu_skb[i]->end);
                return false;
            }
            skb_put(amsdu_skb[i], subframe_len);
            if (!siwifi_single_amsdu_to_8023s(amsdu_skb[i], list))
                return false;
        }
#else
        ieee80211_amsdu_to_8023s(skb, list, siwifi_vif->ndev->dev_addr,
                SIWIFI_VIF_TYPE(siwifi_vif), 0, NULL, NULL);
#endif
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        count = skb_queue_len(list);
        if (count > ARRAY_SIZE(siwifi_hw->stats.amsdus_rx))
            count = ARRAY_SIZE(siwifi_hw->stats.amsdus_rx);
        siwifi_hw->stats.amsdus_rx[count - 1]++;
#endif
    } else {
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->stats.amsdus_rx[0]++;
#endif
        __skb_queue_head(list, skb);
    }
    return true;
}
#ifdef CONFIG_WIFI_RX_THREAD
static bool siwifi_rx_data_skb(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                            struct sk_buff *skb, struct hw_rxhdr *rxhdr, struct sk_buff **amsdu_skb)
{
    struct siwifi_sta *sta = NULL;
    bool resend = false, forward = true;
    struct sk_buff *rx_skb = NULL;
    struct rx_skb_element *rx_elt = NULL;
    uint8_t skip_after_eth_hdr = 0;
    skb->dev = siwifi_vif->ndev;
    rx_elt = (struct rx_skb_element *)siwifi_kmalloc(sizeof(struct rx_skb_element), GFP_ATOMIC);
    if (!rx_elt) {
        printk("oom, can not alloc rx_skb_element\n");
        return false;
    }
    if (!siwifi_rx_get_skb_list(siwifi_hw, siwifi_vif,
                skb, rxhdr, amsdu_skb, &rx_elt->skb_list)) {
        siwifi_kfree(rx_elt);
        return false;
    }
    if (((SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_AP) ||
         (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_AP_VLAN) ||
         (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_P2P_GO)) &&
        !(siwifi_vif->ap.flags & SIWIFI_AP_ISOLATE)) {
        const struct ethhdr *eth;
        rx_skb = skb_peek(&rx_elt->skb_list);
        skb_reset_mac_header(rx_skb);
        eth = eth_hdr(rx_skb);
        if (unlikely(is_multicast_ether_addr(eth->h_dest))) {
            resend = true;
        } else {
            if (rxhdr->flags_dst_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX))
            {
                sta = &siwifi_hw->sta_table[rxhdr->flags_dst_idx];
                if (sta->valid && (sta->vlan_idx == siwifi_vif->vif_index))
                {
                    forward = false;
                    resend = true;
                }
            }
        }
    } else if (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_MESH_POINT) {
        const struct ethhdr *eth;
        rx_skb = skb_peek(&rx_elt->skb_list);
        skb_reset_mac_header(rx_skb);
        eth = eth_hdr(rx_skb);
        if (rxhdr->flags_dst_idx != SIWIFI_INVALID_STA) {
            if (is_multicast_ether_addr(eth->h_dest)) {
                uint8_t *mesh_ctrl = (uint8_t *)(eth + 1);
                skip_after_eth_hdr = 8 + 6;
                if ((*mesh_ctrl & MESH_FLAGS_AE) == MESH_FLAGS_AE_A4)
                    skip_after_eth_hdr += ETH_ALEN;
                else if ((*mesh_ctrl & MESH_FLAGS_AE) == MESH_FLAGS_AE_A5_A6)
                    skip_after_eth_hdr += 2 * ETH_ALEN;
            } else {
                forward = false;
                resend = true;
            }
        }
#if 0
        if (!is_multicast_ether_addr(eth->h_dest)) {
            if (rxhdr->flags_dst_idx != SIWIFI_INVALID_STA)
            {
                forward = false;
                resend = true;
            }
        }
#endif
    }
    if (!sta && (rxhdr->flags_sta_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX))) {
        sta = &siwifi_hw->sta_table[rxhdr->flags_sta_idx];
    } else if (rxhdr->flags_sta_idx != SIWIFI_INVALID_STA) {
        printk("wtf, invalid flags_sta_idx passed from lmac, %d\n", rxhdr->flags_sta_idx);
    }
    rx_elt->vif = siwifi_vif;
    rx_elt->sta = sta;
    rx_elt->rxhdr = rxhdr;
    rx_elt->forward = forward;
    rx_elt->resend = resend;
    rx_elt->skip_len = skip_after_eth_hdr;
    spin_lock_bh(&siwifi_hw->rx_lock);
    list_add_tail(&rx_elt->list, &siwifi_hw->rx_skbs);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    siwifi_hw->rx_thread_dbg.elt_cur_cnt ++;
    if (siwifi_hw->rx_thread_dbg.elt_cur_cnt > siwifi_hw->rx_thread_dbg.elt_max_cnt) {
        siwifi_hw->rx_thread_dbg.elt_max_cnt = siwifi_hw->rx_thread_dbg.elt_cur_cnt;
    }
    siwifi_hw->rx_thread_dbg.skb_cur_cnt += skb_queue_len(&rx_elt->skb_list);
    if (skb_queue_len(&rx_elt->skb_list) > siwifi_hw->rx_thread_dbg.skb_max_per_elt) {
        siwifi_hw->rx_thread_dbg.skb_max_per_elt = skb_queue_len(&rx_elt->skb_list);
    }
    if (siwifi_hw->rx_thread_dbg.skb_cur_cnt > siwifi_hw->rx_thread_dbg.skb_max_cnt) {
        siwifi_hw->rx_thread_dbg.skb_max_cnt = siwifi_hw->rx_thread_dbg.skb_cur_cnt;
    }
#endif
    spin_unlock_bh(&siwifi_hw->rx_lock);
    atomic_inc(&siwifi_hw->rx_pending_count);
    wake_up_all(&siwifi_hw->rx_wq);
    return true;
}
#else
#define SF_HNAT_DEV_OFFSET 0
#define SF_HNAT_FLAG 47
#define SF_CB_HNAT_FORWARD 22
static bool siwifi_rx_data_skb(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                            struct sk_buff *skb, struct hw_rxhdr *rxhdr, struct sk_buff **amsdu_skb)
{
    struct sk_buff_head list;
    struct sk_buff *rx_skb;
    bool resend = false, forward = true;
 struct siwifi_sta *sta = NULL;
 struct siwifi_sta *dst_sta = NULL;
    uint32_t tx_dropped = 0;
    uint32_t tx_errors = 0;
    uint32_t forward_len = 0;
    uint32_t forward_num = 0;
    uint8_t skip_after_eth_hdr = 0;
    skb->dev = siwifi_vif->ndev;
    if (!siwifi_rx_get_skb_list(siwifi_hw, siwifi_vif,
                skb, rxhdr, amsdu_skb, &list))
        return false;
    while (!skb_queue_empty(&list)) {
        struct ethhdr *eth = NULL;
        resend = false;
        forward = true;
        dst_sta = NULL;
        rx_skb = __skb_dequeue(&list);
        skb_reset_mac_header(rx_skb);
        eth = eth_hdr(rx_skb);
        if (((SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_AP) ||
             (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_AP_VLAN) ||
             (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_P2P_GO)) &&
            !(siwifi_vif->ap.flags & SIWIFI_AP_ISOLATE)) {
            if (unlikely(is_multicast_ether_addr(eth->h_dest))) {
                resend = true;
            } else {
                dst_sta = siwifi_get_sta(siwifi_hw, eth->h_dest);
                if (dst_sta && dst_sta->valid && (dst_sta->vlan_idx == siwifi_vif->vif_index))
                {
                    forward = false;
                    resend = true;
                }
            }
        } else if (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_MESH_POINT) {
            if (is_multicast_ether_addr(eth->h_dest)) {
                uint8_t *mesh_ctrl = (uint8_t *)(eth + 1);
                skip_after_eth_hdr = 8 + 6;
                if ((*mesh_ctrl & MESH_FLAGS_AE) == MESH_FLAGS_AE_A4)
                    skip_after_eth_hdr += ETH_ALEN;
                else if ((*mesh_ctrl & MESH_FLAGS_AE) == MESH_FLAGS_AE_A5_A6)
                    skip_after_eth_hdr += 2 * ETH_ALEN;
            } else {
                forward = false;
                resend = true;
            }
        }
        sta = siwifi_get_sta(siwifi_hw, eth->h_source);
        if (sta != NULL && sta->valid) {
            siwifi_rx_sta_stats(sta, rxhdr);
        }
        {
            if (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_STATION) {
                struct ethhdr *sf_eth = NULL;
                struct siwifi_src_filter *src_filter = NULL;
                skb_reset_mac_header(rx_skb);
                sf_eth = eth_hdr(rx_skb);
                src_filter = siwifi_src_filter_hash_search(siwifi_vif, sf_eth->h_source);
                if (src_filter) {
                    src_filter->drop_count++;
                    dev_kfree_skb(rx_skb);
                    continue;
                }
            }
        }
#ifdef CONFIG_VDR_HW
        if (sta != NULL && sta->valid) {
            vendor_hook_rxdata(sta, rx_skb, rxhdr);
        }
#endif
        if (resend) {
            struct sk_buff *skb_copy;
            int res;
#if 0
            skb_copy = skb_copy_expand(rx_skb, sizeof(struct siwifi_txhdr) +
                                       SIWIFI_SWTXHDR_ALIGN_SZ, 0, GFP_ATOMIC);
            if (!skb_copy) {
                netdev_err(siwifi_vif->ndev, "Failed to copy skb");
                goto FORWARD;
            }
#else
            if (forward) {
                skb_copy = skb_clone(rx_skb, GFP_ATOMIC);
                if (!skb_copy) {
                    netdev_err(siwifi_vif->ndev, "Failed to copy skb");
                    goto FORWARD;
                }
            } else {
                skb_copy = rx_skb;
            }
#endif
            skb_copy->protocol = htons(ETH_P_802_3);
            skb_reset_network_header(skb_copy);
            skb_reset_mac_header(skb_copy);
            if (siwifi_vif)
                siwifi_vif->is_resending = true;
            spin_unlock(&siwifi_hw->cb_lock);
            res = dev_queue_xmit(skb_copy);
            spin_lock(&siwifi_hw->cb_lock);
            if (siwifi_vif)
                siwifi_vif->is_resending = false;
            if (res == NET_XMIT_DROP) {
                tx_dropped ++;
            } else if (res != NET_XMIT_SUCCESS) {
                tx_errors ++;
            }
        }
FORWARD:
        if (forward) {
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
            struct sf_hnat_priv *phnat_priv;
            struct platform_device *hnat_pdev;
#endif
            skb_reset_mac_header(rx_skb);
            memset(rx_skb->cb, 0, sizeof(rx_skb->cb));
            siwifi_trace_rx_in(siwifi_hw, sta, rx_skb);
            rx_skb->protocol = eth_type_trans(rx_skb, siwifi_vif->ndev);
            if (unlikely(skip_after_eth_hdr)) {
                memmove(skb_mac_header(rx_skb) + skip_after_eth_hdr,
                        skb_mac_header(rx_skb), sizeof(struct ethhdr));
                __skb_pull(rx_skb, skip_after_eth_hdr);
                skb_reset_mac_header(rx_skb);
                skip_after_eth_hdr = 0;
            }
#ifdef CONFIG_SIWIFI_IGMP
            siwifi_check_igmp(siwifi_hw, rx_skb);
#endif
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
            phnat_priv = siwifi_vif->phnat_priv;
            hnat_pdev = siwifi_vif->hnat_pdev;
            if (phnat_priv) {
                phnat_priv->search(hnat_pdev, rx_skb);
            }
#endif
            spin_unlock(&siwifi_hw->cb_lock);
            REG_SW_SET_PROFILING(siwifi_hw, SW_PROF_IEEE80211RX);
            forward_num ++;
            forward_len += rx_skb->len;
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
            if(rx_skb->cb[SF_HNAT_FLAG] == SF_CB_HNAT_FORWARD){
                rx_skb->dev = (struct net_device *)(*((unsigned int*)(rx_skb->cb + SF_HNAT_DEV_OFFSET)));
                skb_push(rx_skb, ETH_HLEN);
                rx_skb->dev->netdev_ops->ndo_start_xmit(rx_skb, rx_skb->dev);
            }else
#endif
            {
                netif_receive_skb(rx_skb);
            }
            REG_SW_CLEAR_PROFILING(siwifi_hw, SW_PROF_IEEE80211RX);
   spin_lock(&siwifi_hw->cb_lock);
  }
        siwifi_hw->total_rx ++;
        siwifi_vif->total_rx ++;
 }
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    siwifi_hw->stats.total_rx += forward_num;
#endif
    if (siwifi_vif) {
        if (tx_dropped) {
            siwifi_vif->net_stats.rx_dropped += tx_dropped;
            siwifi_vif->net_stats.tx_dropped += tx_dropped;
        }
        if (tx_errors) {
            siwifi_vif->net_stats.tx_errors += tx_errors;
        }
        siwifi_vif->net_stats.rx_packets += forward_num;
        siwifi_vif->net_stats.rx_bytes += forward_len;
        if (sta != NULL && sta->valid) {
            sta->stats.rx_packets += forward_num;
            sta->stats.rx_bytes += forward_len;
            if (tx_dropped) {
                sta->stats.rx_dropped += tx_dropped;
                if(dst_sta)
                    dst_sta->stats.tx_dropped += tx_dropped;
            }
            if (tx_errors && dst_sta)
                dst_sta->stats.tx_failed += tx_errors;
        }
    }
    return (forward || resend);
}
#endif
#ifdef CONFIG_SIWIFI_EASYMESH
static void siwifi_easymesh_mgmt_rx_hook(struct ieee80211_mgmt *mgmt, struct sk_buff *skb)
{
    sf_notify_event_data event_data;
    bool matched = false;
    if (ieee80211_is_assoc_req(mgmt->frame_control)) {
        event_data.data.mgmt_rx_event.frame_type = SF_TUNNELED_MSG_ASSOC_REQ;
        matched = true;
    } else if (ieee80211_is_reassoc_req(mgmt->frame_control)) {
        event_data.data.mgmt_rx_event.frame_type = SF_TUNNELED_MSG_REASSOC_REQ;
        matched = true;
    } else {
        if (ieee80211_is_action(mgmt->frame_control)) {
            u8 action_code = mgmt->u.action.u.wme_action.action_code;
            switch (mgmt->u.action.category) {
                case WLAN_CATEGORY_WNM:
                    if (action_code == WLAN_WNM_NOTIFY_REQ) {
                        event_data.data.mgmt_rx_event.frame_type = SF_TUNNELED_MSG_WNM_REQ;
                        matched = true;
                    } else if (action_code == WLAN_WNM_BTM_QUERY) {
                        event_data.data.mgmt_rx_event.frame_type = SF_TUNNELED_MSG_BTM_QUERY;
                        matched = true;
                    }
                    break;
                case WLAN_CATEGORY_PUBLIC:
                    if (action_code == WLAN_PA_GAS_INITIAL_REQ) {
                        event_data.data.mgmt_rx_event.frame_type = SF_TUNNELED_MSG_ANQP_NEIGHBOR_REQ;
                        matched = true;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    if (matched) {
        event_data.type = SF_NOTIFY_MGMT_RX_EVENT;
        event_data.data.mgmt_rx_event.frame_length = skb->len;
        event_data.data.mgmt_rx_event.frame_data = kmalloc(skb->len, GFP_ATOMIC);
        if (!event_data.data.mgmt_rx_event.frame_data) {
            printk("Failed to allocate memory for management frame data.\n");
            return;
        }
        if (skb_copy_bits(skb, 0, (void *)event_data.data.mgmt_rx_event.frame_data, skb->len) != 0) {
            printk("Failed to copy management frame data.\n");
            kfree(event_data.data.mgmt_rx_event.frame_data);
            return;
        }
        report_sf_notify_event(&event_data);
        kfree(event_data.data.mgmt_rx_event.frame_data);
    }
}
#endif
static void siwifi_rx_mgmt(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                         struct sk_buff *skb, struct hw_rxhdr *hw_rxhdr)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;
    struct rx_vector_1 *rxvect = &hw_rxhdr->hwvect.rx_vect1;
    siwifi_trace_mgmt_rx_in(siwifi_hw, skb);
#ifdef CONFIG_SIWIFI_EASYMESH
    siwifi_easymesh_mgmt_rx_hook(mgmt, skb);
#endif
    if (siwifi_hw->enable_dbg_sta_conn) {
        if (ieee80211_is_auth(mgmt->frame_control)) {
            printk("rcv auth from [%pM] status code %d\n", mgmt->sa, mgmt->u.auth.status_code);
        } else if (ieee80211_is_assoc_req(mgmt->frame_control)) {
            printk("rcv assoc_req from [%pM]\n", mgmt->sa);
        } else if (ieee80211_is_assoc_resp(mgmt->frame_control)) {
            printk("rcv assoc_resp from [%pM] status code %d\n", mgmt->sa, mgmt->u.assoc_resp.status_code);
        } else if (ieee80211_is_reassoc_resp(mgmt->frame_control)) {
            printk("rcv reassoc_resp from [%pM] status code %d\n", mgmt->sa, mgmt->u.reassoc_resp.status_code);
        } else if (ieee80211_is_deauth(mgmt->frame_control)) {
            printk("rcv deauth from [%pM] reasoncode: %d\n", mgmt->sa, mgmt->u.deauth.reason_code);
        } else if (ieee80211_is_disassoc(mgmt->frame_control)) {
            printk("rcv disassoc from [%pM] reasoncode: %d\n", mgmt->sa, mgmt->u.disassoc.reason_code);
        }
    }
    if (ieee80211_is_deauth(mgmt->frame_control) ||
                ieee80211_is_disassoc(mgmt->frame_control)){
       printk("rx pkt(%d) %s from [%02x:%02x:%02x:%02x:%02x:%02x] reasoncode: %d\n", siwifi_hw->mod_params->is_hb, ieee80211_is_deauth(mgmt->frame_control) ? "deauth" : "disassoc",
               mgmt->sa[0], mgmt->sa[1], mgmt->sa[2], mgmt->sa[3], mgmt->sa[4], mgmt->sa[5], mgmt->u.deauth.reason_code);
    }
    if (ieee80211_is_beacon(mgmt->frame_control)) {
        if ((SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_MESH_POINT) &&
            hw_rxhdr->flags_new_peer) {
            cfg80211_notify_new_peer_candidate(siwifi_vif->ndev, mgmt->sa,
                                               mgmt->u.beacon.variable,
                                               skb->len - offsetof(struct ieee80211_mgmt,
                                                                   u.beacon.variable),
                                               0, GFP_ATOMIC);
        } else {
            cfg80211_report_obss_beacon(siwifi_hw->wiphy, skb->data, skb->len,
                                        hw_rxhdr->phy_info.phy_prim20_freq,
                                        rxvect->rssi1);
        }
        siwifi_vif->mgmt_stats.rx_bcn_cnts++;
    } else if ((ieee80211_is_deauth(mgmt->frame_control) ||
                ieee80211_is_disassoc(mgmt->frame_control)) &&
               (mgmt->u.deauth.reason_code == WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA ||
                mgmt->u.deauth.reason_code == WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA)) {
        cfg80211_rx_unprot_mlme_mgmt(siwifi_vif->ndev, skb->data, skb->len);
        siwifi_vif->mgmt_stats.rx_non_cnts++;
    } else if ((SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_STATION) &&
               (ieee80211_is_action(mgmt->frame_control) &&
                (mgmt->u.action.category == 6))) {
        struct cfg80211_ft_event_params ft_event;
        ft_event.target_ap = (uint8_t *)&mgmt->u.action + ETH_ALEN + 2;
        ft_event.ies = (uint8_t *)&mgmt->u.action + ETH_ALEN * 2 + 2;
        ft_event.ies_len = skb->len - (ft_event.ies - (uint8_t *)mgmt);
        ft_event.ric_ies = NULL;
        ft_event.ric_ies_len = 0;
        cfg80211_ft_event(siwifi_vif->ndev, &ft_event);
        siwifi_vif->mgmt_stats.rx_ft_cnts++;
    } else {
        cfg80211_rx_mgmt(&siwifi_vif->wdev, hw_rxhdr->phy_info.phy_prim20_freq,
                         rxvect->rssi1, skb->data, skb->len, 0);
        siwifi_vif->mgmt_stats.rx_mgmt_cnts++;
    }
}
static void siwifi_rx_mgmt_any(struct siwifi_hw *siwifi_hw, struct sk_buff *skb,
                             struct hw_rxhdr *hw_rxhdr)
{
    struct siwifi_vif *siwifi_vif;
    int vif_idx = hw_rxhdr->flags_vif_idx;
    trace_mgmt_rx(hw_rxhdr->phy_info.phy_prim20_freq, vif_idx,
                  hw_rxhdr->flags_sta_idx, (struct ieee80211_mgmt *)skb->data);
    if (vif_idx == SIWIFI_INVALID_VIF) {
        list_for_each_entry(siwifi_vif, &siwifi_hw->vifs, list) {
            if (! siwifi_vif->up)
                continue;
            siwifi_rx_mgmt(siwifi_hw, siwifi_vif, skb, hw_rxhdr);
        }
    } else {
        siwifi_vif = siwifi_rx_get_vif(siwifi_hw, vif_idx);
        if (siwifi_vif)
            siwifi_rx_mgmt(siwifi_hw, siwifi_vif, skb, hw_rxhdr);
    }
    dev_kfree_skb(skb);
}
static u8 siwifi_rx_rtap_hdrlen(struct rx_vector_1 *rxvect,
                              bool has_vend_rtap)
{
    u8 rtap_len;
    rtap_len = sizeof(struct ieee80211_radiotap_header) + 8;
    if (hweight32(rxvect->antenna_set) > 1)
        rtap_len += 4 * hweight8(rxvect->antenna_set);
    if (!has_vend_rtap) {
        rtap_len = ALIGN(rtap_len, 8);
        rtap_len += 8;
    }
    rtap_len++;
    if (hweight32(rxvect->antenna_set) == 1)
        rtap_len++;
    rtap_len = ALIGN(rtap_len, 2);
    if ((rxvect->format_mod == FORMATMOD_HT_MF) ||
        (rxvect->format_mod == FORMATMOD_HT_GF))
        rtap_len += 3;
    if (!(has_vend_rtap) && ((rxvect->format_mod >= FORMATMOD_VHT) ||
                             ((rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) &&
                                                     (rxvect->aggregation)))) {
        rtap_len = ALIGN(rtap_len, 4);
        rtap_len += 8;
    }
    if (rxvect->format_mod == FORMATMOD_VHT) {
        rtap_len = ALIGN(rtap_len, 2);
        rtap_len += 12;
    }
    if (rxvect->format_mod == FORMATMOD_HE_SU) {
        rtap_len = ALIGN(rtap_len, 2);
        rtap_len += sizeof(struct ieee80211_radiotap_he);
    }
    if (hweight32(rxvect->antenna_set) > 1) {
        rtap_len += 2 * hweight8(rxvect->antenna_set);
    }
    if (has_vend_rtap) {
        rtap_len += 4;
        rtap_len = ALIGN(rtap_len, 2);
    }
    return rtap_len;
}
static void siwifi_rx_add_rtap_hdr(struct siwifi_hw* siwifi_hw,
                                 struct sk_buff *skb,
                                 struct rx_vector_1 *rxvect,
                                 struct phy_channel_info_desc *phy_info,
                                 struct hw_vect *hwvect,
                                 int rtap_len,
                                 u8 vend_rtap_len,
                                 u32 vend_it_present)
{
    struct ieee80211_radiotap_header *rtap;
    u8 *pos, rate_idx;
    __le32 *it_present;
    u32 it_present_val = 0;
    bool fec_coding = false;
    bool short_gi = false;
    bool stbc = false;
    bool aggregation = false;
    rtap = (struct ieee80211_radiotap_header *)skb_push(skb, rtap_len);
    memset((u8*) rtap, 0, rtap_len);
    rtap->it_version = 0;
    rtap->it_pad = 0;
    rtap->it_len = cpu_to_le16(rtap_len + vend_rtap_len);
    it_present = &rtap->it_present;
    if (hweight32(rxvect->antenna_set) > 1) {
        int chain;
        unsigned long chains = rxvect->antenna_set;
        for_each_set_bit(chain, &chains, IEEE80211_MAX_CHAINS) {
            it_present_val |=
                BIT(IEEE80211_RADIOTAP_EXT) |
                BIT(IEEE80211_RADIOTAP_RADIOTAP_NAMESPACE);
            put_unaligned_le32(it_present_val, it_present);
            it_present++;
            it_present_val = BIT(IEEE80211_RADIOTAP_ANTENNA) |
                             BIT(IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
        }
    }
    if (vend_rtap_len) {
        it_present_val |= BIT(IEEE80211_RADIOTAP_VENDOR_NAMESPACE) |
                          BIT(IEEE80211_RADIOTAP_EXT);
        put_unaligned_le32(it_present_val, it_present);
        it_present++;
        it_present_val = vend_it_present;
    }
    put_unaligned_le32(it_present_val, it_present);
    pos = (void *)(it_present + 1);
    if (hwvect) {
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_TSFT);
        while ((pos - (u8 *)rtap) & 7)
            *pos++ = 0;
        put_unaligned_le64((((u64)le32_to_cpu(hwvect->tsf_hi) << 32) +
                            (u64)le32_to_cpu(hwvect->tsf_lo)), pos);
        pos += 8;
    }
    rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_FLAGS);
    if (hwvect && (!hwvect->frm_successful_rx))
        *pos |= IEEE80211_RADIOTAP_F_BADFCS;
    if (!rxvect->pre_type
            && (rxvect->format_mod <= FORMATMOD_NON_HT_DUP_OFDM))
        *pos |= IEEE80211_RADIOTAP_F_SHORTPRE;
    pos++;
    if (rxvect->format_mod >= FORMATMOD_HE_SU) {
        rate_idx = rxvect->mcs;
        fec_coding = rxvect->fec_coding;
        stbc = rxvect->stbc;
        aggregation = true;
        *pos = 0;
    } else if (rxvect->format_mod == FORMATMOD_VHT) {
        rate_idx = rxvect->mcs & 0x0F;
        fec_coding = rxvect->fec_coding;
        short_gi = rxvect->short_gi;
        stbc = rxvect->stbc;
        aggregation = true;
        *pos = 0;
    } else if (rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) {
        rate_idx = rxvect->mcs;
        fec_coding = rxvect->fec_coding;
        short_gi = rxvect->short_gi;
        stbc = rxvect->stbc;
        aggregation = rxvect->aggregation;
        *pos = 0;
    } else {
        struct ieee80211_supported_band* band =
                siwifi_hw->wiphy->bands[phy_info->phy_band];
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_RATE);
        BUG_ON((rate_idx = legrates_lut[rxvect->leg_rate]) == -1);
        if (phy_info->phy_band == NL80211_BAND_5GHZ)
            rate_idx -= 4;
        *pos = DIV_ROUND_UP(band->bitrates[rate_idx].bitrate, 5);
    }
    pos++;
    rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_CHANNEL);
    put_unaligned_le16(phy_info->phy_prim20_freq, pos);
    pos += 2;
    if (phy_info->phy_band == NL80211_BAND_5GHZ)
        put_unaligned_le16(IEEE80211_CHAN_OFDM | IEEE80211_CHAN_5GHZ, pos);
    else if (rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM)
        put_unaligned_le16(IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ, pos);
    else
        put_unaligned_le16(IEEE80211_CHAN_CCK | IEEE80211_CHAN_2GHZ, pos);
    pos += 2;
    if (hweight32(rxvect->antenna_set) == 1) {
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
        *pos++ = rxvect->rssi1;
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_ANTENNA);
        *pos++ = rxvect->antenna_set;
    }
    rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_RX_FLAGS);
    if ((pos - (u8 *)rtap) & 1)
        *pos++ = 0;
    put_unaligned_le16(0, pos);
    pos += 2;
    if ((rxvect->format_mod == FORMATMOD_HT_MF)
            || (rxvect->format_mod == FORMATMOD_HT_GF)) {
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_MCS);
        *pos++ = IEEE80211_RADIOTAP_MCS_HAVE_MCS |
                 IEEE80211_RADIOTAP_MCS_HAVE_GI |
                 IEEE80211_RADIOTAP_MCS_HAVE_BW;
        *pos = 0;
        if (short_gi)
            *pos |= IEEE80211_RADIOTAP_MCS_SGI;
        if (rxvect->ch_bw == __CHBW_CBW40)
            *pos |= IEEE80211_RADIOTAP_MCS_BW_40;
        if (rxvect->format_mod == FORMATMOD_HT_GF)
            *pos |= IEEE80211_RADIOTAP_MCS_FMT_GF;
        if (fec_coding)
            *pos |= IEEE80211_RADIOTAP_MCS_FEC_LDPC;
        *pos++ |= stbc << IEEE80211_RADIOTAP_MCS_STBC_SHIFT;
        *pos++ = rate_idx;
    }
    if (aggregation && hwvect) {
        while ((pos - (u8 *)rtap) & 3)
            pos++;
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_AMPDU_STATUS);
        put_unaligned_le32(hwvect->ampdu_cnt, pos);
        pos += 4;
        put_unaligned_le32(0, pos);
        pos += 4;
    }
    if (rxvect->format_mod == FORMATMOD_VHT) {
        u16 vht_details = IEEE80211_RADIOTAP_VHT_KNOWN_GI |
                          IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH;
        u8 vht_nss = (rxvect->stbc ? rxvect->n_sts/2 : rxvect->n_sts) + 1;
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_VHT);
        if ((rxvect->ch_bw == __CHBW_CBW160)
                && phy_info->phy_center2_freq)
            vht_details &= ~IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH;
        put_unaligned_le16(vht_details, pos);
        pos += 2;
        if (short_gi)
            *pos |= IEEE80211_RADIOTAP_VHT_FLAG_SGI;
        if (stbc)
            *pos |= IEEE80211_RADIOTAP_VHT_FLAG_STBC;
        pos++;
        if (rxvect->ch_bw == __CHBW_CBW40)
            *pos++ = 1;
        if (rxvect->ch_bw == __CHBW_CBW80)
            *pos++ = 4;
        else if ((rxvect->ch_bw == __CHBW_CBW160)
                && phy_info->phy_center2_freq)
            *pos++ = 0;
        else if (rxvect->ch_bw == __CHBW_CBW160)
            *pos++ = 11;
        else
            *pos++ = 0;
        *pos = (rate_idx << 4) | vht_nss;
        pos += 4;
        if (fec_coding)
            *pos |= IEEE80211_RADIOTAP_CODING_LDPC_USER0;
        pos++;
        pos++;
        pos += 2;
    }
    if (hweight32(rxvect->antenna_set) > 1) {
        int chain;
        unsigned long chains = rxvect->antenna_set;
        u8 rssis[4] = {rxvect->rssi1, rxvect->rssi1, rxvect->rssi1, rxvect->rssi1};
        for_each_set_bit(chain, &chains, IEEE80211_MAX_CHAINS) {
            *pos++ = rssis[chain];
            *pos++ = chain;
        }
    }
}
static int siwifi_rx_monitor(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                           struct sk_buff *skb, struct hw_rxhdr *hw_rxhdr_ptr,
                           u8 rtap_len)
{
    skb->dev = siwifi_vif->ndev;
    if (siwifi_vif->wdev.iftype != NL80211_IFTYPE_MONITOR) {
        netdev_err(siwifi_vif->ndev, "not a monitor vif\n");
        return -1;
    }
    siwifi_rx_add_rtap_hdr(siwifi_hw, skb, &hw_rxhdr_ptr->hwvect.rx_vect1,
                         &hw_rxhdr_ptr->phy_info, &hw_rxhdr_ptr->hwvect,
                         rtap_len, 0, 0);
    skb_reset_mac_header(skb);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    skb->pkt_type = PACKET_OTHERHOST;
    skb->protocol = htons(ETH_P_802_2);
    netif_receive_skb(skb);
    return 0;
}
u8 siwifi_unsup_rx_vec_ind(void *pthis, void *hostid) {
    struct siwifi_hw *siwifi_hw = pthis;
    struct siwifi_ipc_skb_elem *elem = hostid;
    struct rx_vector_desc *rx_desc;
    struct sk_buff *skb;
    struct rx_vector_1 *rx_vect1;
    struct phy_channel_info_desc *phy_info;
    struct vendor_radiotap_hdr *rtap;
    u16 ht_length;
    struct siwifi_vif *siwifi_vif;
    struct rx_vector_desc rx_vect_desc;
    u8 rtap_len, vend_rtap_len = sizeof(*rtap);
    dma_sync_single_for_cpu(siwifi_hw->dev, elem->dma_addr,
                            sizeof(struct rx_vector_desc), DMA_FROM_DEVICE);
    skb = elem->skb;
    if (((struct rx_vector_desc *) (skb->data))->pattern == 0) {
        dma_sync_single_for_device(siwifi_hw->dev, elem->dma_addr,
                                     sizeof(struct rx_vector_desc), DMA_FROM_DEVICE);
        return -1;
    }
    if (siwifi_hw->monitor_vif == SIWIFI_INVALID_VIF) {
        dma_unmap_single(siwifi_hw->dev, elem->dma_addr, siwifi_hw->ipc_env->unsuprxvec_bufsz,
                         DMA_FROM_DEVICE);
        elem->skb = NULL;
        dev_kfree_skb(skb);
        if (siwifi_ipc_unsup_rx_vec_elem_allocs(siwifi_hw, elem))
            dev_err(siwifi_hw->dev, "Failed to alloc new unsupported rx vector buf\n");
        return -1;
    }
#if DEBUG_ARRAY_CHECK
    BUG_ON(siwifi_hw->monitor_vif >= NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX);
#endif
    siwifi_vif = siwifi_hw->vif_table[siwifi_hw->monitor_vif];
    skb->dev = siwifi_vif->ndev;
    memcpy(&rx_vect_desc, skb->data, sizeof(rx_vect_desc));
    rx_desc = &rx_vect_desc;
    rx_vect1 = (struct rx_vector_1 *) (rx_desc->rx_vect1);
    phy_info = (struct phy_channel_info_desc *) (&rx_desc->phy_info);
    if (rx_vect1->format_mod >= FORMATMOD_VHT)
        ht_length = 0;
    else
        ht_length = (u16) le32_to_cpu(rx_vect1->ht_length);
    skb_reserve(skb, RADIOTAP_HDR_MAX_LEN);
    rtap = (struct vendor_radiotap_hdr *) skb->data;
    rtap->oui[0] = 0x00;
    rtap->oui[1] = 0x25;
    rtap->oui[2] = 0x3A;
    rtap->subns = 0;
    rtap->len = sizeof(ht_length);
    put_unaligned_le16(ht_length, rtap->data);
    vend_rtap_len += rtap->len;
    skb_put(skb, vend_rtap_len);
    put_unaligned_le16(0, skb->data + vend_rtap_len);
    skb_put(skb, UNSUP_RX_VEC_DATA_LEN);
    rtap_len = siwifi_rx_rtap_hdrlen(rx_vect1, true);
    if (skb_headroom(skb) < rtap_len) {
        netdev_err(siwifi_vif->ndev, "not enough headroom %d need %d\n", skb_headroom(skb), rtap_len);
        dma_unmap_single(siwifi_hw->dev, elem->dma_addr, siwifi_hw->ipc_env->unsuprxvec_bufsz,
                DMA_FROM_DEVICE);
        elem->skb = NULL;
        dev_kfree_skb(skb);
        if (siwifi_ipc_unsup_rx_vec_elem_allocs(siwifi_hw, elem))
            dev_err(siwifi_hw->dev, "Failed to alloc new unsupported rx vector buf\n");
        return -1;
    }
    siwifi_rx_add_rtap_hdr(siwifi_hw, skb, rx_vect1, phy_info, NULL,
                         rtap_len, vend_rtap_len, BIT(0));
    skb_reset_mac_header(skb);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    skb->pkt_type = PACKET_OTHERHOST;
    skb->protocol = htons(ETH_P_802_2);
    dma_unmap_single(siwifi_hw->dev, elem->dma_addr, siwifi_hw->ipc_env->unsuprxvec_bufsz,
                     DMA_FROM_DEVICE);
    elem->skb = NULL;
    netif_receive_skb(skb);
    if (siwifi_ipc_unsup_rx_vec_elem_allocs(siwifi_hw, elem))
        netdev_err(siwifi_vif->ndev, "Failed to alloc new unsupported rx vector buf\n");
    return 0;
}
static void siwifi_rx_monitor_skb(struct siwifi_hw *siwifi_hw, struct sk_buff *skb, u16_l status, int msdu_offset)
{
    struct sk_buff *skb_monitor;
    struct hw_rxhdr hw_rxhdr_copy;
    struct hw_rxhdr *hw_rxhdr;
    struct siwifi_vif *siwifi_vif;
    u8 rtap_len;
    u16 frm_len;
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    siwifi_hw->stats.monitor_rx ++;
#endif
    siwifi_vif = siwifi_rx_get_vif(siwifi_hw, siwifi_hw->monitor_vif);
    if (!siwifi_vif) {
        dev_err(siwifi_hw->dev, "Received monitor frame but there is no monitor interface open\n");
        return ;
    }
    hw_rxhdr = (struct hw_rxhdr *)skb->data;
    rtap_len = siwifi_rx_rtap_hdrlen(&hw_rxhdr->hwvect.rx_vect1, false);
    skb->data += msdu_offset;
    frm_len = le32_to_cpu(hw_rxhdr->hwvect.len);
    skb->len = frm_len;
    if (status == RX_STAT_MONITOR) {
        siwifi_ipc_rxbuf_elem_pull(siwifi_hw, skb);
        if (skb_headroom(skb) > rtap_len) {
            skb_monitor = skb;
            memcpy(&hw_rxhdr_copy, hw_rxhdr, sizeof(hw_rxhdr_copy));
            hw_rxhdr = &hw_rxhdr_copy;
        } else {
            skb_monitor = skb_copy_expand(skb, rtap_len, 0, GFP_ATOMIC);
            skb->data = (void*) hw_rxhdr;
        }
    }
    else
    {
#ifdef CONFIG_SIWIFI_MON_DATA
        if (!hw_rxhdr->flags_is_80211_mpdu) {
            u16 machdr_len = hw_rxhdr->mac_hdr_backup.buf_len;
            u8* machdr_ptr = hw_rxhdr->mac_hdr_backup.buffer;
            skb_pull(skb, sizeof(struct ethhdr));
            skb_monitor = skb_copy_expand(skb,
                    rtap_len + machdr_len,
                    0, GFP_ATOMIC);
            skb_push(skb_monitor, machdr_len);
            memcpy(skb_monitor->data, machdr_ptr, machdr_len);
            frm_len += machdr_len - sizeof(struct ethhdr);
        } else {
            skb_monitor = skb_copy_expand(skb, rtap_len, 0, GFP_ATOMIC);
        }
        skb->data = (void*) hw_rxhdr;
#else
        skb->data = (void*) hw_rxhdr;
        wiphy_err(siwifi_hw->wiphy, "RX status %d is invalid when MON_DATA is disabled\n", status);
        return ;
#endif
    }
    skb_reset_tail_pointer(skb);
    skb->len = 0;
    skb_reset_tail_pointer(skb_monitor);
    skb_monitor->len = 0;
    skb_put(skb_monitor, frm_len);
    if (siwifi_rx_monitor(siwifi_hw, siwifi_vif, skb_monitor, hw_rxhdr, rtap_len))
        dev_kfree_skb(skb_monitor);
    if (status == RX_STAT_MONITOR) {
        status |= RX_STAT_ALLOC;
        if (skb_monitor != skb) {
            dev_kfree_skb(skb);
        }
    }
}
u8 siwifi_rxdataind(void *pthis, void *hostid)
{
    struct siwifi_hw *siwifi_hw = pthis;
    struct siwifi_ipc_elem *elem = hostid;
    struct hw_rxhdr *hw_rxhdr = NULL;
    struct rxdesc_tag *rxdesc;
    struct siwifi_vif *siwifi_vif;
    struct sk_buff *skb = NULL;
#ifdef CFG_DEAGG
    struct sk_buff *amsdu_skb[RX_AMSDU_CNT_MAX - 1] = { NULL };
#endif
    uint32_t rx_buff_idx;
    int msdu_offset = siwifi_hw->ipc_env->rx_pd_offset;
    int peek_len = msdu_offset + sizeof(struct ethhdr);
#ifdef CFG_DEAGG
    int i = 0;
    uint32_t rx_buff_idx_amsdu;
    int get_amsdu = 0;
    int free_skb = 0;
#endif
    u16_l status;
    u16_l amsdu_num;
    REG_SW_SET_PROFILING(siwifi_hw, SW_PROF_SIWIFIDATAIND);
    if (test_bit(SIWIFI_DEV_HW_DEAD, &siwifi_hw->drv_flags)) {
        return -1;
    }
    rxdesc = elem->addr;
    status = rxdesc->status;
    amsdu_num = rxdesc->amsdu_num;
    if (!status) {
        return -1;
    }
    rx_buff_idx = SIWIFI_RXBUFF_HOSTID_TO_IDX(rxdesc->host_id);
    if (SIWIFI_RXBUFF_VALID_IDX(rx_buff_idx))
        skb = siwifi_hw->rxbuf_elems.skb[rx_buff_idx];
    if (!skb) {
        dev_err(siwifi_hw->dev, "RX Buff invalid idx [%d]\n", rx_buff_idx);
        return -1;
    }
    if (SIWIFI_RXBUFF_PATTERN_GET(skb) != siwifi_rxbuff_pattern) {
        printk("RXBUFF_PATTERN_GET(skb): %08x, siwifi_rxbuff_pattern:%08x\n", SIWIFI_RXBUFF_PATTERN_GET(skb), siwifi_rxbuff_pattern);
        dev_err(siwifi_hw->dev, "RX Buff Pattern not correct\n");
        BUG();
    }
    spin_lock(&siwifi_hw->cb_lock);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    siwifi_hw->stats.rxind_cnts++;
#endif
    if (status & RX_STAT_DELETE) {
        if (!(status & RX_STAT_ALLOC)) {
            siwifi_hw->reord_release_cnt++;
        }
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->stats.delete_rx ++;
#endif
        siwifi_ipc_rxbuf_elem_pull(siwifi_hw, skb);
#ifdef CFG_DEAGG
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        if (hw_rxhdr->flags_is_amsdu) {
   get_amsdu = 1;
   free_skb = 1;
  } else {
            dev_kfree_skb(skb);
        }
#else
        dev_kfree_skb(skb);
#endif
        goto end;
    }
    if (unlikely(status & RX_STAT_MONITOR)) {
        siwifi_rx_monitor_skb(siwifi_hw, skb, status, msdu_offset);
    }
    if (status & RX_STAT_LEN_UPDATE) {
        dma_addr_t dma_addr;
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->stats.len_update_rx ++;
#endif
        dma_addr = SIWIFI_RXBUFF_DMA_ADDR_GET(skb);
        dma_sync_single_for_cpu(siwifi_hw->dev, dma_addr,
                                peek_len, DMA_FROM_DEVICE);
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        hw_rxhdr->hwvect.len = rxdesc->frame_len;
        if (status & RX_STAT_ETH_LEN_UPDATE) {
            struct ethhdr *hdr = (struct ethhdr *)((u8 *)hw_rxhdr + msdu_offset);
            hdr->h_proto = htons(rxdesc->frame_len - sizeof(struct ethhdr));
        }
        dma_sync_single_for_device(siwifi_hw->dev, dma_addr,
                                   peek_len, DMA_BIDIRECTIONAL);
        goto end;
    }
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
    if (siwifi_hw->ate_env.rx_frame_start)
        siwifi_ate_rx_cb_rssi(siwifi_hw, skb);
#endif
    if (unlikely(status & RX_STAT_SPURIOUS)) {
        struct ieee80211_hdr *hdr;
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->stats.spurious_rx ++;
#endif
        siwifi_ipc_rxbuf_elem_sync(siwifi_hw, skb, msdu_offset + sizeof(*hdr));
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        hdr = (struct ieee80211_hdr *)(skb->data + msdu_offset);
        siwifi_vif = siwifi_rx_get_vif(siwifi_hw, hw_rxhdr->flags_vif_idx);
        if (siwifi_vif) {
            cfg80211_rx_spurious_frame(siwifi_vif->ndev, hdr->addr2, 0, GFP_ATOMIC);
        }
        siwifi_ipc_rxbuf_elem_repush(siwifi_hw, skb);
        goto end;
    }
    if (status & RX_STAT_FORWARD) {
        siwifi_ipc_rxbuf_elem_pull_inv(siwifi_hw, skb, true);
        if (!(status & RX_STAT_ALLOC)) {
            siwifi_hw->reord_release_cnt++;
        }
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        skb_reserve(skb, msdu_offset);
#ifdef CFG_DEAGG
        if (hw_rxhdr->flags_is_amsdu) {
            int len, subframe_len;
            struct ethhdr *eth;
            eth = (struct ethhdr *)(skb->data);
            len = ntohs(eth->h_proto);
            subframe_len = sizeof(struct ethhdr) + len;
            if (len < 0 || subframe_len > siwifi_hw->ipc_env->rx_bufsz || (skb->tail + subframe_len) > skb->end) {
                dev_err(siwifi_hw->dev, "Invalid skb len %d for first amsdu tail %p end %p\n",
                        subframe_len, skb->tail, skb->end);
    get_amsdu = 1;
    free_skb = 1;
                goto check_alloc;
            }
            skb_put(skb, subframe_len);
        } else {
            skb_put(skb, le32_to_cpu(hw_rxhdr->hwvect.len));
        }
#else
        skb_put(skb, le32_to_cpu(hw_rxhdr->hwvect.len));
#endif
        if (hw_rxhdr->flags_is_80211_mpdu) {
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
            siwifi_hw->stats.mgmt_rx ++;
#endif
            siwifi_rx_mgmt_any(siwifi_hw, skb, hw_rxhdr);
        } else {
            siwifi_vif = siwifi_rx_get_vif(siwifi_hw, hw_rxhdr->flags_vif_idx);
            if (!siwifi_vif) {
                dev_err(siwifi_hw->dev, "Frame received but no active vif (%d, %d)",
                        hw_rxhdr->flags_vif_idx, ((struct hw_rxhdr *)CKSEG1ADDR(hw_rxhdr))->flags_vif_idx);
                dev_err(siwifi_hw->dev, "siwifi hw vif map : 0x%x\n", siwifi_hw->avail_idx_map);
#ifndef CFG_DEAGG
                dev_kfree_skb(skb);
#else
                if (hw_rxhdr->flags_vif_idx < NX_VIRT_DEV_MAX
                        && hw_rxhdr->flags_is_amsdu) {
                    get_amsdu = 1;
                    free_skb = 1;
                } else {
                    dev_kfree_skb(skb);
                }
#endif
                goto check_alloc;
            }
            if (hw_rxhdr->flags_sta_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) {
                struct siwifi_sta *sta;
                sta = &siwifi_hw->sta_table[hw_rxhdr->flags_sta_idx];
    sta->stats.idle = ktime_get_seconds();
                siwifi_rx_statistic(siwifi_hw, hw_rxhdr, sta);
                if (sta != NULL && sta->valid) {
                    struct ethhdr eth;
                    memcpy(&eth, skb->data, sizeof(eth));
                    if (!is_multicast_ether_addr(eth.h_dest)){
                        siwifi_rx_sta_stats(sta, hw_rxhdr);
                    }
                }
                if (sta->vlan_idx != siwifi_vif->vif_index) {
                    siwifi_vif = siwifi_hw->vif_table[sta->vlan_idx];
                    if (!siwifi_vif) {
      #ifndef CFG_DEAGG
                        dev_kfree_skb(skb);
      #else
                        if (hw_rxhdr->flags_is_amsdu) {
          get_amsdu = 1;
          free_skb = 1;
                        } else {
                            dev_kfree_skb(skb);
                        }
      #endif
                        goto check_alloc;
                    }
                }
                if (hw_rxhdr->flags_is_4addr && !siwifi_vif->use_4addr) {
                    cfg80211_rx_unexpected_4addr_frame(siwifi_vif->ndev,
                                                       sta->mac_addr, 0, GFP_ATOMIC);
                }
            } else {
                dev_err(siwifi_hw->dev, "Frame received but no active sta (%d)",
                        hw_rxhdr->flags_sta_idx);
            }
            skb->priority = 256 + hw_rxhdr->flags_user_prio;
#ifndef CFG_DEAGG
            if (!siwifi_rx_data_skb(siwifi_hw, siwifi_vif, skb, hw_rxhdr, NULL))
                dev_kfree_skb(skb);
#else
            if (hw_rxhdr->flags_is_amsdu) {
                for (i = 0; i < RX_AMSDU_CNT_MAX - 1; i++) {
                    if (!hw_rxhdr->amsdu_hostids[i])
                        break;
                    rx_buff_idx_amsdu = SIWIFI_RXBUFF_HOSTID_TO_IDX(hw_rxhdr->amsdu_hostids[i]);
                    if (!SIWIFI_RXBUFF_VALID_IDX(rx_buff_idx_amsdu)) {
                        dev_err(siwifi_hw->dev, "RX Buff invalid2 idx [%d]\n", rx_buff_idx_amsdu);
      free_skb = 1;
                        break;
                    }
                    amsdu_skb[i] = siwifi_hw->rxbuf_elems.skb[rx_buff_idx_amsdu];
                    if (!amsdu_skb[i]) {
                        dev_err(siwifi_hw->dev, "amsdu_skb is null[%d]\n", rx_buff_idx_amsdu);
      free_skb = 1;
                        break;
                    }
                    if (SIWIFI_RXBUFF_PATTERN_GET(amsdu_skb[i]) != siwifi_rxbuff_pattern) {
                        dev_err(siwifi_hw->dev, "RX Buff Pattern not correct\n");
      free_skb = 1;
                        break;
                    }
                    siwifi_ipc_rxbuf_elem_pull(siwifi_hw, amsdu_skb[i]);
                    if (!(status & RX_STAT_ALLOC)) {
                        siwifi_hw->reord_release_cnt++;
                    }
                    skb_reserve(amsdu_skb[i], msdu_offset);
                }
                if (!siwifi_rx_data_skb(siwifi_hw, siwifi_vif, skb, hw_rxhdr, amsdu_skb)) {
     free_skb = 1;
                }
            } else {
                if (!siwifi_rx_data_skb(siwifi_hw, siwifi_vif, skb, hw_rxhdr, amsdu_skb))
                    dev_kfree_skb(skb);
            }
#endif
#ifdef CONFIG_SF19A28_WIFI_LED
   siwifi_led_rx(siwifi_hw);
#endif
        }
    }
check_alloc:
    if ((status & RX_STAT_ALLOC)) {
        siwifi_ipc_rxbuf_elem_allocs(siwifi_hw);
        if (!(status & (RX_STAT_FORWARD | RX_STAT_DELETE))) {
            siwifi_hw->reord_alloc_cnt++;
        }
#ifdef CFG_DEAGG
        if (amsdu_num != 0) {
            for (i = 0; i < amsdu_num; i++) {
                siwifi_ipc_rxbuf_elem_allocs(siwifi_hw);
                if (!(status & (RX_STAT_FORWARD | RX_STAT_DELETE))) {
                    siwifi_hw->reord_alloc_cnt++;
                }
            }
        }
#endif
    }
end:
#ifdef CFG_DEAGG
 if (get_amsdu)
 {
  for (i = 0; i < RX_AMSDU_CNT_MAX - 1; i++) {
   if (!hw_rxhdr->amsdu_hostids[i])
    break;
   rx_buff_idx_amsdu = SIWIFI_RXBUFF_HOSTID_TO_IDX(hw_rxhdr->amsdu_hostids[i]);
   if (!SIWIFI_RXBUFF_VALID_IDX(rx_buff_idx_amsdu)) {
    dev_err(siwifi_hw->dev, "get amsdu RX Buff invalid idx [%d]\n", rx_buff_idx_amsdu);
                continue;
   }
   amsdu_skb[i] = siwifi_hw->rxbuf_elems.skb[rx_buff_idx_amsdu];
   if (!amsdu_skb[i]) {
    dev_err(siwifi_hw->dev, "get amsdu amsdu_skb is null[%d]\n", rx_buff_idx_amsdu);
                continue;
   }
   if (SIWIFI_RXBUFF_PATTERN_GET(amsdu_skb[i]) != siwifi_rxbuff_pattern) {
    dev_err(siwifi_hw->dev, "get amsdu RX Buff Pattern not correct\n");
                continue;
   }
   siwifi_ipc_rxbuf_elem_pull(siwifi_hw, amsdu_skb[i]);
  }
 }
 if (free_skb)
 {
  for (i = 0; i < RX_AMSDU_CNT_MAX - 1; i++) {
   if (!hw_rxhdr->amsdu_hostids[i])
    break;
            if (amsdu_skb[i])
       dev_kfree_skb(amsdu_skb[i]);
  }
  dev_kfree_skb(skb);
 }
#endif
    spin_unlock(&siwifi_hw->cb_lock);
    REG_SW_CLEAR_PROFILING(siwifi_hw, SW_PROF_SIWIFIDATAIND);
    siwifi_ipc_rxdesc_elem_repush(siwifi_hw, elem);
    return 0;
}
#ifdef CONFIG_WIFI_RX_THREAD
static void siwifi_rx_thread_process_skbs(struct siwifi_hw *siwifi_hw, struct rx_skb_element *rx_elt)
{
    struct siwifi_vif *vif;
    struct sk_buff *rx_skb;
    uint32_t tx_dropped = 0;
    uint32_t tx_errors = 0;
    uint32_t forward_len = 0;
    uint32_t forward_num = 0;
    vif = rx_elt->vif;
    if (!vif) {
        while ((rx_skb = __skb_dequeue(&rx_elt->skb_list))) {
            dev_kfree_skb(rx_skb);
            siwifi_hw->total_rx ++;
        }
        return;
    }
    while ((rx_skb = __skb_dequeue(&rx_elt->skb_list))) {
        spin_lock_bh(&siwifi_hw->cb_lock);
        if (rx_elt->sta != NULL && rx_elt->sta->valid) {
#ifdef CONFIG_VDR_HW
            vendor_hook_rxdata(rx_elt->sta, rx_skb, rx_elt->rxhdr);
#endif
            siwifi_rx_sta_stats(rx_elt->sta, rx_elt->rxhdr);
        }
        spin_unlock_bh(&siwifi_hw->cb_lock);
        {
            if (SIWIFI_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) {
                struct ethhdr *sf_eth = NULL;
                struct siwifi_src_filter *src_filter = NULL;
                skb_reset_mac_header(rx_skb);
                sf_eth = eth_hdr(rx_skb);
                src_filter = siwifi_src_filter_hash_search(vif, sf_eth->h_source);
                if (src_filter) {
                    src_filter->drop_count++;
                    dev_kfree_skb(rx_skb);
                    continue;
                }
            }
        }
        if (rx_elt->resend) {
            struct sk_buff *skb_copy;
            int res;
#if 0
            skb_copy = skb_copy_expand(rx_skb, sizeof(struct siwifi_txhdr) +
                    SIWIFI_SWTXHDR_ALIGN_SZ, 0, GFP_ATOMIC);
            if (!skb_copy) {
                printk("%s,%d skb copy expand failed\n",
                        siwifi_mod_params.is_hb ? "HB" : "LB", __LINE__);
                goto FORWARD;
            }
#else
            if (rx_elt->forward) {
                skb_copy = skb_clone(rx_skb, GFP_ATOMIC);
                if (!skb_copy) {
                    printk("%s,%d skb copy expand failed\n",
                            siwifi_mod_params.is_hb ? "HB" : "LB", __LINE__);
                    goto FORWARD;
                }
            } else {
                skb_copy = rx_skb;
            }
#endif
            skb_copy->protocol = htons(ETH_P_802_3);
            skb_reset_network_header(skb_copy);
            skb_reset_mac_header(skb_copy);
            res = dev_queue_xmit(skb_copy);
            if (res == NET_XMIT_DROP) {
                tx_dropped ++;
            } else if (res != NET_XMIT_SUCCESS) {
                tx_errors ++;
            }
        }
FORWARD:
        if (rx_elt->forward) {
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
            struct sf_hnat_priv *phnat_priv;
            struct platform_device *hnat_pdev;
#endif
            skb_reset_mac_header(rx_skb);
            memset(rx_skb->cb, 0, sizeof(rx_skb->cb));
            siwifi_trace_rx_in(siwifi_hw, rx_elt->sta, rx_skb);
#ifdef CONFIG_SIWIFI_IGMP
            siwifi_check_igmp(siwifi_hw, rx_skb);
#endif
            spin_lock_bh(&siwifi_hw->cb_lock);
            rx_skb->protocol = eth_type_trans(rx_skb, vif->ndev);
            if (unlikely(rx_elt->skip_len)) {
                memmove(skb_mac_header(rx_skb) + rx_elt->skip_len,
                        skb_mac_header(rx_skb), sizeof(struct ethhdr));
                __skb_pull(rx_skb, rx_elt->skip_len);
                skb_reset_mac_header(rx_skb);
                rx_elt->skip_len = 0;
            }
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
            phnat_priv = vif->phnat_priv;
            hnat_pdev = vif->hnat_pdev;
            if (phnat_priv) {
                phnat_priv->search(hnat_pdev, rx_skb);
            }
#endif
            spin_unlock_bh(&siwifi_hw->cb_lock);
            REG_SW_SET_PROFILING(siwifi_hw, SW_PROF_IEEE80211RX);
            forward_num ++;
            forward_len += rx_skb->len;
            local_bh_disable();
            netif_receive_skb(rx_skb);
            local_bh_enable();
            REG_SW_CLEAR_PROFILING(siwifi_hw, SW_PROF_IEEE80211RX);
        }
        if (!rx_elt->forward && !rx_elt->resend)
            dev_kfree_skb(rx_skb);
        siwifi_hw->total_rx ++;
        vif->total_rx ++;
    }
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    siwifi_hw->stats.total_rx += forward_num;
#endif
    spin_lock_bh(&siwifi_hw->cb_lock);
    if (tx_dropped) {
        vif->net_stats.rx_dropped += tx_dropped;
        vif->net_stats.tx_dropped += tx_dropped;
    }
    if (tx_errors) {
        vif->net_stats.tx_errors += tx_errors;
    }
    vif->net_stats.rx_packets += forward_num;
    vif->net_stats.rx_bytes += forward_len;
    if (rx_elt->sta && rx_elt->sta->valid) {
        rx_elt->sta->stats.rx_packets += forward_num;
        rx_elt->sta->stats.rx_bytes += forward_len;
        if (tx_dropped) {
            rx_elt->sta->stats.rx_dropped += tx_dropped;
            rx_elt->sta->stats.tx_dropped += tx_dropped;
        }
        if (tx_errors)
            rx_elt->sta->stats.tx_failed += tx_errors;
    }
    spin_unlock_bh(&siwifi_hw->cb_lock);
}
static int siwifi_rx_thread(void *data)
{
    int result = 0;
    struct siwifi_hw *siwifi_hw = (struct siwifi_hw *)data;
    size_t pending;
    struct rx_skb_element *rx_elt;
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    int loop_time;
#endif
    spin_lock_bh(&siwifi_hw->rx_lock);
    BUG_ON(siwifi_hw->rx_kthread != NULL);
    siwifi_hw->rx_kthread = current;
    spin_unlock_bh(&siwifi_hw->rx_lock);
    while (1) {
        pending = 0;
        wait_event_interruptible(
                siwifi_hw->rx_wq,
                (kthread_should_stop()
                 || (pending = atomic_read(&siwifi_hw->rx_pending_count)))
                );
        if (kthread_should_stop()) {
            printk("user want rx thread(%s) exit\n", siwifi_mod_params.is_hb ? "HB" : "LB");
            break;
        }
        if (pending == 0)
            continue;
        do {
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
            loop_time = 0;
#endif
            spin_lock_bh(&siwifi_hw->rx_lock);
            if (list_empty(&siwifi_hw->rx_skbs)) {
                spin_unlock_bh(&siwifi_hw->rx_lock);
                atomic_set(&siwifi_hw->rx_pending_count, 0);
                break;
            }
            rx_elt = list_first_entry(&siwifi_hw->rx_skbs, struct rx_skb_element, list);
            list_del(&rx_elt->list);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
            siwifi_hw->rx_thread_dbg.elt_cur_cnt --;
            siwifi_hw->rx_thread_dbg.skb_cur_cnt -= skb_queue_len(&rx_elt->skb_list);
#endif
            spin_unlock_bh(&siwifi_hw->rx_lock);
            if (kthread_should_stop()) {
                printk("user want rx thread(%s) exit\n", siwifi_mod_params.is_hb ? "HB" : "LB");
                break;
            }
            if (unlikely(!skb_queue_len(&rx_elt->skb_list))) {
                printk("Wanrning : there is no skb in rx skb elemnt%s\n", siwifi_mod_params.is_hb ? "HB" : "LB");
                siwifi_kfree(rx_elt);
                continue;
            }
            siwifi_rx_thread_process_skbs(siwifi_hw, rx_elt);
            siwifi_kfree(rx_elt);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
            if (++loop_time > siwifi_hw->rx_thread_dbg.elt_loop_max_cnt)
                siwifi_hw->rx_thread_dbg.elt_loop_max_cnt = loop_time;
#endif
        } while (1);
    }
    spin_lock_bh(&siwifi_hw->rx_lock);
    siwifi_hw->rx_kthread = NULL;
    while (!list_empty(&siwifi_hw->rx_skbs)) {
        struct sk_buff *rx_skb;
        rx_elt = list_first_entry(&siwifi_hw->rx_skbs, struct rx_skb_element, list);
        list_del(&rx_elt->list);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
            siwifi_hw->rx_thread_dbg.elt_cur_cnt --;
            siwifi_hw->rx_thread_dbg.skb_cur_cnt -= skb_queue_len(&rx_elt->skb_list);
#endif
        while ((rx_skb = __skb_dequeue(&rx_elt->skb_list))) {
            dev_kfree_skb(rx_skb);
        }
        siwifi_kfree(rx_elt);
    }
    spin_unlock_bh(&siwifi_hw->rx_lock);
    return result;
}
void siwifi_rx_thread_flush(struct siwifi_hw *siwifi_hw, struct siwifi_vif *vif)
{
    struct rx_skb_element *rx_elt, *rx_elt1;
    struct sk_buff *rx_skb;
    printk("siwifi_rx_thread_flush\n");
    spin_lock_bh(&siwifi_hw->rx_lock);
    list_for_each_entry_safe(rx_elt, rx_elt1, &siwifi_hw->rx_skbs, list)
    {
        if (rx_elt->vif != vif)
            continue;
        list_del(&rx_elt->list);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->rx_thread_dbg.elt_cur_cnt --;
        siwifi_hw->rx_thread_dbg.skb_cur_cnt -= skb_queue_len(&rx_elt->skb_list);
#endif
        while ((rx_skb = __skb_dequeue(&rx_elt->skb_list))) {
            dev_kfree_skb(rx_skb);
        }
        siwifi_kfree(rx_elt);
    }
    spin_unlock_bh(&siwifi_hw->rx_lock);
    printk("siwifi_rx_thread_flush over!\n");
}
int siwifi_rx_thread_setup(struct siwifi_hw *siwifi_hw)
{
    struct task_struct *kthread;
    int result = 0;
    printk("siwifi_rx_thread_setup ++\n");
    if (!list_empty(&siwifi_hw->rx_skbs)) {
        printk("rx skbs list should be empty when setup\n");
        return -1;
    }
    kthread = kthread_run(siwifi_rx_thread, siwifi_hw, "%s-rx",
            siwifi_mod_params.is_hb ? "HB" : "LB");
    if (IS_ERR(kthread)) {
        result = PTR_ERR(kthread);
        printk("RX: cannot start thread: %d\n", result);
    }
    return result;
}
void siwifi_rx_thread_release(struct siwifi_hw *siwifi_hw)
{
    struct task_struct *kthread;
    printk("siwifi_rx_thread_release++\n");
    spin_lock_bh(&siwifi_hw->rx_lock);
    kthread = siwifi_hw->rx_kthread;
    siwifi_hw->rx_kthread = NULL;
    spin_unlock_bh(&siwifi_hw->rx_lock);
    if (kthread)
        kthread_stop(kthread);
    else
        printk("RX: kthread had already exited, %s\n",
                siwifi_mod_params.is_hb ? "HB" : "LB");
}
#endif
