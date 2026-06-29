#ifndef _SIWIFI_TXQ_H_
#define _SIWIFI_TXQ_H_ 
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/ieee80211.h>
#define NX_NB_TID_PER_STA 8
#define NX_NB_TXQ_PER_STA (NX_NB_TID_PER_STA + 1)
#define NX_NB_TXQ_PER_VIF 2
#define NX_NB_TXQ ((NX_NB_TXQ_PER_STA * NX_REMOTE_STA_MAX) + \
                   (NX_NB_TXQ_PER_VIF * NX_VIRT_DEV_MAX) + 1)
#define NX_FIRST_VIF_TXQ_IDX (NX_REMOTE_STA_MAX * NX_NB_TXQ_PER_STA)
#define NX_FIRST_BCMC_TXQ_IDX NX_FIRST_VIF_TXQ_IDX
#define NX_FIRST_UNK_TXQ_IDX (NX_FIRST_BCMC_TXQ_IDX + NX_VIRT_DEV_MAX)
#define NX_OFF_CHAN_TXQ_IDX (NX_FIRST_VIF_TXQ_IDX + \
                             (NX_VIRT_DEV_MAX * NX_NB_TXQ_PER_VIF))
#define NX_BCMC_TXQ_TYPE 0
#define NX_UNK_TXQ_TYPE 1
#define NX_NB_NDEV_TXQ ((NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX) + 1 )
#define NX_BCMC_TXQ_NDEV_IDX (NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX)
#define NX_STA_NDEV_IDX(tid,sta_idx) ((tid) + (sta_idx) * NX_NB_TID_PER_STA)
#define NDEV_NO_TXQ 0xffff
#if (NX_NB_NDEV_TXQ >= NDEV_NO_TXQ)
#error("Need to increase struct siwifi_txq->ndev_idx size")
#endif
#define SIWIFI_NDEV_FLOW_CTRL_STOP 512
#define SIWIFI_NDEV_FLOW_CTRL_RESTART 256
#define SIWIFI_NDEV_PS_ACTIVE_DROP ((SIWIFI_NDEV_FLOW_CTRL_STOP + SIWIFI_NDEV_FLOW_CTRL_RESTART) / 2)
#define TXQ_INACTIVE 0xffff
#if (NX_NB_TXQ >= TXQ_INACTIVE)
#error("Need to increase struct siwifi_txq->idx size")
#endif
#define NX_TXQ_INITIAL_CREDITS 4
#define MAX_NUM_HWQ_TOKENS 4
#define MAX_SKBS_PER_TOKEN 64
extern const int nx_tid_prio[NX_NB_TID_PER_STA];
#define MAX_EARLY_SKB_LEN 160
#define NUM_TX_DESCS_PER_AC 2
#define NUM_PER_HWQ_RECORD 320
struct siwifi_hwq {
    struct list_head list;
    s16 credits[CONFIG_USER_MAX];
    u8 size;
    u8 id;
    bool need_processing;
#ifdef TOKEN_ENABLE
 u8 outstanding_tokens;
 u8 token_status[NUM_TX_DESCS_PER_AC];
 u64 ampdu_num[32];
 u8 record_txq[NUM_PER_HWQ_RECORD];
 u8 record_ampdu_num[NUM_PER_HWQ_RECORD];
 u16 current_record_num;
#ifdef CONFIG_BRIDGE_ACCELERATE
    u8 record_accel_txq_ready[NUM_PER_HWQ_RECORD];
#endif
    u8 record_txq_ready[NUM_PER_HWQ_RECORD];
    u8 record_hwq_credit[NUM_PER_HWQ_RECORD];
    u8 record_txq_credit[NUM_PER_HWQ_RECORD];
#endif
 u32 cfm_cnt;
 u32 push_cnt;
#ifdef NEW_SCHEDULE
    u8 max_skb_per_desc[CONFIG_USER_MAX];
#endif
};
enum siwifi_push_flags {
    SIWIFI_PUSH_RETRY = BIT(0),
    SIWIFI_PUSH_IMMEDIATE = BIT(1),
};
enum siwifi_txq_flags {
    SIWIFI_TXQ_IN_HWQ_LIST = BIT(0),
    SIWIFI_TXQ_STOP_FULL = BIT(1),
    SIWIFI_TXQ_STOP_CSA = BIT(2),
    SIWIFI_TXQ_STOP_STA_PS = BIT(3),
    SIWIFI_TXQ_STOP_VIF_PS = BIT(4),
    SIWIFI_TXQ_STOP_CHAN = BIT(5),
    SIWIFI_TXQ_STOP_MU_POS = BIT(6),
#ifdef CONFIG_SIWIFI_RF_RECALI
    SIWIFI_TXQ_STOP_RECALI = BIT(7),
#endif
    SIWIFI_TXQ_STOP = (SIWIFI_TXQ_STOP_FULL | SIWIFI_TXQ_STOP_CSA |
                             SIWIFI_TXQ_STOP_STA_PS | SIWIFI_TXQ_STOP_VIF_PS |
                             SIWIFI_TXQ_STOP_CHAN
#ifdef CONFIG_SIWIFI_RF_RECALI
                            | SIWIFI_TXQ_STOP_RECALI
#endif
                            ) ,
    SIWIFI_TXQ_NDEV_FLOW_CTRL = BIT(8),
    SIWIFI_TXQ_MEMORY_CTRL = BIT(9),
    SIWIFI_TXQ_IN_STUCK_CHECK_LIST = BIT(10),
};
#define SIWIFI_TXQ_STOP_POS_VIF_STOP 1
#define SIWIFI_TXQ_STOP_POS_STATION_STOP 2
#define SIWIFI_TXQ_STOP_POS_DATA_CFM 3
#define SIWIFI_TXQ_STOP_POS_CREDIT_UPDATE 4
#define SIWIFI_TXQ_STOP_POS_PUSH_FULL 5
#define SIWIFI_TXQ_STOP_POS_IOCTL 6
#define SIWIFI_TXQ_STOP_POS_SELECT_USER 7
struct txq_time_stat {
    u32 inlmac_total;
    u32 inlmac_retry;
    u32 inlmac_100ms;
    u32 inlmac_50ms;
    u32 inlmac_20ms;
    u32 inlmac_10ms;
    u32 inlmac_0ms;
    u32 come_xmit;
    u32 free_xmit;
};
struct txq_atf {
    uint8_t enable;
    uint32_t rateinfo;
    uint32_t record_rateinfo;
    uint32_t addup_rateinfo;
    uint8_t have_sent;
    uint32_t debug_get_token_cnt;
    uint32_t debug_skip_token_cnt;
    uint32_t debug_get_token_cnt_last;
    uint32_t debug_skip_token_cnt_last;
    uint32_t debug_get_token_cnt_record;
    uint32_t debug_skip_token_cnt_record;
};
struct siwifi_txq {
    u16 idx;
    u16 status;
    s16 credits;
    u8 pkt_sent;
    u8 pkt_pushed[CONFIG_USER_MAX];
    uint32_t pkt_send_total;
    uint32_t pkt_send_success;
    struct list_head sched_list;
    struct sk_buff_head sk_list;
    struct sk_buff *last_retry_skb;
    struct siwifi_hwq *hwq;
    int nb_retry;
    u8 push_limit;
#ifdef CONFIG_MAC80211_TXQ
    unsigned long nb_ready_mac80211;
#endif
    struct siwifi_sta *sta;
    u8 ps_id;
    u32 init_time;
    u16 ndev_idx;
    struct net_device *ndev;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
    struct siwifi_sw_txhdr *amsdu;
    u16 amsdu_len;
    u8 amsdu_maxnb;
#endif
 int token_pkt_num[NX_TXQ_CNT][NUM_TX_DESCS_PER_AC];
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    u8 mumimo_info;
#endif
#ifdef CONFIG_BRIDGE_ACCELERATE
    struct sk_buff_head accel_sk_list;
#endif
    u32 ps_on_drop;
    u32 ps_active_drop;
    u32 ps_off_drop;
    int stop_num;
    int wake_num;
    u16 last_stop_pos;
    struct siwifi_hw *siwifi_hw;
    u64 check_stuck_start;
    u32 stuck_time;
    s16 memctl_balance;
    struct txq_time_stat time_stat;
    struct txq_time_stat record_time_stat;
    struct txq_time_stat last_timer_time_stat;
    struct txq_atf atf;
};
struct siwifi_sta;
struct siwifi_vif;
struct siwifi_hw;
struct siwifi_sw_txhdr;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
#define SIWIFI_TXQ_GROUP_ID(txq) ((txq)->mumimo_info & 0x3f)
#define SIWIFI_TXQ_POS_ID(txq) (((txq)->mumimo_info >> 6) & 0x3)
#else
#define SIWIFI_TXQ_GROUP_ID(txq) 0
#define SIWIFI_TXQ_POS_ID(txq) 0
#endif
static inline bool siwifi_txq_is_stopped(struct siwifi_txq *txq)
{
    return (txq->status & SIWIFI_TXQ_STOP);
}
static inline bool siwifi_txq_is_full(struct siwifi_txq *txq)
{
    return (txq->status & SIWIFI_TXQ_STOP_FULL);
}
static inline bool siwifi_txq_is_scheduled(struct siwifi_txq *txq)
{
    return (txq->status & SIWIFI_TXQ_IN_HWQ_LIST);
}
static inline bool siwifi_txq_has_data(struct siwifi_txq *txq)
{
    return (skb_queue_len(&txq->sk_list)
#ifdef CONFIG_BRIDGE_ACCELERATE
            || skb_queue_len(&txq->accel_sk_list)
#endif
            );
}
#ifdef CONFIG_BRIDGE_ACCELERATE
static inline bool siwfi_txq_accel_is_full(struct siwifi_txq *txq)
{
    return (skb_queue_len(&txq->accel_sk_list) > SIWIFI_NDEV_FLOW_CTRL_STOP);
}
#endif
static inline bool siwifi_txq_is_ready_for_push(struct siwifi_txq *txq)
{
    return (!siwifi_txq_is_stopped(txq) &&
            txq->hwq->credits[SIWIFI_TXQ_POS_ID(txq)] > 0 &&
            skb_queue_empty(&txq->sk_list));
}
#ifdef CONFIG_MAC80211_TXQ
#define foreach_sta_txq(sta,txq,tid,siwifi_hw) \
    for (tid = 0, txq = siwifi_txq_sta_get(sta, 0); \
         tid < NX_NB_TXQ_PER_STA; \
         tid++, txq = siwifi_txq_sta_get(sta, tid))
