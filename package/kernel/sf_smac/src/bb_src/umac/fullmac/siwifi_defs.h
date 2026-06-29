#ifndef _SIWIFI_DEFS_H_
#define _SIWIFI_DEFS_H_ 
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/skbuff.h>
#include <net/cfg80211.h>
#include <linux/slab.h>
#include <net/mac80211.h>
#include "siwifi_mod_params.h"
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
#include "siwifi_debugfs.h"
#endif
#include "siwifi_tx.h"
#include "siwifi_rx.h"
#include "siwifi_radar.h"
#include "siwifi_utils.h"
#include "siwifi_mu_group.h"
#include "rf_pl_ref.h"
#if 1
#include "siwifi_mpw0.h"
#else
#include "siwifi_platform.h"
#endif
#include "siwifi_cmds.h"
#ifdef CONFIG_SIWIFI_TX_POWER_CALI
#include "siwifi_factory_info.h"
#endif
#ifdef CONFIG_SF19A28_WIFI_LED
#include <linux/leds.h>
#endif
#ifdef CONFIG_SIWIFI_IGMP
#include "siwifi_igmp.h"
#endif
#include "siwifi_trace.h"
#define WPI_HDR_LEN 18
#define WPI_PN_LEN 16
#define WPI_PN_OFST 2
#define WPI_MIC_LEN 16
#define WPI_KEY_LEN 32
#define WPI_SUBKEY_LEN 16
#define LEGACY_PS_ID 0
#define UAPSD_ID 1
#define LB_MODULE 1
#define HB_MODULE 2
#define STA_HASH_SIZE 256
#define STA_HASH(sta) (sta[3])
#define DEBUG_ARRAY_CHECK 1
#ifdef CONFIG_SIWIFI_SAVE_TXHDR_ALLOC
#define SIWIFI_TX_HEADROOM (uint32_t)(sizeof(struct siwifi_txhdr) + SIWIFI_SWTXHDR_ALIGN_SZ + sizeof(struct siwifi_sw_txhdr))
#else
#define SIWIFI_TX_HEADROOM (uint32_t)(sizeof(struct siwifi_txhdr) + SIWIFI_SWTXHDR_ALIGN_SZ)
#endif
#ifdef CONFIG_SIWIFI_AMSDUS_TX
#define SIWIFI_TX_AMSDU_HEADROOM ((uint32_t)(sizeof(struct siwifi_amsdu_txhdr) + sizeof(struct ethhdr) + 4 + sizeof(rfc1042_header) + 2))
#else
#define SIWIFI_TX_AMSDU_HEADROOM (uint32_t)(0)
#endif
#define SIWIFI_RX_HEADROOM (uint32_t)((sizeof(struct hw_rxhdr) + 2))
#define SRC_FILTER_AGING_TIME 30
struct siwifi_bcn {
    u8 *head;
    u8 *tail;
    u8 *ies;
    size_t head_len;
    size_t tail_len;
    size_t ies_len;
    size_t tim_len;
    size_t len;
    u8 dtim;
};
struct siwifi_key {
    u8 valid;
    u8 hw_idx;
};
struct siwifi_mesh_path {
    struct list_head list;
    u8 path_idx;
    struct mac_addr tgt_mac_addr;
    struct siwifi_sta *p_nhop_sta;
};
struct siwifi_mesh_proxy {
    struct list_head list;
    struct mac_addr ext_sta_addr;
    struct mac_addr proxy_addr;
    bool local;
};
struct siwifi_csa {
    struct siwifi_vif *vif;
    struct siwifi_bcn bcn;
    struct siwifi_ipc_elem_var elem;
    struct cfg80211_chan_def chandef;
    int count;
    int status;
    int ch_idx;
    struct work_struct work;
};
enum tdls_status_tag {
        TDLS_LINK_IDLE,
        TDLS_SETUP_REQ_TX,
        TDLS_SETUP_RSP_TX,
        TDLS_LINK_ACTIVE,
        TDLS_STATE_MAX
};
struct siwifi_tdls {
    bool active;
    bool initiator;
    bool chsw_en;
    u8 last_tid;
    u16 last_sn;
    bool ps_on;
    bool chsw_allowed;
};
enum siwifi_ap_flags {
    SIWIFI_AP_ISOLATE = BIT(0),
};
struct siwifi_mgmt_status
{
    u32 rx_bcn_cnts;
    u32 rx_non_cnts;
    u32 rx_ft_cnts;
    u32 rx_mgmt_cnts;
    u32 tx_mgmt;
    u32 tx_mgmt_drop;
};
struct siwifi_key_recovery_info {
    bool valid;
    u8 key_index;
    bool pairwise;
    struct key_params *params;
};
struct siwifi_lite_mem_ctrl {
    u32 tx_memory_usage;
    u32 tx_cnt;
    u32 amsdu_tx_cnt;
    u32 amsdu_tx_memory_usage;
    u32 tx_memory_limit;
    u32 tx_cnt_limit;
};
#ifdef CONFIG_DEBUG_TXQ_STOP
struct siwifi_txq_record
{
    uint8_t pos;
    uint32_t txq;
    uint32_t txqidx;
    uint32_t txq_status;
    uint32_t txqsize;
    uint32_t memctl_balance;
    struct siwifi_lite_mem_ctrl memctrl;
};
#endif
struct siwifi_src_filter {
    unsigned char src_macaddr[ETH_ALEN];
    unsigned int drop_count;
    struct siwifi_src_filter *next;
    unsigned long last_use;
};
struct siwifi_vif {
    struct list_head list;
    struct siwifi_hw *siwifi_hw;
    struct wireless_dev wdev;
    struct net_device *ndev;
    struct net_device_stats net_stats;
    struct siwifi_mgmt_status mgmt_stats;
    struct siwifi_key key[MAX_VIF_KEY_NUM];
    struct cfg80211_ap_settings *ap_settings;
    struct siwifi_key_recovery_info rec_key[MAX_VIF_KEY_NUM];
    u8 drv_vif_index;
    u8 vif_index;
    u8 ch_index;
    bool up;
    bool use_4addr;
    bool is_resending;
    bool user_mpm;
    bool roc_tdls;
    u8 tdls_status;
    bool tdls_chsw_prohibited;
    s8 txpower;
    u8 txpower_idx;
    union
    {
        struct
        {
            struct siwifi_sta *ap;
            struct siwifi_sta *tdls_sta;
            bool external_auth;
        } sta;
        struct
        {
            u16 flags_recovery;
            u16 flags;
            struct list_head sta_list;
            struct list_head local_sta_list;
            struct siwifi_bcn bcn;
            u8 bcmc_index;
            struct siwifi_csa *csa;
            bool channel_switching;
            struct list_head mpath_list;
            struct list_head proxy_list;
            bool create_path;
            int generation;
        } ap;
        struct
        {
            struct siwifi_vif *master;
            struct siwifi_sta *sta_4a;
        } ap_vlan;
    };
#if defined(CONFIG_SF19A28_FULLMASK) && IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER) && IS_ENABLED(CONFIG_NF_FLOW_TABLE)
 void * phnat_priv;
 struct platform_device *hnat_pdev;
#endif
    struct siwifi_lite_mem_ctrl *lm_ctl;
    u32 be_wmm_param;
    u64 total_rx;
    u32 hw_be_cnt;
    u32 hw_vi_cnt;
    u32 hw_vo_cnt;
    u32 hw_all_cnt;
    u64 hw_rx_cnt;
    u32 hw_last_wmm_step;
    u32 hw_wmm_counter;
 uint32_t sta_hash[STA_HASH_SIZE];
    uint32_t src_filter_hash[STA_HASH_SIZE];
    struct timer_list src_filter_timer;
    spinlock_t src_filter_lock;
    u8 repeater_status;
    int wds_success;
};
enum repeater_status
{
    REPEATER_FAIL = 0,
    REPEATER_VIF_UP,
    REPEATER_AUTH,
    REPEATER_ASSOC,
    REPEATER_EAPOL,
};
#define SIWIFI_VIF_TYPE(siwifi_vif) (siwifi_vif->wdev.iftype)
struct siwifi_sta_ps {
    bool active;
    u16 pkt_ready[2];
    u16 sp_cnt[2];
};
struct siwifi_rx_rate_stats {
    uint64_t *table;
    int size;
    uint64_t cpt;
};
#ifdef CONFIG_SIWIFI_SPLIT_TX_BUF
struct siwifi_amsdu_stats {
    int done;
    int failed;
};
#endif
struct siwifi_sta_stats {
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    struct hw_vect last_rx;
    struct siwifi_rx_rate_stats rx_rate;
#endif
#ifdef CONFIG_SIWIFI_SPLIT_TX_BUF
    struct siwifi_amsdu_stats amsdus[NX_TX_PAYLOAD_MAX];
    unsigned long last_set_amsdu_tp;
#endif
    u32 connected_time;
    u32 idle;
    u32 tx_packets;
    u32 rx_packets;
    u64 tx_bytes;
    u64 rx_bytes;
    u32 tx_failed;
    u32 rx_dropped;
    u32 tx_dropped;
    u8 short_gi;
    s8 data_rssi;
    u8 data_rx_bw;
    u8 format_mod;
    u8 data_rx_mcs;
    u8 data_rx_nss;
    u16 data_rx_rate;
    u32 leg_rate;
    s8 data_rssi_avg;
    s8 data_rssi_max;
    s8 data_rssi_min;
    u16 last_tx_info;
    u16 last_tx_rate_config;
    u16 count;
    s16 data_rssi_sum;
    s8 data_rssi_old[256];
    u8 preamble;
    u64 last_rx_bytes;
    u64 last_tx_bytes;
    u32 last_rx_tp;
    u32 last_tx_tp;
    u32 last_calc_tp;
    u32 last_th_tp;
#ifdef CONFIG_VDR_HW
    u64 tx_unicast_bytes;
    u64 rx_unicast_bytes;
    u64 tx_mcast_bytes;
    u64 rx_mcast_bytes;
    u64 last_rx_bytes;
    u64 last_tx_bytes;
    u32 ac_pkts[AC_MAX];
    u32 last_rx_tp;
    u32 last_tx_tp;
    u32 last_calc_tp;
    u32 last_ps_ts;
    u16 ps_times;
    u16 data_rx_rate;
    u8 no_ss;
    u8 short_gi;
    u8 data_rssi;
    u8 data_rx_bw;
#endif
};
struct siwifi_sta_rec_info {
    struct list_head list;
    u8 mac_addr[ETH_ALEN];
    struct station_parameters *params;
    struct siwifi_key_recovery_info rec_key;
    bool cp_open_set;
    int control_port_open;
    bool pstate_set;
    int plink_state;
    int qos;
    void *vlan;
    u8 sta_idx;
};
struct siwifi_sta {
    struct list_head list;
    u16 aid;
    u8 sta_idx;
    u8 vif_idx;
    u8 vlan_idx;
    enum nl80211_band band;
    enum nl80211_chan_width width;
    u16 center_freq;
    u32 center_freq1;
    u32 center_freq2;
    u8 ch_idx;
    bool qos;
    u8 acm;
    u16 uapsd_tids;
    u8 mac_addr[ETH_ALEN];
    struct siwifi_key key;
    bool valid;
    struct siwifi_sta_ps ps;
#ifdef CONFIG_SIWIFI_BFMER
    struct siwifi_bfmer_report *bfm_report;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    struct siwifi_sta_group_info group_info;
#endif
#endif
    bool ht;
    bool vht;
    u32 ac_param[AC_MAX];
    struct siwifi_tdls tdls;
    struct siwifi_sta_stats stats;
    struct siwifi_sta_rec_info rec_info;
 struct siwifi_sta *hnext;
    u16 update_time_count;
};
static inline const u8 *siwifi_sta_addr(struct siwifi_sta *siwifi_sta) {
    return siwifi_sta->mac_addr;
}
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
struct siwifi_stats {
    int cfm_balance[NX_TXQ_CNT];
    unsigned long last_rx, last_tx;
    int ampdus_tx[IEEE80211_MAX_AMPDU_BUF_HT];
    int ampdus_rx[IEEE80211_MAX_AMPDU_BUF_HT];
    int ampdus_rx_map[4];
    u32 ampdus_rx_miss;
#ifdef CONFIG_SIWIFI_SPLIT_TX_BUF
    struct siwifi_amsdu_stats amsdus[NX_TX_PAYLOAD_MAX];
#endif
    u32 tx_failed;
    u32 tx_copy_expand;
    u32 tx_drop_full;
    u32 tx_drop_sta_null;
    u32 tx_drop_txq_inactive;
    u32 tx_drop_hdr_alloc_fail;
    u32 tx_drop_prep_tx;
    u32 tx_retry;
    u32 single_retry;
    u32 tx_flush;
    u32 tx_discard;
    u64 tx_pkt;
    u32 amsdus_rx[64];
 unsigned int delete_rx;
 unsigned int monitor_rx;
 unsigned int len_update_rx;
 unsigned int mgmt_rx;
 unsigned int spurious_rx;
    u32 ldpc_rx;
    u32 bcc_rx;
    u32 stbc_rx;
    u32 ipc_pending_cnt;
    u64 total_rx;
    u64 rxind_cnts;
    int queues_stops;
    int queues_starts;
};
#endif
struct siwifi_sec_phy_chan {
    u16 prim20_freq;
    u16 center_freq1;
    u16 center_freq2;
    enum nl80211_band band;
    u8 type;
};
struct siwifi_roc_elem {
    struct wireless_dev *wdev;
    struct ieee80211_channel *chan;
    unsigned int duration;
    bool mgmt_roc;
    bool on_chan;
};
struct siwifi_survey_info {
    u32 filled;
    u32 chan_time_ms;
    u32 chan_time_busy_ms;
    s8 noise_dbm;
};
#define SIWIFI_CH_NOT_SET 0xFF
#define SIWIFI_INVALID_VIF 0xFF
#define SIWIFI_INVALID_STA 0xFF
struct siwifi_chanctx {
    struct cfg80211_chan_def chan_def;
    u8 count;
};
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
enum ate_op_flag {
    CREATE_CHANTX_OP = 0,
    UPDATE_CHANTX_OP,
    DELETE_CHANTX_OP,
    SET_HW_ACTIVE,
    SET_HW_IDLE,
};
enum format_mode {
    NON_HT_MODE = 0,
    NON_HT_DUP_MODE,
    HT_MM_MODE,
    HT_GF_MODE,
    VHT_MODE,
};
enum payload_format {
    user_payload_format = 1,
    inc_payload_format,
};
struct siwifi_ate_rate_params {
    enum format_mode mode;
    unsigned int rate_idx;
    bool use_short_preamble;
    bool use_short_gi;
};
struct siwifi_ate_package_params {
    unsigned int frame_num;
    unsigned int frame_len;
    unsigned int tx_power;
    uint8_t payload[4096];
    unsigned int payload_mode;
    enum payload_format mode;
    unsigned int frame_bw;
    bool cont_flag;
    unsigned int macbypass_interval;
};
struct siwifi_ate_machdr_params {
    unsigned char siwifi_da[6];
    unsigned char siwifi_sa[6];
    unsigned char siwifi_bssid[6];
    unsigned short fc;
    unsigned short dur;
    unsigned short seqc;
};
struct siwifi_ate_qos_machdr_params {
    unsigned char siwifi_da[6];
    unsigned char siwifi_sa[6];
    unsigned char siwifi_bssid[6];
    unsigned short fc;
    unsigned short dur;
    unsigned short seqc;
    unsigned short qosc;
};
struct siwifi_ate_config_params {
    struct cfg80211_chan_def chandef;
    struct siwifi_ate_rate_params rate;
    struct siwifi_ate_package_params pkg;
    struct siwifi_ate_machdr_params machdr;
    struct siwifi_ate_qos_machdr_params qos_machdr;
};
struct siwifi_ate_reg {
    unsigned int read_reg_val;
    unsigned int origi_val1;
    int status;
};
struct siwifi_ate_env {
    bool ate_start;
    bool vif_ctx_flag;
    bool tx_frame_start;
 bool tx_macbypass_start;
    bool agg_tx;
    bool agg_action;
    int tx_total_count;
    int tx_left_count;
    int tx_count;
    int tx_retry_count;
    int tx_successful_count;
    int tx_agg_done;
    int tx_agg_retry;
    int hw_queue;
    int tx_queue;
    struct siwifi_vif *siwifi_vif;
    struct siwifi_chanctx *siwifi_chanctx;
    struct ieee80211_tx_control *control;
    struct ieee80211_ampdu_params *params;
    struct tasklet_struct tx_task;
    bool rx_frame_start;
 bool rx_macbypass_start;
    unsigned int rec_rx_count;
    unsigned int all_err;
    unsigned int phy_err;
    unsigned int fcs_err;
 unsigned int fcs_ok;
    unsigned int per;
 unsigned int fcs_ok_for_macaddr;
 unsigned int fcs_group;
    int rssi;
    struct siwifi_ate_config_params conf;
    struct siwifi_ate_reg reg;
    struct cfg80211_ops ate_ops;
};
#endif
#if 0
#define CB_LOCK_INIT() spin_lock_init(&siwifi_hw->cb_lock)
#define CB_LOCK(bh) \
    do { \
        bh ? spin_lock_bh(&siwifi_hw->cb_lock) : spin_lock(&siwifi_hw->cb_lock); \
    }while(0)
