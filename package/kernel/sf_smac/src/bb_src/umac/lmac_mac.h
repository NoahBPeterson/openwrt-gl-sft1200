#ifndef _MAC_H_
#define _MAC_H_ 
#ifndef __KERNEL__
#include <stdbool.h>
#else
#include <linux/types.h>
#endif
#define TU_DURATION 1024
#define MAC_DOMAINCHANNEL_24G_MAX 14
#define MAC_DOMAINCHANNEL_5G_MAX 45
#define MAC_BASIC_RATE 0x80
#define WORD_ALIGN 3
#define MAX_AMSDU_LENGTH 7935
#define MAC_ADDR_CMP(addr1_ptr,addr2_ptr) \
    ((*(((u16*)(addr1_ptr)) + 0) == *(((u16*)(addr2_ptr)) + 0)) && \
     (*(((u16*)(addr1_ptr)) + 1) == *(((u16*)(addr2_ptr)) + 1)) && \
     (*(((u16*)(addr1_ptr)) + 2) == *(((u16*)(addr2_ptr)) + 2)))
#define MAC_ADDR_CMP_PACKED(__a1,__a2) \
    (memcmp(__a1, __a2, MAC_ADDR_LEN) == 0)
#define MAC_ADDR_CPY(addr1_ptr,addr2_ptr) \
    *(((u16*)(addr1_ptr)) + 0) = *(((u16*)(addr2_ptr)) + 0); \
    *(((u16*)(addr1_ptr)) + 1) = *(((u16*)(addr2_ptr)) + 1); \
    *(((u16*)(addr1_ptr)) + 2) = *(((u16*)(addr2_ptr)) + 2)
#define MAC_SSID_CMP(ssid1_ptr,ssid2_ptr) \
    (((ssid1_ptr)->length == (ssid2_ptr)->length) && \
     (memcmp((&(ssid1_ptr)->array[0]), (&(ssid2_ptr)->array[0]), (ssid1_ptr)->length) == 0))