#else
#define foreach_sta_txq(sta,txq,tid,siwifi_hw) \
    for (tid = 0, txq = siwifi_txq_sta_get(sta, 0, siwifi_hw); \
         tid < (is_multicast_sta(sta->sta_idx) ? 1 : NX_NB_TXQ_PER_STA); \
         tid++, txq++)
#endif
#if 0
#define foreach_sta_txq_prio(sta,txq,tid,i,siwifi_hw) \
    for (i = 0, tid = nx_tid_prio[0], txq = siwifi_txq_sta_get(sta, tid, siwifi_hw); \
         i < NX_NB_TID_PER_STA; \
         i++, tid = nx_tid_prio[i], txq = siwifi_txq_sta_get(sta, tid, siwifi_hw))
#endif
#ifdef CONFIG_MAC80211_TXQ
#define foreach_vif_txq(vif,txq,ac) \
    for (ac = SIWIFI_HWQ_BK, txq = siwifi_txq_vif_get(vif, ac); \
         ac < NX_NB_TXQ_PER_VIF; \
         ac++, txq = siwifi_txq_vif_get(vif, ac))
#else
#define foreach_vif_txq(vif,txq,ac) \
    for (ac = SIWIFI_HWQ_BK, txq = &vif->txqs[0]; \
         ac < NX_NB_TXQ_PER_VIF; \
         ac++, txq++)