#define CB_UNLOCK(bh) \
    do { \
        bh ? spin_unlock_bh(&siwifi_hw->cb_lock) : spin_unlock(&siwifi_hw->cb_lock); \
    }while(0)
#endif
#ifdef CONFIG_SIWIFI_ACS
struct vendor_channel_data {
    u8 chan;
    enum nl80211_band band;
    u32 freq;
    u8 flag;
    u32 allowed_bw;
    struct list_head survey_list;
    s8 min_nf;
    u32 interference_factor;
};
struct hostapd_acs_chan_param {
 uint32_t vif_index;
 uint16_t ht_enabled:1,
    ht40_enabled:1,
    vht_enabled:1;
 uint8_t primary_chan;
 uint8_t sec_chan;
 uint8_t hw_mode;
 uint8_t ch_width;
 uint8_t seg0_chan;
 uint8_t seg1_chan;
};
struct hostapd_acs_chan_result {
 uint32_t last_time;
 uint8_t primary_chan;
 uint8_t sec_chan;
 uint8_t hw_mode;
 uint8_t ch_width;
 uint8_t seg0_chan;
 uint8_t seg1_chan;
};
#endif
#ifdef CONFIG_SF19A28_WIFI_LED
#define SIWIFI_LED_MAX_NAME_LEN 31
#define LED_GPIO_LABEL "gpio-leds"
struct siwifi_led {
 char name[SIWIFI_LED_MAX_NAME_LEN + 1];
 struct led_classdev led_dev;
 struct siwifi_hw *hw;
};
#endif
#ifdef CONFIG_SIWIFI_TEMPERATURE_CONTROL
#define TEMP_CTL_DEFAULT_TRIGGER_TEMP 40
#define TEMP_CTL_DEFAULT_JUMP_LIMIT 5
struct siwifi_temp_ctl {
    int temp_ctl_parameters[9];
    int temp_ctl_level;
    int last_change_temp;
};
#endif
#ifdef CONFIG_WIFI_RX_THREAD
struct rx_thread_dbginfo {
    uint32_t elt_cur_cnt;
    uint32_t elt_max_cnt;
    uint32_t skb_cur_cnt;
    uint32_t skb_max_cnt;
    uint32_t skb_max_per_elt;
    uint32_t elt_loop_max_cnt;
};
#endif
struct siwifi_atf {
    uint8_t enable;
    uint32_t last_update_time;
    uint8_t txq_nosent_cnt;
    uint32_t max_rateinfo;
    uint32_t update_cnt;
};
#ifdef CONFIG_SIWIFI_IQENGINE
struct siwifi_iqe {
    bool init;
    uint32_t error;
    int mode;
    void *iq_buffer;
    uint32_t iq_buffer_phy;
    uint32_t iq_buffer_len;
    uint32_t iq_irq;
};
#endif
#ifdef CONFIG_SIWIFI_EASYMESH
#define WLAN_WNM_BTM_QUERY 6
#define WLAN_PA_GAS_INITIAL_REQ 10
#define WLAN_WNM_NOTIFY_REQ 26
typedef enum {
    SF_NOTIFY_STA_CHANGE_EVENT = 0,
    SF_NOTIFY_STA_INFO_EVENT,
    SF_NOTIFY_MGMT_RX_EVENT,
} sf_notify_event_type;
typedef enum {
    SF_TUNNELED_MSG_ASSOC_REQ = 0x00,
    SF_TUNNELED_MSG_REASSOC_REQ = 0x01,
    SF_TUNNELED_MSG_BTM_QUERY = 0x02,
    SF_TUNNELED_MSG_WNM_REQ = 0x03,
    SF_TUNNELED_MSG_ANQP_NEIGHBOR_REQ = 0x04,
} sf_thnneled_frame_type;
typedef struct {
    sf_notify_event_type type;
    union {
        struct {
            const u8 *sta_mac;
            const u8 *prev_mac;
            bool updown;
        } sta_change_event;
        struct {
            const u8 *sta_mac;
            const u8 *prev_mac;
            int8_t rssi;
        } sta_info_event;
        struct {
            u8 *frame_data;
            uint16_t frame_length;
            sf_thnneled_frame_type frame_type;
        } mgmt_rx_event;
    } data;
} sf_notify_event_data;
typedef struct {
    struct timer_list timer;
    uint8_t timeout;
    struct siwifi_hw *siwifi_hw;
} sf_sta_timer_info;
typedef void (*siwifi_event_callback)(const sf_notify_event_data *event_data);
void report_sf_notify_event(const sf_notify_event_data *event_data);
#endif
#ifdef CONFIG_SIWIFI_SORT_SCAN
#define MAX_SCAN_BSS_CNT 64
#endif
struct siwifi_hw {
    struct siwifi_mod_params *mod_params;
    bool use_phy_bw_tweaks;
    struct device *dev;
    struct wiphy *wiphy;
    struct list_head vifs;
    struct siwifi_vif *vif_table[NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX];
    struct siwifi_sta sta_table[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
#ifdef LOCAL_RECOVERY
    struct siwifi_sta_rec_info local_sta_table[NX_REMOTE_STA_MAX];
    struct siwifi_sta *recovering_sta;
    u8 recovering_sta_idx;
#endif
    bool dying_gasp_valid;
    struct siwifi_ipc_elem_var dying_gasp_info_elem;
    struct siwifi_survey_info survey[SCAN_CHANNEL_MAX];
    struct cfg80211_scan_request *scan_request;
    int scan_timeout;
    int enable_dbg_sta_conn;
    struct siwifi_chanctx chanctx_table[NX_CHAN_CTXT_CNT];
    u8 cur_chanctx;
    struct siwifi_trace_ctx trace_ctx;
#ifdef CONFIG_SIWIFI_ACS_INTERNAL
    bool acs_internal;
    struct work_struct csa_work;
    struct cfg80211_chan_def csa_chandef;
    uint32_t last_time;
#endif
#if defined (CONFIG_SIWIFI_ACS) || defined (CONFIG_SIWIFI_ACS_INTERNAL)
    bool acs_scan;
    struct cfg80211_scan_request *acs_request;
#endif
#ifdef CONFIG_SIWIFI_ACS
 struct vendor_channel_data chan_data[SCAN_CHANNEL_MAX];
 struct hostapd_acs_chan_param acs_params[NX_VIRT_DEV_MAX];
 struct hostapd_acs_chan_result acs_result;
#endif
    u8 monitor_vif;
    struct siwifi_roc_elem *roc_elem;
    u32 roc_cookie_cnt;
    struct siwifi_cmd_mgr cmd_mgr;
#ifdef CONFIG_SF19A28_WIFI_LED
 struct siwifi_led rx_led;
 struct siwifi_led tx_led;
 unsigned int led_pin,led_on;
 struct led_trigger local_tx_led, local_rx_led;
 atomic_t tx_led_active, rx_led_active;
#endif
    unsigned long drv_flags;
    struct siwifi_plat *plat;
    spinlock_t tx_lock;
    spinlock_t cb_lock;
    struct mutex mutex;
    struct tasklet_struct task;
    struct mm_version_cfm version_cfm;
#ifdef CONFIG_HEART_BEAT
    struct delayed_work heart_work;
    unsigned int recovery_hb_num;
#endif
    struct delayed_work txq_stat_work;
    u32 tcp_pacing_shift;
    u32 task_max_process_time;
    u64 total_rx;
    struct siwifi_ipc_elem_var debug_frame_buf;
    struct ipc_host_env_tag *ipc_env;
    struct siwifi_ipc_elem_pool e2amsgs_pool;
    struct siwifi_ipc_elem_pool dbgmsgs_pool;
    struct siwifi_ipc_elem_pool e2aradars_pool;
    struct siwifi_ipc_elem_var pattern_elem;
    struct siwifi_ipc_dbgdump_elem dbgdump_elem;
    struct siwifi_ipc_elem_pool e2arxdesc_pool;
    struct siwifi_ipc_skb_elem *e2aunsuprxvec_elems;
    struct siwifi_ipc_rxbuf_elems rxbuf_elems;
    struct siwifi_ipc_elem_var scan_ie;
    struct siwifi_ipc_elem_var dbg_mgmt_info_elem;
    struct siwifi_ipc_elem_var dbg_ctrl_info_elem;
#ifdef CONFIG_SIWIFI_TRX_STAT
    struct siwifi_ipc_elem_var dbg_trx_stat_elem;
#endif
#ifdef NEW_SCHEDULE
    struct siwifi_ipc_elem_pool a2etxdesc_pool[CONFIG_USER_MAX][NX_TXQ_CNT];
#endif
#ifdef CONFIG_SIWIFI_CACHE_ALLOC
    struct kmem_cache *sw_txhdr_cache;
#endif
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    struct siwifi_debugfs debugfs;
    struct siwifi_stats stats;
    uint32_t enable_rc;
    uint32_t tx_ctrl;
    uint32_t rx_ctrl;
    uint32_t debug_get_survey;
    uint32_t disable_wmm_edca;
    uint32_t wmm_edca_interval;
    uint32_t wmm_edca_counter_drop;
    uint32_t wmm_edca_pkt_threshold;
    uint32_t wmm_debug_enable;
    uint32_t amsdu_threshold;
    uint32_t debug_frame_statinfo;
    uint32_t amsdu_nb_disable;
    uint32_t amsdu_nb_percent;
    uint32_t amsdu_nb_cleanup;
    uint32_t amsdu_nb_threshold;
#endif
#ifdef CONFIG_SIWIFI_PROCFS
 struct proc_dir_entry *procfsdir;
#endif
    struct siwifi_txq txq[NX_NB_TXQ];
    struct siwifi_hwq hwq[NX_TXQ_CNT];
    struct siwifi_sec_phy_chan sec_phy_chan;
    u8 phy_cnt;
    u8 chan_ctxt_req;
    u8 avail_idx_map;
    u8 vif_started;
    bool adding_sta;
    struct phy_cfg_tag phy_config;
    struct siwifi_radar radar;
    u8 ext_capa[8];
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    struct siwifi_mu_info mu;
#endif
#ifdef LOCAL_RECOVERY
    struct work_struct restart_work;
#endif
 struct work_struct restart_work_rfcmd;
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
    struct siwifi_ate_env ate_env;
#endif
    s16 hwq_credits_dec[NX_TXQ_CNT];
#ifdef CONFIG_SIWIFI_TX_POWER_CALI
    struct siwifi_factory_info factory_info;
#endif
#ifdef CONFIG_SIWIFI_COOLING_TEMP
 int change_power;
    int change_power_trend;
    uint32_t temp_disable;
    u8 max_txpower_idx;
    enum sfax8_rf_cooling_state cool_state;
#endif
#ifdef CONFIG_SIWIFI_IGMP
    struct multicast_group *mul_group;
#endif
#ifdef CONFIG_SIWIFI_TEMPERATURE_CONTROL
struct siwifi_temp_ctl temp_ctl;
#endif
    bool scaning;
 struct completion scan_complete;
 bool wait_scan_complete;
    int tx_queue_num;
    uint32_t rx_skb_alloc_fail_cnt;
    bool lm_ctrl_enable;
    struct list_head stuck_check_list;
    uint32_t stuck_check_time;
#ifdef CONFIG_WIFI_RX_THREAD
    struct task_struct *rx_kthread;
    spinlock_t rx_lock;
    struct list_head rx_skbs;
    atomic_t rx_pending_count;
    wait_queue_head_t rx_wq;
    struct rx_thread_dbginfo rx_thread_dbg;
#endif
#ifdef CONFIG_SIWIFI_SORT_SCAN
    struct ipc_e2a_msg scan_results[MAX_SCAN_BSS_CNT];
    struct ipc_e2a_msg result_temp;
    struct cfg80211_bss *bss_results[MAX_SCAN_BSS_CNT];
    uint8_t scan_num;
#endif
    struct siwifi_atf atf;
    char *assoc_req_insert_info;
    struct me_assoc_req_insert_info_req assoc_insert;
#ifdef CONFIG_SF_SKB_POOL
 struct skb_pool_param_t *skb_pool_dev_param;
#endif
    u32 fixed_gain;
    uint32_t fast_csa_time;
#ifdef CONFIG_SIWIFI_IQENGINE
    struct siwifi_iqe iqe;
#endif
    uint32_t disable_cca_channel_switch;
    uint32_t reord_release_cnt;
    uint32_t reord_alloc_cnt;
#ifdef CONFIG_SIWIFI_CH_OFFSET
    int ch_offset;
#endif
#ifdef CONFIG_SIWIFI_EASYMESH
    sf_sta_timer_info sta_timer_info;
#endif
};
#ifdef CONFIG_SF_SKB_POOL
#define MAX_WIFI_POOL_SKB_RAW_SIZE ( 2816)
#endif
#ifdef CONFIG_SIWIFI_CACHE_ALLOC
extern struct kmem_cache *sw_txhdr_cache;
#endif
u8 *siwifi_build_bcn(struct siwifi_bcn *bcn, struct cfg80211_beacon_data *new, u8 **real_addr);
void siwifi_chanctx_link(struct siwifi_vif *vif, u8 idx,
                        struct cfg80211_chan_def *chandef);
void siwifi_chanctx_unlink(struct siwifi_vif *vif);
int siwifi_chanctx_valid(struct siwifi_hw *siwifi_hw, u8 idx);
static inline bool is_multicast_sta(int sta_idx)
{
    return (sta_idx >= NX_REMOTE_STA_MAX);
}
struct siwifi_sta *siwifi_get_sta(struct siwifi_hw *siwifi_hw, const u8 *mac_addr);
static inline uint8_t master_vif_idx(struct siwifi_vif *vif)
{
    if (unlikely(vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN)) {
        return vif->ap_vlan.master->vif_index;
    } else {
        return vif->vif_index;
    }
}
void siwifi_external_auth_enable(struct siwifi_vif *vif);
void siwifi_external_auth_disable(struct siwifi_vif *vif);
struct siwifi_sw_txhdr *siwifi_alloc_swtxhdr(struct siwifi_hw *siwifi_hw, struct sk_buff *skb);
void siwifi_free_swtxhdr(struct siwifi_hw *siwifi_hw, struct siwifi_sw_txhdr *sw_txhdr);
void siwifi_sta_hash_del(struct siwifi_vif *vif, struct siwifi_sta *sta);
void siwifi_sta_hash_add(struct siwifi_vif *vif, struct siwifi_sta *sta);
struct siwifi_sta *siwifi_sta_hash_get(struct siwifi_vif *vif, const u8 *addr);
int siwifi_check_skb_is_dhcp(struct sk_buff *skb);
void siwifi_update_src_filter(struct siwifi_vif *siwifi_vif, unsigned char *src_mac);
struct siwifi_src_filter *siwifi_src_filter_hash_search(struct siwifi_vif *siwifi_vif, unsigned char *src_mac);
void siwifi_src_filter_hash_free(struct siwifi_vif *siwifi_vif);
void src_filter_aging(struct timer_list *timer);
void set_repeater_status(struct siwifi_vif *siwifi_vif, u8 status);
int siwifi_set_assoc_req_insert_info(struct siwifi_hw *siwifi_hw, char *insert_info, int insert_length);
int siwifi_channel_recovery_check(struct siwifi_hw *siwifi_hw);
#endif