#define MAC_ADDR_GROUP(mac_addr_ptr) ((*(mac_addr_ptr)) & 1)
#define MAC_ADDR_LEN 6
struct mac_addr
{
    u16_l array[MAC_ADDR_LEN/2];
};
#define MAC_SSID_LEN 32
struct mac_ssid
{
    u8_l length;
    u8_l array[MAC_SSID_LEN];
};
#define MAC_RATESET_LEN 12
#define MAC_OFDM_PHY_RATESET_LEN 8
#define MAC_EXT_RATES_OFF 8
struct mac_rateset
{
    u8_l length;
    u8_l array[MAC_RATESET_LEN];
};
#define MAC_MCS_WORD_CNT 3
struct mac_rates
{
    u32 mcs[MAC_MCS_WORD_CNT];
    u16 legacy;
};
#define MAC_IV_LEN 4
#define MAC_EIV_LEN 4
struct rx_seciv
{
    u8 iv[MAC_IV_LEN];
    u8 ext_iv[MAC_EIV_LEN];
};
#define MAX_MCS_LEN 16
struct mac_mcsset
{
    u8 length;
    u8 array[MAX_MCS_LEN];
};
#define MAC_WEP_KEY_CNT 4
#define MAC_WEP_KEY_LEN 13
struct mac_wep_key
{
    u8 array[MAC_WEP_KEY_LEN];
};
#define MAC_SEC_KEY_LEN 32
struct mac_sec_key
{
    u8_l length;
    u32_l array[MAC_SEC_KEY_LEN/4];
};
#define MAC_MAX_CH 40
struct mac_ch_list
{
    u16 nbr;
    u8 list[MAC_MAX_CH];
};
struct mac_country_subband
{
    u8 first_chn;
    u8 nbr_of_chn;
    u8 max_tx_power;
};
#define MAX_COUNTRY_LEN 3
#define MAX_COUNTRY_SUBBAND 5
struct mac_country
{
    u8 length;
    u8 string[MAX_COUNTRY_LEN];
    struct mac_country_subband subband[MAX_COUNTRY_SUBBAND];
};
struct mac_htcapability
{
    u16_l ht_capa_info;
    u8_l a_mpdu_param;
    u8_l mcs_rate[MAX_MCS_LEN];
    u16_l ht_extended_capa;
    u32_l tx_beamforming_capa;
    u8_l asel_capa;
};
struct mac_vhtcapability
{
    u32_l vht_capa_info;
    u16_l rx_mcs_map;
    u16_l rx_highest;
    u16_l tx_mcs_map;
    u16_l tx_highest;
};
struct mac_htoprnelmt
{
    u8 prim_channel;
    u8 ht_oper_1;
    u16 ht_oper_2;
    u16 ht_oper_3;
    u8 mcs_rate[MAX_MCS_LEN];
};
#define MAC_HE_MAC_CAPA_LEN 5
#define MAC_HE_PHY_CAPA_LEN 9
#define MAC_HE_PPE_THRES_MAX_LEN 25
struct mac_he_mcs_nss_supp
{
    u16_l rx_mcs_80;
    u16_l tx_mcs_80;
    u16_l rx_mcs_160;
    u16_l tx_mcs_160;
    u16_l rx_mcs_80p80;
    u16_l tx_mcs_80p80;
};
struct mac_hecapability
{
    u8_l mac_cap_info[MAC_HE_MAC_CAPA_LEN];
    u8_l phy_cap_info[MAC_HE_PHY_CAPA_LEN];
    struct mac_he_mcs_nss_supp mcs_supp;
    u8_l ppe_thres[MAC_HE_PPE_THRES_MAX_LEN];
};
struct mac_qoscapability
{
    u8 qos_info;
};
#define MAC_RAW_RSN_IE_LEN 34
struct mac_raw_rsn_ie
{
    u8 data[2 + MAC_RAW_RSN_IE_LEN];
};
#define MAC_RAW_ENC_LEN 0x1A
struct mac_wpa_frame
{
    u8 array[MAC_RAW_ENC_LEN];
};
#define MAC_WME_PARAM_LEN 16
struct mac_wmm_frame
{
    u8 array [MAC_WME_PARAM_LEN];
};
struct mac_bss_load
{
    u16 sta_cnt;
    u8 ch_utilization;
    u16 avail_adm_capacity;
};
struct mac_edca_param_set
{
    u8 qos_info;
    u32 ac_be_param_record;
    u32 ac_bk_param_record;
    u32 ac_vi_param_record;
    u32 ac_vo_param_record;
};
struct mac_twenty_fourty_bss
{
    u8 bss_coexistence;
};
struct mac_ba_param
{
    struct mac_addr peer_sta_address;
    u16 buffer_size;
    u16 start_sequence_number;
    u16 ba_timeout;
    u8 dev_type;
    u8 block_ack_policy;
    u8 buffer_cnt;
};
struct mac_ts_info
{
    u8 traffic_type;
    u8 ack_policy;
    u8 access_policy;
    u8 dir;
    u8 tsid;
    u8 user_priority;
    bool aggregation;
    bool apsd;
    bool schedule;
};
struct mac_tspec_param
{
    struct mac_ts_info ts_info;
    u16 nominal_msdu_size;
    u16 max_msdu_size;
    u32 min_service_interval;
    u32 max_service_interval;
    u32 inactivity_interval;
    u32 short_inactivity_interval;
    u32 service_start_time;
    u32 max_burst_size;
    u32 min_data_rate;
    u32 mean_data_rate;
    u32 min_phy_rate;
    u32 peak_data_rate;
    u32 delay_bound;
    u16 medium_time;
    u8 surplusbwallowance;
};
struct mac_scan_result
{
    struct mac_addr bssid;
    struct mac_ssid ssid;
    u16 bsstype;
    u16 ch_nbr;
    u16 beacon_period;
    u32 timestamp_high;
    u32 timestamp_low;
    u16 dtim_period;
    u16 ibss_parameter;
    u16 cap_info;
    struct mac_rateset rate_set;
    struct mac_bss_load bss_load;
    u8 country_element[3];
    struct mac_edca_param_set edca_param;
    struct mac_raw_rsn_ie rsn_ie;
    struct mac_qoscapability qos_cap;
    struct mac_htcapability ht_cap;
    u8 sec_ch_oft;
    struct mac_twenty_fourty_bss twenty_fourty_bss;
    bool valid_flag;
    u8 rssi;
};
struct mac_request_set
{
    u8 mode;
    u8 type;
    u16 duration;
    uint64_t start_time;
    u8 ch_number;
};
struct mac_report_set
{
    u8 mode;
    u8 type;
    u16 duration;
    uint64_t start_time;
    u8 ch_number;
    u8 map;
    u8 cca_busy_fraction;
    u8 rpi_histogram[8];
};
struct mac_version
{
    char mac_sw_version[16];
    char mac_sw_version_date[48];
    char mac_sw_build_date[48];
    u32 mac_hw_version1;
    u32 mac_hw_version2;
};
struct mac_bss_conf
{
    u32 flags;
    u16 beacon_period;
};
enum
{
    TID_0,
    TID_1,
    TID_2,
    TID_3,
    TID_4,
    TID_5,
    TID_6,
    TID_7,
    TID_MGT,
    TID_MAX
};
enum
{
    AC_BK = 0,
    AC_BE,
    AC_VI,
    AC_VO,
    AC_MAX
};
enum
{
    SCAN_PASSIVE,
    SCAN_ACTIVE
};
enum
{
    MAC_RATE_1MBPS = 2,
    MAC_RATE_2MBPS = 4,
    MAC_RATE_5_5MBPS = 11,
    MAC_RATE_6MBPS = 12,
    MAC_RATE_9MBPS = 18,
    MAC_RATE_11MBPS = 22,
    MAC_RATE_12MBPS = 24,
    MAC_RATE_18MBPS = 36,
    MAC_RATE_24MBPS = 48,
    MAC_RATE_36MBPS = 72,
    MAC_RATE_48MBPS = 96,
    MAC_RATE_54MBPS = 108
};
enum
{
    STA_QOS_CAPA = 1 << 0,
    STA_HT_CAPA = 1 << 1,
    STA_VHT_CAPA = 1 << 2,
    STA_MFP_CAPA = 1 << 3,
    STA_OPMOD_NOTIF = 1 << 4,
    STA_HE_CAPA = 1 << 5,
};
enum
{
    CONTROL_PORT_HOST = 1 << 0,
    CONTROL_PORT_NO_ENC = 1 << 1,
    DISABLE_HT = 1 << 2,
    WPA_WPA2_IN_USE = 1 << 3,
    MFP_IN_USE = 1 << 4,
};
extern const u8 mac_tid2ac[];
extern const u8 mac_id2rate[];
extern const u16 mac_mcs_params_20[];
extern const u16 mac_mcs_params_40[];
#endif