#endif
struct siwifi_txq *siwifi_txq_sta_get(struct siwifi_sta *sta, u8 tid,
                                  struct siwifi_hw * siwifi_hw);
struct siwifi_txq *siwifi_txq_vif_get(struct siwifi_vif *vif, u8 type);
static inline u8 siwifi_txq_vif_get_status(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_txq *txq = siwifi_txq_vif_get(siwifi_vif, 0);
    return (txq->status & (SIWIFI_TXQ_STOP_CHAN | SIWIFI_TXQ_STOP_VIF_PS));
}
void siwifi_txq_vif_init(struct siwifi_hw * siwifi_hw, struct siwifi_vif *vif,
                       u8 status);
void siwifi_txq_vif_deinit(struct siwifi_hw * siwifi_hw, struct siwifi_vif *vif);
void siwifi_txq_sta_init(struct siwifi_hw * siwifi_hw, struct siwifi_sta *siwifi_sta,
                       u8 status);
void siwifi_txq_sta_deinit(struct siwifi_hw * siwifi_hw, struct siwifi_sta *siwifi_sta);
void siwifi_txq_unk_vif_init(struct siwifi_vif *siwifi_vif);
void siwifi_txq_unk_vif_deinit(struct siwifi_vif *vif);
void siwifi_txq_offchan_init(struct siwifi_vif *siwifi_vif);
void siwifi_txq_offchan_deinit(struct siwifi_vif *siwifi_vif);
void siwifi_txq_tdls_vif_init(struct siwifi_vif *siwifi_vif);
void siwifi_txq_tdls_vif_deinit(struct siwifi_vif *vif);
void siwifi_txq_tdls_sta_start(struct siwifi_vif *siwifi_vif, u16 reason,
                             struct siwifi_hw *siwifi_hw);
