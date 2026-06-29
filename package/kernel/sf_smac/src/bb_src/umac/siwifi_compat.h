#ifndef _SIWIFI_COMPAT_H_
#define _SIWIFI_COMPAT_H_ 
#include <linux/version.h>
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
#error "Minimum kernel version supported is 3.16"
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
#define __bf_shf(x) (__builtin_ffsll(x) - 1)
#define FIELD_PREP(_mask,_val) \
    (((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask))
#else
#include <linux/bitfield.h>
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#define IEEE80211_RADIOTAP_HE 23
#define IEEE80211_RADIOTAP_HE_MU 24
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#define CCFS0(vht) vht->center_freq_seg1_idx
#define CCFS1(vht) vht->center_freq_seg2_idx
#else
#define CCFS0(vht) vht->center_freq_seg0_idx
#define CCFS1(vht) vht->center_freq_seg1_idx
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0) && !CPTCFG_WIRELESS
#define cfg80211_disconnected(dev,reason,ie,len,local,gfp) \
    cfg80211_disconnected(dev, reason, ie, len, gfp)
#endif
#if (MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)) && !(defined CONFIG_VENDOR_SIWIFI)
#define ieee80211_chandef_to_operating_class(chan_def,op_class) 0
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0) && !defined CPTCFG_WIRELESS
#define SURVEY_INFO_TIME SURVEY_INFO_CHANNEL_TIME
#define SURVEY_INFO_TIME_BUSY SURVEY_INFO_CHANNEL_TIME_BUSY
#define SURVEY_INFO_TIME_EXT_BUSY SURVEY_INFO_CHANNEL_TIME_EXT_BUSY
#define SURVEY_INFO_TIME_RX SURVEY_INFO_CHANNEL_TIME_RX
#define SURVEY_INFO_TIME_TX SURVEY_INFO_CHANNEL_TIME_TX
#define SURVEY_TIME(s) s->channel_time
#define SURVEY_TIME_BUSY(s) s->channel_time_busy
#else
#define SURVEY_TIME(s) s->time
#define SURVEY_TIME_BUSY(s) s->time_busy
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
#define cfg80211_ch_switch_started_notify(dev,chandef,count) 
#define WLAN_BSS_COEX_INFORMATION_REQUEST BIT(0)
#define WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING BIT(2)
#define WLAN_EXT_CAPA4_TDLS_BUFFER_STA BIT(4)
#define WLAN_EXT_CAPA4_TDLS_PEER_PSM BIT(5)
#define WLAN_EXT_CAPA4_TDLS_CHAN_SWITCH BIT(6)
#define WLAN_EXT_CAPA5_TDLS_CH_SW_PROHIBITED BIT(7)
#define NL80211_FEATURE_TDLS_CHANNEL_SWITCH 0
#define STA_TDLS_INITIATOR(sta) 0
#define REGULATORY_IGNORE_STALE_KICKOFF 0
#else
#define STA_TDLS_INITIATOR(sta) sta->tdls_initiator
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
#define siwifi_cfg80211_del_station_compat(wiphy,dev,params) \
    siwifi_cfg80211_del_station(wiphy, dev, const u8 *mac)
#else
#define siwifi_cfg80211_del_station_compat siwifi_cfg80211_del_station
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
#define cfg80211_rx_mgmt(wdev,freq,rssi,buf,len,flags) \
    cfg80211_rx_mgmt(wdev, freq, rssi, buf, len, flags, GFP_ATOMIC)
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#define siwifi_cfg80211_tdls_mgmt(wiphy,dev,peer,link_id,act,tok,status,peer_capability,initiator,buf,len) \
    siwifi_cfg80211_tdls_mgmt(wiphy, dev, peer, act, tok, status, peer_capability, buf, len)
#define siwifi_cfg80211_del_station_compat(wiphy,dev,params) \
    siwifi_cfg80211_del_station(wiphy, dev, const u8 *mac)
#include <linux/types.h>
struct ieee80211_wmm_ac_param {
 u8 aci_aifsn;
 u8 cw;
 __le16 txop_limit;
} __packed;
struct ieee80211_wmm_param_ie {
 u8 element_id;
 u8 len;
 u8 oui[3];
 u8 oui_type;
 u8 oui_subtype;
 u8 version;
 u8 qos_info;
 u8 reserved;
 struct ieee80211_wmm_ac_param ac[4];
} __packed;
#endif
#define RX_ENC_HT(s) s->encoding |= RX_ENC_HT
#define RX_ENC_HT_GF(s) { s->encoding |= RX_ENC_HT; s->enc_flags |= RX_ENC_FLAG_HT_GF;}
#define RX_ENC_VHT(s) s->encoding |= RX_ENC_VHT
#define RX_ENC_HE(s) s->encoding |= RX_ENC_VHT
#define RX_ENC_FLAG_SHORT_GI(s) s->enc_flags |= RX_ENC_FLAG_SHORT_GI
#define RX_ENC_FLAG_SHORT_PRE(s) s->enc_flags |= RX_ENC_FLAG_SHORTPRE
#define RX_ENC_FLAG_LDPC(s) s->enc_flags |= RX_ENC_FLAG_LDPC
#define RX_BW_40MHZ(s) s->bw = RATE_INFO_BW_40
#define RX_BW_80MHZ(s) s->bw = RATE_INFO_BW_80
#define RX_BW_160MHZ(s) s->bw = RATE_INFO_BW_160
#define RX_NSS(s) s->nss
#ifndef CONFIG_VENDOR_SIWIFI_AMSDUS_TX
#ifndef CPTCFG_WIRELESS
#if (MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
#define siwifi_ops_ampdu_action(hw,vif,params) \
    siwifi_ops_ampdu_action(hw, vif, enum ieee80211_ampdu_mlme_action action, \
                          struct ieee80211_sta *sta, u16 tid, u16 *ssn, u8 buf_size)
#elif (MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0))
#define siwifi_ops_ampdu_action(hw,vif,params) \
    siwifi_ops_ampdu_action(hw, vif, enum ieee80211_ampdu_mlme_action action, \
                          struct ieee80211_sta *sta, u16 tid, u16 *ssn, u8 buf_size, \
                          bool amsdu)
#endif
#endif
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0) && !CPTCFG_WIRELESS
#define IEEE80211_HW_SUPPORT_FAST_XMIT 0
#define ieee80211_hw_check(hw,feat) (hw->flags & IEEE80211_HW_ ##feat)
#define ieee80211_hw_set(hw,feat) {hw->flags |= IEEE80211_HW_ ##feat;}
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
#define siwifi_ops_sw_scan_start(hw,vif,mac_addr) \
    siwifi_ops_sw_scan_start(hw)
#define siwifi_ops_sw_scan_complete(hw,vif) \
    siwifi_ops_sw_scan_complete(hw)
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#define siwifi_ops_hw_scan(hw,vif,hw_req) \
    siwifi_ops_hw_scan(hw, vif, struct cfg80211_scan_request *req)
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#define siwifi_select_queue(dev,skb,sb_dev,fallback) \
    siwifi_select_queue(dev, skb, void *accel_priv, fallback)
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#define alloc_netdev_mqs(size,name,assign,setup,txqs,rxqs) \
    alloc_netdev_mqs(size, name, setup, txqs, rxqs)
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#define NET_NAME_UNKNOWN 0
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
#define trace_print_symbols_seq ftrace_print_symbols_seq
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#define trace_seq_buffer_ptr(p) p->buffer + p->len
#endif
#endif
