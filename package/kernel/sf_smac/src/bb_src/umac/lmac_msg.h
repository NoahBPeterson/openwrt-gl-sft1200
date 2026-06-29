#ifndef LMAC_MSG_H_
#define LMAC_MSG_H_ 
#include "lmac_types.h"
#include "lmac_mac.h"
enum
{
    TASK_NONE = (u8_l) -1,
    TASK_MM = 0,
    TASK_DBG,
    TASK_SCAN,
    TASK_TDLS,
    TASK_SCANU,
    TASK_ME,
    TASK_SM,
    TASK_APM,
    TASK_BAM,
    TASK_MESH,
    TASK_RXU,
    TASK_LAST_EMB = TASK_RXU,
    TASK_API,
    TASK_MAX,
};
enum
{
    HW_IDLE = 0,
    HW_RESERVED,
    HW_DOZE,
    HW_ACTIVE
};
enum mm_ps_mode_state
{
    MM_PS_MODE_OFF,
    MM_PS_MODE_ON,
    MM_PS_MODE_ON_DYN,
    MM_PS_MODE_OFF_FORCE,
};
enum
{
    CO_OK,
    CO_FAIL,
    CO_EMPTY,
    CO_FULL,
    CO_BAD_PARAM,
    CO_NOT_FOUND,
    CO_NO_MORE_ELT_AVAILABLE,
    CO_NO_ELT_IN_USE,
    CO_BUSY,
    CO_OP_IN_PROGRESS,
};
enum mm_remain_on_channel_op
{
    MM_ROC_OP_START = 0,
    MM_ROC_OP_CANCEL,
};
#define DRV_TASK_ID 100
typedef u16 lmac_msg_id_t;
typedef u16 lmac_task_id_t;
#define LMAC_FIRST_MSG(task) ((lmac_msg_id_t)((task) << 10))
#define MSG_T(msg) ((lmac_task_id_t)((msg) >> 10))
#define MSG_I(msg) ((msg) & ((1<<10)-1))
struct co_list_hdr
{
    struct co_list_hdr *next;
};
struct co_list
{
    struct co_list_hdr *first;
    struct co_list_hdr *last;
    u32_l cnt;
};
struct lmac_msg
{
    lmac_msg_id_t id;
    lmac_task_id_t dest_id;
    lmac_task_id_t src_id;
    u16 param_len;
    u32 param[];
};
enum mm_msg_tag
{
    MM_RESET_REQ = LMAC_FIRST_MSG(TASK_MM),
    MM_RESET_CFM,
    MM_START_REQ,
    MM_START_CFM,
    MM_VERSION_REQ,
    MM_VERSION_CFM,
    MM_ADD_IF_REQ,
    MM_ADD_IF_CFM,
    MM_REMOVE_IF_REQ,
    MM_REMOVE_IF_CFM,
    MM_STA_ADD_REQ,
    MM_STA_ADD_CFM,
    MM_STA_DEL_REQ,
    MM_STA_DEL_CFM,
    MM_SET_FILTER_REQ,
    MM_SET_FILTER_CFM,
    MM_SET_CHANNEL_REQ,
    MM_SET_CHANNEL_CFM,
    MM_SET_DTIM_REQ,
    MM_SET_DTIM_CFM,
    MM_SET_BEACON_INT_REQ,
    MM_SET_BEACON_INT_CFM,
    MM_SET_BASIC_RATES_REQ,
    MM_SET_BASIC_RATES_CFM,
    MM_SET_BSSID_REQ,
    MM_SET_BSSID_CFM,
    MM_SET_EDCA_REQ,
    MM_SET_EDCA_CFM,
    MM_SET_MODE_REQ,
    MM_SET_MODE_CFM,
    MM_SET_VIF_STATE_REQ,
    MM_SET_VIF_STATE_CFM,
    MM_SET_SLOTTIME_REQ,
    MM_SET_SLOTTIME_CFM,
    MM_SET_IDLE_REQ,
    MM_SET_IDLE_CFM,
    MM_KEY_ADD_REQ,
    MM_KEY_ADD_CFM,
    MM_KEY_DEL_REQ,
    MM_KEY_DEL_CFM,
    MM_DEFAULT_KEY_ADD_REQ,
    MM_DEFAULT_KEY_ADD_CFM,
    MM_BA_ADD_REQ,
    MM_BA_ADD_CFM,
    MM_BA_DEL_REQ,
    MM_BA_DEL_CFM,
    MM_PRIMARY_TBTT_IND,
    MM_SECONDARY_TBTT_IND,
    MM_SET_POWER_REQ,
    MM_SET_POWER_CFM,
    MM_DBG_TRIGGER_REQ,
    MM_SET_PS_MODE_REQ,
    MM_SET_PS_MODE_CFM,
    MM_CHAN_CTXT_ADD_REQ,
    MM_CHAN_CTXT_ADD_CFM,
    MM_CHAN_CTXT_DEL_REQ,
    MM_CHAN_CTXT_DEL_CFM,
    MM_CHAN_CTXT_LINK_REQ,
    MM_CHAN_CTXT_LINK_CFM,
    MM_CHAN_CTXT_UNLINK_REQ,
    MM_CHAN_CTXT_UNLINK_CFM,
    MM_CHAN_CTXT_UPDATE_REQ,
    MM_CHAN_CTXT_UPDATE_CFM,
    MM_CHAN_CTXT_SCHED_REQ,
    MM_CHAN_CTXT_SCHED_CFM,
    MM_BCN_CHANGE_REQ,
    MM_BCN_CHANGE_CFM,
    MM_TIM_UPDATE_REQ,
    MM_TIM_UPDATE_CFM,
    MM_CONNECTION_LOSS_IND,
    MM_CHANNEL_SWITCH_IND,
    MM_CHANNEL_PRE_SWITCH_IND,
    MM_REMAIN_ON_CHANNEL_REQ,
    MM_REMAIN_ON_CHANNEL_CFM,
    MM_REMAIN_ON_CHANNEL_EXP_IND,
    MM_PS_CHANGE_IND,
    MM_TRAFFIC_REQ_IND,
    MM_SET_PS_OPTIONS_REQ,
    MM_SET_PS_OPTIONS_CFM,
    MM_P2P_VIF_PS_CHANGE_IND,
    MM_CSA_COUNTER_IND,
    MM_CHANNEL_SURVEY_IND,
    MM_BFMER_ENABLE_REQ,
    MM_SET_P2P_NOA_REQ,
    MM_SET_P2P_OPPPS_REQ,
    MM_SET_P2P_NOA_CFM,
    MM_SET_P2P_OPPPS_CFM,
    MM_P2P_NOA_UPD_IND,
    MM_CFG_RSSI_REQ,
    MM_RSSI_STATUS_IND,
    MM_CSA_FINISH_IND,
    MM_CSA_TRAFFIC_IND,
    MM_MU_GROUP_UPDATE_REQ,
    MM_MU_GROUP_UPDATE_CFM,
    MM_ANT_DIV_INIT_REQ,
    MM_ANT_DIV_STOP_REQ,
    MM_ANT_DIV_UPDATE_REQ,
    MM_SWITCH_ANTENNA_REQ,
    MM_PKTLOSS_IND,
    MM_RF_CTRL_REQ,
    MM_RF_CTRL_CFM,
    MM_RF_CALI_REQ,
    MM_RF_CALI_CFM,
    MM_RF_CMD_DUMP_REQ,
    MM_RF_CMD_DUMP_CFM,
    MM_FORCE_IDLE_REQ,
    MM_SCAN_CHANNEL_START_IND,
    MM_SCAN_CHANNEL_END_IND,
    MM_TBTT_MOVE_REQ,
    MM_RF_CTRL_DONE,
    MM_INIT_COMPLETE,
    MM_LA_INIT_IND,
    MM_HEART_REQ,
    MM_HEART_CFM,
    MM_TXPOWER_CALI_ENABLE_REQ,
    MM_RF_GET_TEMP_REQ,
    MM_RF_GET_TEMP_CFM,
    MM_GET_CHAN_INFO,
    MM_SET_POWER_LVL_REQ,
 MM_SET_ANTENNA_NUMBER_REQ,
    MM_COOLING_TEMP_REQ,
 MM_CHANGE_POWER_REQ,
    MM_VIF_DUMP_REQ,
    MM_VIF_DUMP_CFM,
    MM_DYING_GASP_IND,
    MM_UPDATE_IDLE_TIME_IND,
    MM_SEND_NULL_FRAME_REQ,
    MM_GAIN_CONTROL_REQ,
    MM_CCA_DROP_STEP_IND,
    MM_GET_CHANNEL_INFO_REQ,
    MM_GET_CHANNEL_INFO_CFM,
    MM_SEND_DEBUG_FRAME_REQ,
    MM_SEND_DEBUG_FRAME_IND,
    MM_MAX,
};
struct mm_send_debug_frame_ind
{
    u32_l statinfo;
};
struct mm_send_debug_frame_req
{
    u8_l vif_idx;
    u8_l ac;
    u8_l bw;
    u8_l mcs;
    u8_l nss;
    u8_l format_mod;
    u8_l gi;
    u8_l ldpc;
    u8_l expect_ack;
    u16_l frmlen;
    u32_l payload_addr;
};
enum
{
    MM_STA,
    MM_IBSS,
    MM_AP,
    MM_MESH_POINT,
    MM_MONITOR,
};
enum
{
    BA_AGMT_TX,
    BA_AGMT_RX,
};
enum
{
    BA_AGMT_ESTABLISHED,
    BA_AGMT_ALREADY_EXISTS,
    BA_AGMT_DELETED,
    BA_AGMT_DOESNT_EXIST,
};
enum mm_features
{
    MM_FEAT_BCN_BIT = 0,
    MM_FEAT_AUTOBCN_BIT,
    MM_FEAT_HWSCAN_BIT,
    MM_FEAT_CMON_BIT,
    MM_FEAT_MROLE_BIT,
    MM_FEAT_RADAR_BIT,
    MM_FEAT_PS_BIT,
    MM_FEAT_UAPSD_BIT,
    MM_FEAT_DPSM_BIT,
    MM_FEAT_AMPDU_BIT,
    MM_FEAT_AMSDU_BIT,
    MM_FEAT_CHNL_CTXT_BIT,
    MM_FEAT_REORD_BIT,
    MM_FEAT_P2P_BIT,
    MM_FEAT_P2P_GO_BIT,
    MM_FEAT_UMAC_BIT,
    MM_FEAT_VHT_BIT,
    MM_FEAT_BFMEE_BIT,
    MM_FEAT_BFMER_BIT,
    MM_FEAT_WAPI_BIT,
    MM_FEAT_MFP_BIT,
    MM_FEAT_MU_MIMO_RX_BIT,
    MM_FEAT_MU_MIMO_TX_BIT,
    MM_FEAT_MESH_BIT,
    MM_FEAT_TDLS_BIT,
    MM_FEAT_ANT_DIV_BIT,
    MM_FEAT_UF_BIT,
    MM_AMSDU_MAX_SIZE_BIT0,
    MM_AMSDU_MAX_SIZE_BIT1,
    MM_FEAT_MON_DATA_BIT,
    MM_FEAT_HE_BIT,
    MM_FEAT_LITE_MEM_BIT,
};
#define PHY_CFG_BUF_SIZE 16
struct phy_cfg_tag
{
    u32_l parameters[PHY_CFG_BUF_SIZE];
#ifdef CONFIG_ENABLE_DIGGAINTABLE
 uint32_t digtable[7];
 uint32_t digtable_max[7];
#endif
#ifdef CONFIG_ENABLE_TXPOWERTABLE
 uint8_t lb_power_gain_tb[31];
 uint8_t hb_power_gain_tb[31];
#endif
#ifdef CONFIG_ENABLE_RFGAINTABLE
 uint8_t lb_rf_gain_tb_idx[4];
 uint8_t hb_rf_gain_tb_idx[4];
#endif
};
struct phy_trd_cfg_tag
{
    u8_l path_mapping;
    u32_l tx_dc_off_comp;
};
struct phy_karst_cfg_tag
{
    u32_l tx_iq_comp_2_4G[2];
    u32_l rx_iq_comp_2_4G[2];
    u32_l tx_iq_comp_5G[2];
    u32_l rx_iq_comp_5G[2];
    u8_l path_used;
};
enum phy_cfg_tag_flag
{
    POWER_CALI_DISABLE = 0,
    RF_ENHANCE_LOWRATE_POWER = 1 << 0,
    TXPOWER_CALI_TABLE = 1 << 1,
    RF_EXTERNAL_PA_ENABLE = 1 << 2,
    ATE_TEST_ON = 1 << 3,
 DUAL_ANTENNA_CALIBRATE = 1 << 4,
};
struct phy_aetnensis_cfg_tag
{
    u32_l version;
    u32_l mapping_len;
    u32_l address;
 u32_l sleepmode_address;
 u32_l low_address;
    u32_l normal_address;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    u32_l address_second;
 u32_l sleepmode_address_second;
 u32_l low_address_second;
 u32_l normal_address_second;
#endif
    u32_l high_address;
    u32_l gain_num;
    u32_l chan_num;
    u32_l flag;
};
enum mm_start_flags{
    MM_START_GLOBAL_FLAG_ADDR_MASK_ALL = 0x1 << 0,
 MM_START_GLOBAL_FLAG_NOT_SEND_NULL = 0x1 << 1,
};
#define MM_ENV_CFG_MAX 8
enum MM_GLOBAL_PARAMETERS
{
    MM_AMPDU_MAX_CNT = 0,
#ifdef INDEPENDENT_ANTENNA_CONTROL
 MM_INDEPENDENT_ANTENNA_CONTROL = 1,
#endif
 RTS_CTS_CHANGE = 1,
};
struct mm_start_req
{
    struct phy_cfg_tag phy_cfg;
    u32_l uapsd_timeout;
    u16_l lp_clk_accuracy;
    u32_l global_flags;
    uint32_t parameters[MM_ENV_CFG_MAX];
};
struct mm_set_channel_req
{
    u8_l band;
    u8_l type;
    u16_l prim20_freq;
    u16_l center1_freq;
    u16_l center2_freq;
    u8_l index;
    s8_l tx_power;
 u8_l setchannel;
#ifdef CONFIG_SIWIFI_CH_OFFSET
    int channel_off;
    int8_t mode;
#endif
};
struct mm_set_channel_cfm
{
    u8_l radio_idx;
    s8_l power;
};
struct mm_set_dtim_req
{
    u8_l dtim_period;
};
struct mm_set_power_req
{
    u8_l inst_nbr;
    s8_l power;
};
struct mm_set_power_lvl_req
{
    u8_l power_lvl;
};
#define MM_GAIN_CONTROL_ACTION_FIX_GAIN 0x1
struct mm_set_gain_control_req
{
    u8_l action;
    int param1;
    int param2;
};
struct mm_set_antenna_number_req
{
    u8_l antenna_number;
};
struct mm_set_power_cfm
{
    u8_l radio_idx;
    s8_l power;
};
struct mm_set_beacon_int_req
{
    u16_l beacon_int;
    u8_l inst_nbr;
};
struct mm_set_basic_rates_req
{
    u32_l rates;
    u8_l inst_nbr;
    u8_l band;
};
struct mm_set_bssid_req
{
    struct mac_addr bssid;
    u8_l inst_nbr;
};
struct mm_set_filter_req
{
    u32_l filter;
};
struct mm_add_if_req
{
    u8_l type;
    struct mac_addr addr;
    bool_l p2p;
    bool_l uf;
    bool_l req_index_valid;
    u8_l req_index;
    bool_l use_4addr;
};
struct mm_set_edca_req
{
    u32_l ac_param;
    bool_l uapsd;
    u8_l hw_queue;
    u8_l inst_nbr;
};
struct mm_set_idle_req
{
    u8_l hw_idle;
};
struct mm_set_slottime_req
{
    u8_l slottime;
};
struct mm_set_mode_req
{
    u8_l abgnmode;
};
struct mm_set_vif_state_req
{
    u16_l aid;
    bool_l active;
    u8_l inst_nbr;
};
struct mm_add_if_cfm
{
    u8_l status;
    u8_l inst_nbr;
};
struct mm_remove_if_req
{
    u8_l inst_nbr;
};
struct mm_version_cfm
{
    u32_l version_lmac;
    u32_l version_machw_1;
    u32_l version_machw_2;
    u32_l version_phy_1;
    u32_l version_phy_2;
    u32_l features;
    u16_l max_sta_nb;
    u8_l max_vif_nb;
};
struct mm_sta_add_req
{
    u32_l ampdu_size_max_he;
    u32_l ampdu_size_max_vht;
    u32_l paid_gid;
    u16_l ampdu_size_max_ht;
    struct mac_addr mac_addr;
    u8_l ampdu_spacing_min;
    u8_l inst_nbr;
    bool_l tdls_sta;
    bool_l tdls_sta_initiator;
    bool_l tdls_chsw_allowed;
};
struct mm_sta_add_cfm
{
    u8_l status;
    u8_l sta_idx;
    u8_l hw_sta_idx;
};
struct mm_sta_del_req
{
    u8_l sta_idx;
};
struct mm_sta_del_cfm
{
    u8_l status;
};
struct mm_setpowermode_req
{
    u8_l mode;
    u8_l sta_idx;
};
struct mm_setpowermode_cfm
{
    u8_l status;
};
struct mm_key_add_req
{
    u8_l key_idx;
    u8_l sta_idx;
    struct mac_sec_key key;
    u8_l cipher_suite;
    u8_l inst_nbr;
    u8_l spp;
    bool_l pairwise;
    bool_l recovering;
    u64_l tx_pn;
};
struct mm_key_add_cfm
{
    u8_l status;
    u8_l hw_key_idx;
};
struct mm_key_del_req
{
    u8_l hw_key_idx;
};
struct mm_default_key_add_req
{
    uint8_t default_key_idx;
    uint8_t inst_nbr;
};
struct mm_default_key_add_cfm
{
    uint8_t status;
};
struct mm_ba_add_req
{
    u8_l type;
    u8_l sta_idx;
    u8_l tid;
    u8_l bufsz;
    u16_l ssn;
};
struct mm_ba_add_cfm
{
    u8_l sta_idx;
    u8_l tid;
    u8_l status;
};
struct mm_ba_del_req
{
    u8_l type;
    u8_l sta_idx;
    u8_l tid;
};
struct mm_ba_del_cfm
{
    u8_l sta_idx;
    u8_l tid;
    u8_l status;
};
struct mm_chan_ctxt_add_req
{
    u8_l band;
    u8_l type;
    u16_l prim20_freq;
    u16_l center1_freq;
    u16_l center2_freq;
    s8_l tx_power;
};
struct mm_chan_ctxt_add_cfm
{
    u8_l status;
    u8_l index;
};
struct mm_chan_ctxt_del_req
{
    u8_l index;
};
struct mm_chan_ctxt_link_req
{
    u8_l vif_index;
    u8_l chan_index;
    u8_l chan_switch;
};
struct mm_chan_ctxt_unlink_req
{
    u8_l vif_index;
};
struct mm_chan_ctxt_update_req
{
    u8_l chan_index;
    u8_l band;
    u8_l type;
    u16_l prim20_freq;
    u16_l center1_freq;
    u16_l center2_freq;
    s8_l tx_power;
};
struct mm_chan_ctxt_sched_req
{
    u8_l vif_index;
    u8_l chan_index;
    u8_l type;
};
struct mm_rf_ctrl_req
{
    uint8_t ctrl_id;
};
struct mm_rf_cali_req
{
    uint16_t conf;
    uint16_t chan;
    uint16_t bw;
};
enum mm_rf_ctrl_id
{
    MM_RF_CTRL_STOP_COMMUNICATION = 0,
    MM_RF_CTRL_START_COMMUNICATION,
    MM_RF_CTRL_MAX,
};
#ifdef CONFIG_SIWIFI_RF_CMD_DUMP
struct mm_rf_cmd_dump_req
{
    uint32_t address;
    uint32_t size;
    uint8_t flag;
};
struct mm_rf_cmd_dump_cfm
{
    uint32_t count;
    uint8_t status;
};
#endif
struct mm_rf_get_temp_cfm
{
    uint16_t value;
};
#ifdef CONFIG_SIWIFI_TX_POWER_CALI
struct mm_txpower_cali_enable_req
{
    u32_l enable_flag;
};
#endif
struct mm_channel_switch_ind
{
    u8_l chan_index;
    bool_l roc;
    u8_l vif_index;
    bool_l roc_tdls;
};
struct mm_channel_pre_switch_ind
{
    u8_l chan_index;
};
struct mm_connection_loss_ind
{
    u8_l inst_nbr;
};
struct mm_dbg_trigger_req
{
    uint8_t type;
    char error[64];
};
struct mm_set_ps_mode_req
{
    u8_l new_state;
};
#define BCN_MAX_CSA_CPT 2
struct mm_bcn_change_req
{
    u32_l bcn_ptr;
    u16_l bcn_len;
    u16_l tim_oft;
    u8_l tim_len;
    u8_l inst_nbr;
    u8_l csa_oft[BCN_MAX_CSA_CPT];
};
struct mm_tim_update_req
{
    u16_l aid;
    u8_l tx_avail;
    u8_l inst_nbr;
};
struct mm_remain_on_channel_req
{
    u8_l op_code;
    u8_l vif_index;
    u8_l band;
    u8_l type;
    u16_l prim20_freq;
    u16_l center1_freq;
    u16_l center2_freq;
    u32_l duration_ms;
    s8_l tx_power;
};
struct mm_remain_on_channel_cfm
{
    u8_l op_code;
    u8_l status;
    u8_l chan_ctxt_index;
};
struct mm_remain_on_channel_exp_ind
{
    u8_l vif_index;
    u8_l chan_ctxt_index;
};
struct mm_set_uapsd_tmr_req
{
    u8_l action;
    u32_l timeout;
};
struct mm_set_uapsd_tmr_cfm
{
    u8_l status;
};
struct mm_ps_change_ind
{
    u8_l sta_idx;
    u8_l ps_state;
};
struct mm_p2p_vif_ps_change_ind
{
    u8_l vif_index;
    u8_l ps_state;
};
struct mm_traffic_req_ind
{
    u8_l sta_idx;
    u8_l pkt_cnt;
    bool_l uapsd;
};
struct mm_set_ps_options_req
{
    u8_l vif_index;
    u16_l listen_interval;
    bool_l dont_listen_bc_mc;
};
struct mm_csa_counter_ind
{
    u8_l vif_index;
    u8_l csa_count;
};
struct mm_channel_survey_ind
{
    u16_l freq;
    s8_l noise_dbm;
    u32_l chan_time_ms;
    u32_l chan_time_busy_ms;
};
struct mm_bfmer_enable_req
{
    u32_l host_bfr_addr;
    u16_l host_bfr_size;
    u16_l aid;
    u8_l sta_idx;
    u8_l rx_nss;
    bool_l vht_mu_bfmee;
};
struct mm_set_p2p_noa_req
{
    u8_l vif_index;
    u8_l noa_inst_nb;
    u8_l count;
    bool_l dyn_noa;
    u32_l duration_us;
    u32_l interval_us;
    u32_l start_offset;
};
struct mm_set_p2p_oppps_req
{
    u8_l vif_index;
    u8_l ctwindow;
};
struct mm_set_p2p_noa_cfm
{
    u8_l status;
};
struct mm_set_p2p_oppps_cfm
{
    u8_l status;
};
struct mm_p2p_noa_upd_ind
{
    u8_l vif_index;
    u8_l noa_inst_nb;
    u8_l noa_type;
    u8_l count;
    u32_l duration_us;
    u32_l interval_us;
    u32_l start_time;
};
struct mm_cfg_rssi_req
{
    u8_l vif_index;
    s8_l rssi_thold;
    u8_l rssi_hyst;
};
struct mm_rssi_status_ind
{
    u8_l vif_index;
    bool_l rssi_status;
    s8_l rssi;
};
struct mm_pktloss_ind
{
    u8_l vif_index;
    struct mac_addr mac_addr;
    u32 num_packets;
};
struct mm_csa_finish_ind
{
    u8_l vif_index;
    u8_l status;
    u8_l chan_idx;
    u16_l center_freq;
    u8_l bw;
    u16_l center_freq1;
    u16_l center_freq2;
};
struct mm_csa_traffic_ind
{
    u8_l vif_index;
    bool_l enable;
};
struct mm_mu_group_update_req
{
    u8_l sta_idx;
    u8_l group_cnt;
    struct
    {
        u8_l group_id;
        u8_l user_pos;
    } groups[0];
};
struct mm_init_complete_ind
{
    uint8_t complete;
};
struct mm_init_la_ind
{
    uint8_t type;
    uint8_t enable;
};
struct mm_cooling_temp_req
{
    uint8_t max_pow_idx;
};
struct mm_change_power_req
{
    int change_power;
    int change_power_trend;
};
struct mm_send_null_frame_req
{
    uint8_t sta_idx;
};
#define MAX_VIF_KEY_NUM 6
struct key_dying_gasp {
    uint8_t key_idx;
    uint8_t hw_key_idx;
    uint64_t tx_pn;
};
struct vif_dying_gasp {
    uint8_t index;
    uint8_t key_len;
    struct key_dying_gasp keys[MAX_VIF_KEY_NUM];
};
struct sta_dying_gasp {
    uint8_t index;
    uint8_t vif_index;
    struct key_dying_gasp key;
    uint16_t seq_nbr[TID_MAX];
    uint8_t dialog_token_allocator;
};
struct mm_dying_gasp
{
    uint8_t vif_len;
    uint8_t sta_len;
    struct vif_dying_gasp vifs[NX_VIRT_DEV_MAX];
    struct sta_dying_gasp stas[NX_REMOTE_STA_MAX];
};
struct mm_dying_gasp_ind
{
    uint32_t dma_addr;
};
struct mm_update_idle_time_ind
{
    u8_l sta_idx;
};
struct mm_cca_drop_step_ind
{
    uint8_t cca_step;
};
enum scan_msg_tag
{
    SCAN_START_REQ = LMAC_FIRST_MSG(TASK_SCAN),
    SCAN_START_CFM,
    SCAN_DONE_IND,
    SCAN_CANCEL_REQ,
    SCAN_CANCEL_CFM,
    SCAN_MAX,
};
#define SCAN_SSID_MAX 2
#define SCAN_CHANNEL_2G4 14
#define SCAN_CHANNEL_5G 28
#define SCAN_CHANNEL_MAX (SCAN_CHANNEL_2G4 + SCAN_CHANNEL_5G)
#define SCAN_PASSIVE_BIT BIT(0)
#define SCAN_DISABLED_BIT BIT(1)
#define SCAN_BAND_MAX 2
struct scan_chan_tag
{
    u16_l freq;
    u8_l band;
    u8_l flags;
    s8_l tx_power;
};
struct scan_start_req
{
    struct scan_chan_tag chan[SCAN_CHANNEL_MAX];
    struct mac_ssid ssid[SCAN_SSID_MAX];
    struct mac_addr bssid;
    u32_l add_ies;
    u16_l add_ie_len;
    u8_l vif_idx;
    u8_l chan_cnt;
    u8_l ssid_cnt;
    bool no_cck;
};
struct scan_start_cfm
{
    u8_l status;
};
struct scan_cancel_req
{
};
struct scan_cancel_cfm
{
    u8_l status;
};
enum
{
    SCANU_START_REQ = LMAC_FIRST_MSG(TASK_SCANU),
    SCANU_START_CFM,
    SCANU_JOIN_REQ,
    SCANU_JOIN_CFM,
    SCANU_RESULT_IND,
    SCANU_FAST_REQ,
    SCANU_FAST_CFM,
    SCANU_MAX,
};
struct scanu_start_req
{
    struct scan_chan_tag chan[SCAN_CHANNEL_MAX];
    struct mac_ssid ssid[SCAN_SSID_MAX];
    struct mac_addr bssid;
    u32_l add_ies;
    u16_l add_ie_len;
    u8_l vif_idx;
    u8_l chan_cnt;
    u8_l ssid_cnt;
    bool no_cck;
};
struct scanu_start_cfm
{
    u8_l status;
};
struct scanu_result_ind
{
    u16_l length;
    u16_l framectrl;
    u16_l center_freq;
    u8_l band;
    u8_l sta_idx;
    u8_l inst_nbr;
    s8_l rssi;
    char cache_align[64];
    u32_l payload[];
};
struct scanu_fast_req
{
    struct mac_ssid ssid;
    struct mac_addr bssid;
    u16_l probe_delay;
    u16_l minch_time;
    u16_l maxch_time;
    u16_l ch_nbr;
};
enum
{
    ME_CONFIG_REQ = LMAC_FIRST_MSG(TASK_ME),
    ME_CONFIG_CFM,
    ME_CHAN_CONFIG_REQ,
    ME_CHAN_CONFIG_CFM,
    ME_SET_CONTROL_PORT_REQ,
    ME_SET_CONTROL_PORT_CFM,
    ME_TKIP_MIC_FAILURE_IND,
    ME_STA_ADD_REQ,
    ME_STA_ADD_CFM,
    ME_STA_DEL_REQ,
    ME_STA_DEL_CFM,
    ME_ASSOC_REQ_INSERT_INFO_REQ,
    ME_TX_CREDITS_UPDATE_IND,
    ME_TRAFFIC_IND_REQ,
    ME_TRAFFIC_IND_CFM,
    ME_RC_STATS_REQ,
    ME_RC_STATS_CFM,
    ME_RC_SET_RATE_REQ,
    ME_CONFIG_MONITOR_REQ,
    ME_CONFIG_MONITOR_CFM,
    ME_ATE_TOOLS_OP_REQ,
    ME_ATE_TOOLS_OP_CFM,
    ME_MAX,
};
struct me_config_req
{
    struct mac_htcapability ht_cap;
    struct mac_vhtcapability vht_cap;
    struct mac_hecapability he_cap;
    u16_l tx_lft;
    bool_l ht_supp;
    bool_l vht_supp;
    bool_l he_supp;
    bool_l ps_on;
    bool_l ant_div_on;
    u32_l rx_bufsz;
    u32_l rx_pd_offset;
};
struct me_chan_config_req
{
    struct scan_chan_tag chan2G4[SCAN_CHANNEL_2G4];
    struct scan_chan_tag chan5G[SCAN_CHANNEL_5G];
    u8_l chan2G4_cnt;
    u8_l chan5G_cnt;
};
struct me_set_control_port_req
{
    u8_l sta_idx;
    bool_l control_port_open;
};
struct me_tkip_mic_failure_ind
{
    struct mac_addr addr;
    u64_l tsc;
    bool_l ga;
    u8_l keyid;
    u8_l vif_idx;
};
struct me_sta_add_req
{
    struct mac_addr mac_addr;
    struct mac_rateset rate_set;
    struct mac_htcapability ht_cap;
    struct mac_vhtcapability vht_cap;
    struct mac_hecapability he_cap;
    u32_l flags;
    u16_l aid;
    u8_l uapsd_queues;
    u8_l max_sp_len;
    u8_l opmode;
    u8_l vif_idx;
    bool_l tdls_sta;
    bool_l tdls_sta_initiator;
    bool_l tdls_chsw_allowed;
    bool_l req_index_valid;
    u8_l req_index;
    bool_l recovering;
    u16_l seq_nbr[TID_MAX];
    u8_l dialog_token_allocator;
};
struct me_sta_add_cfm
{
    u8_l sta_idx;
    u8_l status;
    u8_l pm_state;
};
struct me_sta_del_req
{
    u8_l sta_idx;
    bool_l tdls_sta;
};
struct me_tx_credits_update_ind
{
    u8_l sta_idx;
    u8_l tid;
    s8_l credits;
};
struct me_traffic_ind_req
{
    u8_l sta_idx;
    u8_l tx_avail;
    bool_l uapsd;
};
struct me_rc_stats_req
{
    u8_l sta_idx;
};
struct rc_rate_stats
{
    u32_l attempts;
    u32_l success;
    u16_l probability;
    u16_l rate_config;
    u8_l sample_skipped;
    bool_l old_prob_available;
    u8_l n_retry;
    bool_l rate_allowed;
};
struct me_rc_stats_cfm
{
    u8_l sta_idx;
    u16_l no_samples;
    u16_l ampdu_len;
    u16_l ampdu_packets;
    u32_l avg_ampdu_len;
    u8_l sw_retry_step;
    u8_l sample_wait;
    u16_l retry_step_idx[4];
    struct rc_rate_stats rate_stats[10];
    u32_l tp[10];
};
struct me_rc_set_rate_req
{
    u8_l sta_idx;
    u16_l fixed_rate_cfg;
};
struct me_assoc_req_insert_info_req
{
    u8_l info_dmalength;
    u32_l info_dmaaddr;
};
struct me_config_monitor_req
{
    u8_l band;
    u8_l type;
    u16_l prim20_freq;
    u16_l center1_freq;
    u16_l center2_freq;
    s8_l tx_power;
};
struct me_config_monitor_cfm
{
    u8_l chan_index;
};
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
struct me_ate_tools_op_req
{
    struct scan_chan_tag chan;
    u32_l center_freq1;
    u32_l center_freq2;
    u8_l ch_width;
    u8_l vif_idx;
    u8_l flag;
    u8_l mode;
    u8_l rate_idx;
    u8_l frame_bw;
    bool use_short_preamble;
    bool use_short_gi;
    u8_l ch_index;
    struct mac_addr sta_mac_addr;
    struct mac_addr vif_mac_addr;
};
struct me_ate_tools_op_cfm
{
    u8_l status;
    u8_l vif_idx;
    u8_l ch_idx;
    u8_l bcmc_idx;
};
#endif
enum sm_msg_tag
{
    SM_CONNECT_REQ = LMAC_FIRST_MSG(TASK_SM),
    SM_CONNECT_CFM,
    SM_CONNECT_IND,
    SM_DISCONNECT_REQ,
    SM_DISCONNECT_CFM,
    SM_DISCONNECT_IND,
    SM_EXTERNAL_AUTH_REQUIRED_IND,
    SM_EXTERNAL_AUTH_REQUIRED_RSP,
    SM_REPEATER_STATUS_IND,
    SM_MAX,
};
struct sm_repeater_status_ind
{
    uint8_t status;
    uint8_t vif_idx;
};
struct sm_connect_req
{
    struct mac_ssid ssid;
    struct mac_addr bssid;
    struct scan_chan_tag chan;
    u32_l flags;
    u16_l ctrl_port_ethertype;
    u16_l ie_len;
    u16_l listen_interval;
    bool_l dont_wait_bcmc;
    u8_l auth_type;
    u8_l uapsd_queues;
    u8_l vif_idx;
    u32_l ie_buf[64];
};
struct sm_connect_cfm
{
    u8_l status;
};
#define SM_ASSOC_IE_LEN 800
struct sm_connect_ind
{
    u16_l status_code;
    struct mac_addr bssid;
    bool_l roamed;
    u8_l vif_idx;
    u8_l ap_idx;
    u8_l ch_idx;
    bool_l qos;
    u8_l acm;
    u16_l assoc_req_ie_len;
    u16_l assoc_rsp_ie_len;
    u32_l assoc_ie_buf[SM_ASSOC_IE_LEN/4];
    u16_l aid;
    u8_l band;
    u16_l center_freq;
    u8_l width;
    u32_l center_freq1;
    u32_l center_freq2;
    u32_l ac_param[AC_MAX];
};
struct sm_disconnect_req
{
    u16_l reason_code;
    u8_l vif_idx;
};
struct sm_association_ind
{
    struct mac_addr me_mac_addr;
};
struct sm_disconnect_ind
{
    u16_l reason_code;
    u8_l vif_idx;
    bool_l ft_over_ds;
};
struct sm_external_auth_required_ind
{
    u8_l vif_idx;
    struct mac_ssid ssid;
    struct mac_addr bssid;
    u32_l akm;
};
struct sm_external_auth_required_rsp
{
    u8_l vif_idx;
    u16_l status;
};
enum apm_msg_tag
{
    APM_START_REQ = LMAC_FIRST_MSG(TASK_APM),
    APM_START_CFM,
    APM_STOP_REQ,
    APM_STOP_CFM,
    APM_START_CAC_REQ,
    APM_START_CAC_CFM,
    APM_STOP_CAC_REQ,
    APM_STOP_CAC_CFM,
    APM_SEND_DEASSOC_REQ,
    APM_SEND_DEAUTH_REQ,
    APM_MAX,
};
struct apm_start_req
{
    struct mac_rateset basic_rates;
    struct scan_chan_tag chan;
    u32_l center_freq1;
    u32_l center_freq2;
    u8_l ch_width;
    bool is_radar_channel;
    u32_l bcn_addr;
    u16_l bcn_len;
    u16_l tim_oft;
    u16_l bcn_int;
    u32_l flags;
    u16_l ctrl_port_ethertype;
    u8_l tim_len;
    u8_l vif_idx;
    u8_l recovering;
};
struct apm_start_cfm
{
    u8_l status;
    u8_l vif_idx;
    u8_l ch_idx;
    u8_l bcmc_idx;
};
struct apm_stop_req
{
    u8_l vif_idx;
};
struct apm_start_cac_req
{
    struct scan_chan_tag chan;
    u32_l center_freq1;
    u32_l center_freq2;
    u8_l ch_width;
    u8_l vif_idx;
};
struct apm_start_cac_cfm
{
    u8_l status;
    u8_l ch_idx;
};
struct apm_stop_cac_req
{
    u8_l vif_idx;
};
#define MESH_MESHID_MAX_LEN (32)
enum mesh_msg_tag
{
    MESH_START_REQ = LMAC_FIRST_MSG(TASK_MESH),
    MESH_START_CFM,
    MESH_STOP_REQ,
    MESH_STOP_CFM,
    MESH_UPDATE_REQ,
    MESH_UPDATE_CFM,
    MESH_PEER_INFO_REQ,
    MESH_PEER_INFO_CFM,
    MESH_PATH_CREATE_REQ,
    MESH_PATH_CREATE_CFM,
    MESH_PATH_UPDATE_REQ,
    MESH_PATH_UPDATE_CFM,
    MESH_PROXY_ADD_REQ,
    MESH_PEER_UPDATE_IND,
    MESH_PEER_UPDATE_NTF = MESH_PEER_UPDATE_IND,
    MESH_PATH_UPDATE_IND,
    MESH_PROXY_UPDATE_IND,
    MESH_MAX,
};
struct mesh_start_req
{
    struct mac_rateset basic_rates;
    struct scan_chan_tag chan;
    u32_l center_freq1;
    u32_l center_freq2;
    u8_l ch_width;
    u8_l dtim_period;
    u16_l bcn_int;
    u8_l vif_index;
    u8_l mesh_id_len;
    u8_l mesh_id[MESH_MESHID_MAX_LEN];
    u32_l ie_addr;
    u8_l ie_len;
    bool_l user_mpm;
    bool_l is_auth;
    u8_l auth_id;
};
struct mesh_start_cfm
{
    u8_l status;
    u8_l vif_idx;
    u8_l ch_idx;
    u8_l bcmc_idx;
};
struct mesh_stop_req
{
    u8_l vif_idx;
};
struct mesh_stop_cfm
{
    u8_l vif_idx;
    u8_l status;
};
enum mesh_update_flags_bit
{
    MESH_UPDATE_FLAGS_ROOT_MODE_BIT = 0,
    MESH_UPDATE_FLAGS_GATE_MODE_BIT,
    MESH_UPDATE_FLAGS_MESH_FWD_BIT,
    MESH_UPDATE_FLAGS_LOCAL_PSM_BIT,
};
struct mesh_update_req
{
    u8_l flags;
    u8_l vif_idx;
    u8_l root_mode;
    bool_l gate_announ;
    bool_l mesh_forward;
    u8_l local_ps_mode;
};
struct mesh_update_cfm
{
    u8_l status;
};
struct mesh_peer_info_req
{
    u8_l sta_idx;
};
struct mesh_peer_info_cfm
{
    u8_l status;
    u8_l sta_idx;
    u16_l local_link_id;
    u16_l peer_link_id;
    u8_l local_ps_mode;
    u8_l peer_ps_mode;
    u8_l non_peer_ps_mode;
    u8_l link_state;
};
struct mesh_path_create_req
{
    u8_l vif_idx;
    bool_l has_orig_addr;
    struct mac_addr tgt_mac_addr;
    struct mac_addr orig_mac_addr;
};
struct mesh_path_create_cfm
{
    u8_l status;
    u8_l vif_idx;
};
struct mesh_path_update_req
{
    bool_l delete;
    u8_l vif_idx;
    struct mac_addr tgt_mac_addr;
    struct mac_addr nhop_mac_addr;
};
struct mesh_path_update_cfm
{
    u8_l status;
    u8_l vif_idx;
};
struct mesh_proxy_add_req
{
    u8_l vif_idx;
    struct mac_addr ext_sta_addr;
};
struct mesh_proxy_update_ind
{
    bool_l delete;
    bool_l local;
    u8_l vif_idx;
    struct mac_addr ext_sta_addr;
    struct mac_addr proxy_mac_addr;
};
struct mesh_peer_update_ind
{
    bool_l estab;
    u8_l vif_idx;
    u8_l sta_idx;
    struct mac_addr peer_addr;
};
struct mesh_peer_update_ntf
{
    u8_l vif_idx;
    u8_l sta_idx;
    u8_l state;
};
struct mesh_path_update_ind
{
    bool_l delete;
    bool_l ext_sta;
    u8_l vif_idx;
    u8_l path_idx;
    struct mac_addr tgt_mac_addr;
    struct mac_addr ext_sta_mac_addr;
    u8_l nhop_sta_idx;
};
enum dbg_msg_tag
{
    DBG_MEM_READ_REQ = LMAC_FIRST_MSG(TASK_DBG),
    DBG_MEM_READ_CFM,
    DBG_MEM_WRITE_REQ,
    DBG_MEM_WRITE_CFM,
    DBG_SET_MOD_FILTER_REQ,
    DBG_SET_MOD_FILTER_CFM,
    DBG_SET_SEV_FILTER_REQ,
    DBG_SET_SEV_FILTER_CFM,
    DBG_ERROR_IND,
    DBG_GET_SYS_STAT_REQ,
    DBG_GET_SYS_STAT_CFM,
    DBG_GET_TX_STAT_REQ,
    DBG_GET_TX_STAT_CFM,
    DBG_GET_RX_STAT_REQ,
    DBG_GET_RX_STAT_CFM,
#ifdef NEW_SCHEDULE
    DBG_PRINT_TX_BURST_INFO_REQ,
    DBG_PRINT_TX_BURST_INFO_CFM,
#endif
#ifdef CONFIG_VDR_HW
    DBG_GET_VDR_INFO_REQ,
    DBG_GET_VDR_INFO_CFM,
#endif
    DBG_VDR_GET_MP_INFO_REQ,
    DBG_VDR_GET_MP_INFO_CFM,
    DBG_GET_MGMT_INFO_REQ,
    DBG_GET_MGMT_INFO_CFM,
    DBG_GET_CTRL_INFO_REQ,
    DBG_GET_CTRL_INFO_CFM,
    DBG_SET_CCA_PARAMETER_REQ,
#ifdef CONFIG_ENABLE_RFGAINTABLE
    DBG_SET_RF_GAIN_TB_IDX_REQ,
#endif
    DBG_MAX,
};
struct dbg_mem_read_req
{
    u32_l memaddr;
};
struct dbg_mem_read_cfm
{
    u32_l memaddr;
    u32_l memdata;
};
struct dbg_mem_write_req
{
    u32_l memaddr;
    u32_l memdata;
};
struct dbg_mem_write_cfm
{
    u32_l memaddr;
    u32_l memdata;
};
struct dbg_set_mod_filter_req
{
    u32_l mod_filter;
};
struct dbg_set_sev_filter_req
{
    u32_l sev_filter;
};
struct dbg_get_sys_stat_cfm
{
    u32_l cpu_sleep_time;
    u32_l doze_time;
    u32_l stats_time;
};
struct dbg_get_tx_stat_req
{
    u32_l clear;
    u32_l cfm_dma_addr;
};
struct dbg_tx_stat
{
    u32_l s_mpdu_tx[AC_MAX];
    u32_l s_mpdu_successful[AC_MAX];
    u32_l a_mpdu_count[AC_MAX][64];
    u32_l a_mpdu_successful[AC_MAX][64];
    u32_l agg_finish_reason[8];
 u32_l pkt_cnt;
 u32_l transmitting_list_cnt[AC_MAX];
 u32_l cfm_list_cnt[AC_MAX];
 u32_l transmitting_list_add_cnt[AC_MAX];
 u32_l cfmlist_add_cnt[AC_MAX];
 u32_l discard_cnt[AC_MAX];
 u32_l total_transmitting_list_add_cnt[AC_MAX];
 u32_l total_cfmlist_add_cnt[AC_MAX];
 u32_l total_discard_cnt[AC_MAX];
 u32_l total_flush_cnt[AC_MAX];
 u32_l record_cnt[32];
    u32_l bcn_failed_cnt;
};
struct dbg_get_tx_stat_cfm
{
    u32_l dma_addr;
};
struct dbg_get_rx_stat_req
{
    u32_l clear;
    u32_l cfm_dma_addr;
};
struct dbg_rx_stat
{
    u32_l hostbuf_not_available;
    u32_l total;
    u32_l mib;
    u32_l upload;
    u32_l amsdu;
    u32_l key_invalid_spurious_data;
    u32_l key_invalid_duplicate_frame;
    u32_l key_invalid_spurious_mgmt;
    u32_l key_invalid_mgmt_handled;
    u32_l key_invalid_mgmt_upload;
    u32_l sta_nonregister;
    u32_l encrypted_fail;
    u32_l ctrl_frame;
    u32_l mgt_frame;
    u32_l data_frame;
    u32_l data_frame_reord_discard;
    u32_l data_frame_reord_duplicate_discard;
    u32_l data_frame_reord_pn_discard;
    u32_l data_frame_own_sa_discard;
    u32_l data_frame_mpdu1_discard;
    u32_l data_frame_duplicate_discard;
    u32_l data_frame_pn_failed_discard;
    u32_l defrag_frame;
    u32_l rx_pddesc_used_idx;
    u32_l rx_pddesc_free_idx;
    u32_l max_msdu_idx;
    u32_l max_dma_idx;
    u32_l max_used_pddesc;
    u32_l cve_2020_26140;
    u32_l cve_2020_24588;
    u32_l cve_2020_24587;
    u32_l cve_2020_26146;
    uint32_t jumbo_frame;
    u32_l data_frame_pn_failed_upload;
    uint32_t reord_alloc_cnt;
    uint32_t reord_release_cnt;
    uint32_t reord_hostid_nozero_cnt;
};
struct dbg_get_rx_stat_cfm
{
    u32_l dma_addr;
};
struct dbg_get_mgmt_info_req
{
    uint32_t clear;
    uint32_t cfm_dma_addr;
};
struct dbg_get_mgmt_info_cfm
{
    uint32_t dma_addr;
};
struct dbg_mgmt_info {
    uint32_t assocreq_tx_packets;
    uint32_t assocrsp_tx_packets;
    uint32_t probereq_tx_packets;
    uint32_t probersp_tx_packets;
    uint64_t beacon_tx_packets;
    uint32_t authent_tx_packets;
    uint32_t deauthent_tx_packets;
    uint32_t disassoc_tx_packets;
    uint32_t action_tx_packets;
    uint32_t assocreq_rx_packets;
    uint32_t assocrsp_rx_packets;
    uint32_t probereq_rx_packets;
    uint32_t probersp_rx_packets;
    uint64_t beacon_rx_packets;
    uint32_t authent_rx_packets;
    uint32_t deauthent_rx_packets;
    uint32_t disassoc_rx_packets;
    uint32_t action_rx_packets;
};
struct dbg_get_ctrl_info_req
{
    uint32_t clear;
    uint32_t cfm_dma_addr;
};
struct dbg_get_ctrl_info_cfm
{
    uint32_t dma_addr;
};
struct dbg_ctrl_info {
    uint32_t ba_tx_packets;
    uint32_t bar_tx_packets;
    uint32_t pspoll_tx_packets;
    uint32_t rts_tx_packets;
    uint32_t cts_tx_packets;
    uint32_t ack_tx_packets;
    uint32_t cfend_tx_packets;
    uint32_t ba_rx_packets;
    uint32_t bar_rx_packets;
    uint32_t pspoll_rx_packets;
    uint32_t rts_rx_packets;
    uint32_t cts_rx_packets;
    uint32_t ack_rx_packets;
    uint32_t cfend_rx_packets;
};
struct dbg_set_cca_parameter_req
{
    uint8_t set_cca_parameter_enable;
    uint32_t bk_timeout;
    uint32_t be_timeout;
    uint32_t vi_timeout;
    uint32_t vo_timeout;
    uint32_t bcn_timeout;
    uint8_t cca_threshold_rise;
    uint8_t cca_threshold_fall;
    uint8_t trigger_interval_low;
    uint8_t trigger_interval_high;
    uint8_t step_max;
    uint8_t step_distance;
    uint8_t default_cca_threshold_rise;
    uint8_t default_cca_threshold_fall;
};
#ifdef CONFIG_ENABLE_RFGAINTABLE
struct dbg_set_rf_gain_tb_idx_req
{
    uint8_t tb_idx[4];
};
#endif
#ifdef NEW_SCHEDULE
struct dbg_print_burst_info_req
{
    int32_t ac;
};
struct dbg_print_burst_info_cfm
{
    uint32_t reserved;
};
#endif
#ifdef CONFIG_VDR_HW
struct dbg_get_vendor_info_req
{
    uint32_t clear;
    uint8_t vif_idx;
    uint8_t sta_idx;
};
struct dbg_get_vendor_info_cfm
{
    int32_t value;
    uint8_t ampdu_num;
    uint8_t amsdu_on;
    uint8_t rts_rate;
    uint8_t rts_retry_cnt;
    uint16_t tx_rate;
    uint16_t tx_prev_rate;
    uint16_t min_tx_rate;
    uint16_t max_tx_rate;
    uint32_t tx_retry_cnt;
    uint32_t rx_seq_jump_cnt;
};
#endif
struct dbg_get_vendor_mp_info_req
{
    uint32_t params;
};
struct dbg_get_vendor_mp_info_cfm
{
    uint32_t cca_busy_time_us;
    uint32_t cca20_busy_time_us;
    uint32_t cca40_busy_time_us;
    uint32_t rx_frame_time_us;
    uint32_t tx_frame_time_us;
    uint32_t last_cal_time_us;
    int8_t last_noise0_dbm;
    int8_t last_noise1_dbm;
    uint32_t cpu_idle_time_us;
};
struct apm_send_deassoc_req{
    uint8_t vif_idx;
    uint8_t macaddr[6];
    uint16_t reason_code;
};
struct apm_send_deauth_req{
    uint8_t vif_idx;
    uint8_t macaddr[6];
    uint16_t reason_code;
};
struct mm_get_channel_info_req {
    uint32_t params;
};
struct mm_get_channel_info_cfm {
    uint8_t channel_idx;
    uint8_t channel_width;
    uint16_t center_freq1;
    uint16_t center_freq2;
    uint32_t channel_freq;
};
struct dbg_error_ind
{
    uint8_t dump_en;
    uint32_t error_type;
};
enum tdls_msg_tag
{
    TDLS_CHAN_SWITCH_REQ = LMAC_FIRST_MSG(TASK_TDLS),
    TDLS_CHAN_SWITCH_CFM,
    TDLS_CHAN_SWITCH_IND,
    TDLS_CHAN_SWITCH_BASE_IND,
    TDLS_CANCEL_CHAN_SWITCH_REQ,
    TDLS_CANCEL_CHAN_SWITCH_CFM,
    TDLS_PEER_PS_IND,
    TDLS_PEER_TRAFFIC_IND_REQ,
    TDLS_PEER_TRAFFIC_IND_CFM,
    TDLS_MAX
};
struct tdls_chan_switch_req
{
    u8_l vif_index;
    u8_l sta_idx;
    struct mac_addr peer_mac_addr;
    bool_l initiator;
    u8_l band;
    u8_l type;
    u16_l prim20_freq;
    u16_l center1_freq;
    u16_l center2_freq;
    s8_l tx_power;
    u8_l op_class;
};
struct tdls_cancel_chan_switch_req
{
    u8_l vif_index;
    u8_l sta_idx;
    struct mac_addr peer_mac_addr;
};
struct tdls_chan_switch_cfm
{
    u8_l status;
};
struct tdls_cancel_chan_switch_cfm
{
    u8_l status;
};
struct tdls_chan_switch_ind
{
    u8_l vif_index;
    u8_l chan_ctxt_index;
    u8_l status;
};
struct tdls_chan_switch_base_ind
{
    u8_l vif_index;
    u8_l chan_ctxt_index;
};
struct tdls_peer_ps_ind
{
    u8_l vif_index;
    u8_l sta_idx;
    struct mac_addr peer_mac_addr;
    bool ps_on;
};
struct tdls_peer_traffic_ind_req
{
    u8_l vif_index;
    u8_l sta_idx;
    struct mac_addr peer_mac_addr;
    u8_l dialog_token;
    u8_l last_tid;
    u16_l last_sn;
};
struct tdls_peer_traffic_ind_cfm
{
    u8_l status;
};
struct phy_radar_pulse
{
    uint32_t pulse;
};
#endif