void siwifi_txq_tdls_sta_stop(struct siwifi_vif *siwifi_vif, u16 reason,
                            struct siwifi_hw *siwifi_hw);
void siwifi_txq_ps_drop_skb(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq);
void siwifi_txq_add_to_hw_list(struct siwifi_txq *txq);
void siwifi_txq_del_from_hw_list(struct siwifi_txq *txq);
void siwifi_txq_add_to_stuck_check_list(struct siwifi_txq *txq);
void siwifi_txq_del_from_stuck_check_list(struct siwifi_txq *txq);
void siwifi_txq_stop(struct siwifi_txq *txq, u16 reason, u16 pos);
void siwifi_txq_start(struct siwifi_txq *txq, u16 reason);
void siwifi_txq_vif_start(struct siwifi_vif *vif, u16 reason,
                        struct siwifi_hw *siwifi_hw);
void siwifi_txq_vif_stop(struct siwifi_vif *vif, u16 reason,
                       struct siwifi_hw *siwifi_hw);
void siwifi_txq_sta_start(struct siwifi_sta *sta, u16 reason,
                        struct siwifi_hw *siwifi_hw);
void siwifi_txq_sta_stop(struct siwifi_sta *sta, u16 reason,
                       struct siwifi_hw *siwifi_hw);
void siwifi_txq_offchan_start(struct siwifi_hw *siwifi_hw);
void siwifi_txq_sta_switch_vif(struct siwifi_sta *sta, struct siwifi_vif *old_vif,
        struct siwifi_vif *new_vif);
void siwifi_txq_flush(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq);
int siwifi_txq_queue_skb(struct sk_buff *skb, struct siwifi_txq *txq,
                       struct siwifi_hw *siwifi_hw, bool retry);
void siwifi_txq_confirm_any(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq,
                          struct siwifi_hwq *hwq, struct siwifi_sw_txhdr *sw_txhdr, int release_hwq);
void siwifi_hwq_init(struct siwifi_hw *siwifi_hw);
void siwifi_hwq_process(struct siwifi_hw *siwifi_hw, struct siwifi_hwq *hwq);
void siwifi_hwq_process_all(struct siwifi_hw *siwifi_hw);
int siwifi_adjust_hwq_credits(struct siwifi_hw *siwifi_hw,s16 credit_dec);
#endif
