#ifndef _SIWIFI_RX_H_
#define _SIWIFI_RX_H_ 
enum rx_status_bits
{
    RX_STAT_FORWARD = 1 << 0,
    RX_STAT_ALLOC = 1 << 1,
    RX_STAT_DELETE = 1 << 2,
    RX_STAT_LEN_UPDATE = 1 << 3,
    RX_STAT_ETH_LEN_UPDATE = 1 << 4,
    RX_STAT_COPY = 1 << 5,
    RX_STAT_SPURIOUS = 1 << 6,
    RX_STAT_MONITOR = 1 << 7,
};
#define SIWIFI_RX_HD_DECR_UNENC 0
#define SIWIFI_RX_HD_DECR_ICVFAIL 1
#define SIWIFI_RX_HD_DECR_CCMPFAIL 2
#define SIWIFI_RX_HD_DECR_AMSDUDISCARD 3
#define SIWIFI_RX_HD_DECR_NULLKEY 4
#define SIWIFI_RX_HD_DECR_WEPSUCCESS 5
#define SIWIFI_RX_HD_DECR_TKIPSUCCESS 6
#define SIWIFI_RX_HD_DECR_CCMPSUCCESS 7
#ifdef CONFIG_SIWIFI_MON_DATA
#define RX_MACHDR_BACKUP_LEN 64
#endif
#define SIWIFI_RX_LOW_MEM_SIZE 5000
struct rx_vector_1_old {
    u32 leg_length :12;
    u32 leg_rate : 4;
    u32 ht_length :16;
    u32 _ht_length : 4;
    u32 short_gi : 1;
    u32 stbc : 2;
    u32 smoothing : 1;
    u32 mcs : 7;
    u32 pre_type : 1;
    u32 format_mod : 3;
    u32 ch_bw : 2;
    u32 n_sts : 3;
    u32 lsig_valid : 1;
    u32 sounding : 1;
    u32 num_extn_ss : 2;
    u32 aggregation : 1;
    u32 fec_coding : 1;
    u32 dyn_bw : 1;
    u32 doze_not_allowed : 1;
    u32 antenna_set : 8;
    u32 partial_aid : 9;
    u32 group_id : 6;
    u32 first_user : 1;
    s32 rssi1 : 8;
    s32 rssi2 : 8;
    s32 rssi3 : 8;
    s32 rssi4 : 8;
    u32 reserved_1d : 8;
};
struct rx_leg_vect
{
    u8 dyn_bw_in_non_ht : 1;
    u8 chn_bw_in_non_ht : 2;
    u8 rsvd_nht : 4;
    u8 lsig_valid : 1;
} __packed;
struct rx_ht_vect
{
    u16 sounding : 1;
    u16 smoothing : 1;
    u16 short_gi : 1;
    u16 aggregation : 1;
    u16 stbc : 1;
    u16 num_extn_ss : 2;
    u16 lsig_valid : 1;
    u16 mcs : 7;
    u16 fec : 1;
    u16 length :16;
} __packed;
struct rx_vht_vect
{
    u8 sounding : 1;
    u8 beamformed : 1;
    u8 short_gi : 1;
    u8 rsvd_vht1 : 1;
    u8 stbc : 1;
    u8 doze_not_allowed : 1;
    u8 first_user : 1;
    u8 rsvd_vht2 : 1;
    u16 partial_aid : 9;
    u16 group_id : 6;
    u16 rsvd_vht3 : 1;
    u32 mcs : 4;
    u32 nss : 3;
    u32 fec : 1;
    u32 length :20;
    u32 rsvd_vht4 : 4;
} __packed;
struct rx_he_vect
{
    u8 sounding : 1;
    u8 beamformed : 1;
    u8 gi_type : 2;
    u8 stbc : 1;
    u8 rsvd_he1 : 3;
    u8 uplink_flag : 1;
    u8 beam_change : 1;
    u8 dcm : 1;
    u8 he_ltf_type : 2;
    u8 doppler : 1;
    u8 rsvd_he2 : 2;
    u8 bss_color : 6;
    u8 rsvd_he3 : 2;
    u8 txop_duration : 7;
    u8 rsvd_he4 : 1;
    u8 pe_duration : 4;
    u8 spatial_reuse : 4;
    u8 rsvd_he5 : 8;
    u32 mcs : 4;
    u32 nss : 3;
    u32 fec : 1;
    u32 length :20;
    u32 rsvd_he6 : 4;
} __packed;
#define rx_vector_1 rx_vector_1_old
struct rx_vector_2_old {
    u32 rcpi : 8;
    u32 evm1 : 8;
    u32 evm2 : 8;
    u32 evm3 : 8;
    u32 evm4 : 8;
    u32 reserved2b_1 : 8;
    u32 reserved2b_2 : 8;
    u32 reserved2b_3 : 8;
};
#define rx_vector_2 rx_vector_2_old
struct phy_channel_info_desc {
    u32 phy_band : 8;
    u32 phy_channel_type : 8;
    u32 phy_prim20_freq : 16;
    u32 phy_center1_freq : 16;
    u32 phy_center2_freq : 16;
};
struct hw_vect {
    u32 len :16;
    u32 reserved : 8;
    u32 mpdu_cnt : 6;
    u32 ampdu_cnt : 2;
    __le32 tsf_lo;
    __le32 tsf_hi;
    struct rx_vector_1 rx_vect1;
    struct rx_vector_2 rx_vect2;
    u32 rx_vect2_valid : 1;
    u32 resp_frame : 1;
    u32 decr_status : 3;
    u32 rx_fifo_oflow : 1;
    u32 undef_err : 1;
    u32 phy_err : 1;
    u32 fcs_err : 1;
    u32 addr_mismatch : 1;
    u32 ga_frame : 1;
    u32 current_ac : 2;
    u32 frm_successful_rx : 1;
    u32 desc_done_rx : 1;
    u32 key_sram_index : 10;
    u32 key_sram_v : 1;
    u32 type : 2;
    u32 subtype : 4;
};
#ifdef CONFIG_SIWIFI_MON_DATA
struct mon_machdrdesc
{
    u32 buf_len;
    u8 buffer[RX_MACHDR_BACKUP_LEN];
};
#endif
struct hw_rxhdr {
    struct hw_vect hwvect;
    struct phy_channel_info_desc phy_info;
    u32 flags_is_amsdu : 1;
    u32 flags_is_80211_mpdu: 1;
    u32 flags_is_4addr : 1;
    u32 flags_new_peer : 1;
    u32 flags_user_prio : 3;
    u32 flags_rsvd0 : 1;
    u32 flags_vif_idx : 8;
    u32 flags_sta_idx : 8;
    u32 flags_dst_idx : 8;
#ifdef CFG_DEAGG
    u32 amsdu_hostids[RX_AMSDU_CNT_MAX - 1];
#endif
#ifdef CONFIG_SIWIFI_MON_DATA
    struct mon_machdrdesc mac_hdr_backup;
#endif
    u32 pattern;
};
extern const u8 legrates_lut[];
u8 siwifi_unsup_rx_vec_ind(void *pthis, void *hostid);
u8 siwifi_rxdataind(void *pthis, void *hostid);
#ifdef CONFIG_WIFI_RX_THREAD
struct rx_skb_element {
    struct list_head list;
    struct sk_buff_head skb_list;
    struct siwifi_vif *vif;
    struct siwifi_sta *sta;
    struct hw_rxhdr *rxhdr;
    bool forward;
    bool resend;
    u8 skip_len;
};
void siwifi_rx_thread_flush(struct siwifi_hw *siwifi_hw, struct siwifi_vif *vif);
int siwifi_rx_thread_setup(struct siwifi_hw *siwifi_hw);
void siwifi_rx_thread_release(struct siwifi_hw *siwifi_hw);
#endif
#endif
