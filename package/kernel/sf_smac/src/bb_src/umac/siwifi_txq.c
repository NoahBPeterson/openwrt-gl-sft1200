#include <net/mac80211.h>
#include <linux/sched/clock.h>
#include "siwifi_defs.h"
#include "siwifi_trace.h"
#include "siwifi_tx.h"
#include "ipc_host.h"
#include "siwifi_events.h"
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
#include "siwifi_ioctl.h"
#endif
#include "siwifi_traffic.h"
#ifdef WORKAROUND_7577
#include "reg_access.h"
#define EDCA_AC_1_ADDR(band) (WIFI_BASE_ADDR(band) + 0x00080000 + 0x0204)
#define WORKAROUND_RM7577(a) \
{ \
    static unsigned int g_hw_be_cnt = 0; \
    static unsigned int g_txq_push_cnt = 0; \
    g_txq_push_cnt += a; \
    if (hwq->id == AC_BE) g_hw_be_cnt += a; \
    if (g_txq_push_cnt >= 1000) { \
        if (g_hw_be_cnt < 950) { \
            writel(0xa43, (volatile void *)EDCA_AC_1_ADDR); \
        } else { \
            writel(0x211, (volatile void *)EDCA_AC_1_ADDR); \
        } \
        g_txq_push_cnt = 0; \
        g_hw_be_cnt = 0; \
    } \
}
#endif
#define LM_TXM_RQ_RATIO 2/3
#define LM_TXC_RQ_RATIO 2/3
const int nx_tid_prio[NX_NB_TID_PER_STA] = {7, 6, 5, 4, 3, 0, 2, 1};
static inline int siwifi_txq_sta_idx(struct siwifi_sta *sta, u8 tid)
{
    if (is_multicast_sta(sta->sta_idx))
        return NX_FIRST_VIF_TXQ_IDX + sta->vif_idx;
    else
        return (sta->sta_idx * NX_NB_TXQ_PER_STA) + tid;
}
static inline int siwifi_txq_vif_idx(struct siwifi_vif *vif, u8 type)
{
    return NX_FIRST_VIF_TXQ_IDX + master_vif_idx(vif) + (type * NX_VIRT_DEV_MAX);
}
struct siwifi_txq *siwifi_txq_sta_get(struct siwifi_sta *sta, u8 tid,
                                  struct siwifi_hw * siwifi_hw)
{
    if (tid >= NX_NB_TXQ_PER_STA)
        tid = 0;
    return &siwifi_hw->txq[siwifi_txq_sta_idx(sta, tid)];
}
struct siwifi_txq *siwifi_txq_vif_get(struct siwifi_vif *vif, u8 type)
{
    if (type > NX_UNK_TXQ_TYPE)
        type = NX_BCMC_TXQ_TYPE;
    return &vif->siwifi_hw->txq[siwifi_txq_vif_idx(vif, type)];
}
static inline struct siwifi_sta *siwifi_txq_2_sta(struct siwifi_txq *txq)
{
    return txq->sta;
}
static void siwifi_txq_init(struct siwifi_txq *txq, int idx, u8 status,
                          struct siwifi_hwq *hwq,
                          struct siwifi_sta *sta, struct net_device *ndev,
                          struct siwifi_hw *siwifi_hw
                          )
{
    int i;
 int j;
    txq->idx = idx;
    txq->status = status;
    txq->credits = NX_TXQ_INITIAL_CREDITS;
    txq->pkt_sent = 0;
    txq->pkt_send_total = 0;
    txq->pkt_send_success = 0;
    skb_queue_head_init(&txq->sk_list);
#ifdef CONFIG_BRIDGE_ACCELERATE
    skb_queue_head_init(&txq->accel_sk_list);
#endif
    txq->last_retry_skb = NULL;
    txq->nb_retry = 0;
    txq->hwq = hwq;
    txq->sta = sta;
    for (i = 0; i < CONFIG_USER_MAX ; i++)
        txq->pkt_pushed[i] = 0;
    txq->push_limit = 0;
#ifdef CONFIG_MAC80211_TXQ
    txq->nb_ready_mac80211 = 0;
#endif
    txq->ps_id = LEGACY_PS_ID;
    if (idx < NX_FIRST_VIF_TXQ_IDX) {
        int sta_idx = sta->sta_idx;
        int tid = idx - (sta_idx * NX_NB_TXQ_PER_STA);
        if (tid < NX_NB_TID_PER_STA)
   txq->ndev_idx = txq->hwq->id % siwifi_hw->tx_queue_num;
        else if (siwifi_hw->tx_queue_num <= 1)
            txq->ndev_idx = SIWIFI_HWQ_BK;
        else if (siwifi_hw->tx_queue_num <= IEEE80211_NUM_ACS)
            txq->ndev_idx = SIWIFI_HWQ_BE;
        else
            txq->ndev_idx = SIWIFI_HWQ_BCMC;
    } else {
        if (siwifi_hw->tx_queue_num > IEEE80211_NUM_ACS) {
            txq->ndev_idx = SIWIFI_HWQ_BCMC;
        } else {
            txq->ndev_idx = txq->hwq->id % siwifi_hw->tx_queue_num;
        }
    }
    if (txq->hwq->id == SIWIFI_HWQ_BE)
        txq->atf.enable = 1;
    txq->ndev = ndev;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
    txq->amsdu = NULL;
    txq->amsdu_len = 0;
    txq->amsdu_maxnb = 0;
#endif
 for (j = 0;j < NX_TXQ_CNT;j ++)
 {
  for (i = 0;i < NUM_TX_DESCS_PER_AC;i ++)
   txq->token_pkt_num[j][i] = 0;
 }
    txq->stop_num = 0;
    txq->wake_num = 0;
    txq->init_time++;
    txq->ps_on_drop = 0;
    txq->ps_active_drop = 0;
    txq->ps_off_drop = 0;
    txq->siwifi_hw = siwifi_hw;
    txq->stuck_time = 0;
    txq->memctl_balance = 0;
}
#ifdef CONFIG_DEBUG_TXQ_STOP
int g_txq_record_idx = 0;
struct siwifi_txq_record g_txq_record[1024];
void siwifi_trace_txq_stop(uint8_t pos, struct siwifi_txq *txq, struct siwifi_vif *vif)
{
    g_txq_record[g_txq_record_idx].pos = pos;
    g_txq_record[g_txq_record_idx].txq = (uint32_t)(txq);
    g_txq_record[g_txq_record_idx].txqidx = txq->idx;
    g_txq_record[g_txq_record_idx].txqsize = skb_queue_len(&txq->sk_list);
    g_txq_record[g_txq_record_idx].txq_status = txq->status;
    g_txq_record[g_txq_record_idx].memctl_balance = txq->memctl_balance;
    memcpy(&g_txq_record[g_txq_record_idx].memctrl, &(vif->lm_ctl[txq->ndev_idx]), sizeof(struct siwifi_lite_mem_ctrl));
    g_txq_record_idx = (g_txq_record_idx + 1) % 1024;
}
void siwifi_print_txq_trace(void)
{
    int index = g_txq_record_idx;
    int index_max = index + 1024;
    int pos = 0;
    while(index < index_max)
    {
        pos = index % 1024;
        printk("[%d] step %d txq %x idx %d status %4x cnt : %d, size :%d asmdu(cnt: %d size: %d) balance : %d txq size : %d\n",
                pos, g_txq_record[pos].pos, g_txq_record[pos].txq, g_txq_record[pos].txqidx,g_txq_record[pos].txq_status,
                g_txq_record[pos].memctrl.tx_cnt, g_txq_record[pos].memctrl.tx_memory_usage,
                g_txq_record[pos].memctrl.amsdu_tx_cnt, g_txq_record[pos].memctrl.amsdu_tx_memory_usage,
                g_txq_record[pos].memctl_balance,
                g_txq_record[pos].txqsize);
        index++;
    }
}
#define SIWIFI_TRACE_TXQ_STOP siwifi_trace_txq_stop
#else
#define SIWIFI_TRACE_TXQ_STOP(a...) do {} while (0)
#endif
static void siwifi_txq_free_skb(struct siwifi_hw *siwifi_hw, struct sk_buff *skb)
{
    struct siwifi_sw_txhdr *hdr = ((struct siwifi_txhdr *)skb->data)->sw_hdr;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
    if (hdr->desc.host.flags & TXU_CNTRL_AMSDU) {
        siwifi_free_amsdu(siwifi_hw, hdr);
    }
#endif
    dma_unmap_single(siwifi_hw->dev, hdr->dma_addr, hdr->map_len,
            DMA_TO_DEVICE);
    skb_pull(skb, hdr->headroom);
    siwifi_free_swtxhdr(siwifi_hw, hdr);
    dev_kfree_skb_any(skb);
}
void siwifi_txq_ps_drop_skb(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq)
{
    struct sk_buff *skb;
    struct siwifi_vif *vif;
#ifdef CONFIG_BRIDGE_ACCELERATE
    int accel_skb_len;
#endif
    int drop_num = 0;
#ifdef CONFIG_BRIDGE_ACCELERATE
    accel_skb_len = skb_queue_len(&txq->accel_sk_list);
#endif
    BUG_ON(txq->ndev == NULL);
    vif = netdev_priv(txq->ndev);
#ifdef CONFIG_BRIDGE_ACCELERATE
    if (accel_skb_len > 0) {
        while ((skb = skb_dequeue(&txq->accel_sk_list)) != NULL)
        {
            vif->lm_ctl[txq->ndev_idx].tx_memory_usage -= skb->truesize;
            vif->lm_ctl[txq->ndev_idx].tx_cnt --;
            siwifi_txq_free_skb(siwifi_hw, skb);
            drop_num++;
            if (drop_num >= (accel_skb_len / 2))
                break;
        }
        txq->ps_on_drop = drop_num;
    }
    drop_num = 0;
#endif
    while ((skb = skb_dequeue(&txq->sk_list)) != NULL)
    {
        vif->lm_ctl[txq->ndev_idx].tx_memory_usage -= skb->truesize;
        vif->lm_ctl[txq->ndev_idx].tx_cnt --;
        if (txq->nb_retry) {
            txq->nb_retry --;
        }
        siwifi_txq_free_skb(siwifi_hw, skb);
        drop_num ++;
        if (skb_queue_len(&txq->sk_list) <= SIWIFI_NDEV_FLOW_CTRL_RESTART)
            break;
    }
    if (txq->nb_retry == 0 ||
            !skb_queue_len(&txq->sk_list)) {
        txq->last_retry_skb = NULL;
    }
    WARN_ON(skb_queue_len(&txq->sk_list) > SIWIFI_NDEV_FLOW_CTRL_STOP);
    txq->ps_on_drop += drop_num;
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
    if (likely(!siwifi_hw->ate_env.tx_frame_start))
    {
#endif
        SIWIFI_TRACE_TXQ_STOP(1, txq, vif);
        if (txq->status & SIWIFI_TXQ_MEMORY_CTRL) {
            txq->memctl_balance --;
            txq->status &= ~SIWIFI_TXQ_MEMORY_CTRL;
        }
        txq->status &= ~SIWIFI_TXQ_NDEV_FLOW_CTRL;
        netif_wake_subqueue(txq->ndev, txq->ndev_idx);
        trace_txq_flowctrl_restart(txq);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->stats.queues_starts++;
#endif
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
    }
#endif
}
extern void siwifi_flush_txq_ref(struct siwifi_hw *siwifi_hw, uint32_t hwq_index, uint32_t txq, const int user_pos);
void siwifi_txq_flush(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq)
{
    struct sk_buff *skb;
    struct sk_buff_head free_list;
    struct siwifi_vif *vif;
 int skb_list_len = 0;
    if (siwifi_txq_has_data(txq)) {
        skb_list_len = skb_queue_len(&txq->sk_list);
        BUG_ON(txq->ndev == NULL);
        vif = netdev_priv(txq->ndev);
        skb_queue_head_init(&free_list);
        skb_queue_splice_init(&txq->sk_list, &free_list);
#ifdef CONFIG_BRIDGE_ACCELERATE
        skb_list_len += skb_queue_len(&txq->accel_sk_list);
        skb_queue_splice_tail_init(&txq->accel_sk_list, &free_list);
#endif
        while ((skb = skb_dequeue(&free_list)) != NULL) {
            vif->lm_ctl[txq->ndev_idx].tx_memory_usage -= skb->truesize;
            vif->lm_ctl[txq->ndev_idx].tx_cnt --;
            if (unlikely(txq->sta && txq->sta->ps.active))
                txq->sta->ps.pkt_ready[txq->ps_id]--;
            siwifi_txq_free_skb(siwifi_hw, skb);
        }
    }
    txq->last_retry_skb = NULL;
    txq->nb_retry = 0;
    if (((txq->status & (SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL)))
            == (SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL)) {
        vif = netdev_priv(txq->ndev);
        SIWIFI_TRACE_TXQ_STOP(2, txq, vif);
        txq->status &= ~(SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL);
        txq->memctl_balance --;
        netif_wake_subqueue(txq->ndev, txq->ndev_idx);
        trace_txq_flowctrl_restart(txq);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->stats.queues_starts++;
#endif
    }
    if (unlikely(txq->status & SIWIFI_TXQ_NDEV_FLOW_CTRL) &&
            skb_list_len > 0
            && !(txq->status & SIWIFI_TXQ_MEMORY_CTRL)
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
            && !siwifi_hw->ate_env.tx_frame_start
#endif
       ) {
        vif = netdev_priv(txq->ndev);
        SIWIFI_TRACE_TXQ_STOP(3, txq, vif);
        txq->status &= ~SIWIFI_TXQ_NDEV_FLOW_CTRL;
        trace_txq_flowctrl_restart(txq);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
#endif
    }
}
static void siwifi_txq_deinit(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq)
{
#ifdef TOKEN_ENABLE
 int token_id, hwq_id;
#endif
    if (txq->idx == TXQ_INACTIVE) {
        WARN_ON(siwifi_txq_has_data(txq));
        return;
    }
#ifdef TOKEN_ENABLE
 for (hwq_id = 0; hwq_id < NX_TXQ_CNT; hwq_id ++)
 {
  for (token_id = 0; token_id < NUM_TX_DESCS_PER_AC; token_id ++)
  {
   if (txq->token_pkt_num[hwq_id][token_id] > 0)
   {
    siwifi_hw->hwq[hwq_id].token_status[token_id] = 0;
    siwifi_hw->hwq[hwq_id].outstanding_tokens--;
   }
  }
 }
#endif
    siwifi_txq_flush(siwifi_hw, txq);
    siwifi_txq_del_from_hw_list(txq);
    siwifi_txq_del_from_stuck_check_list(txq);
    txq->idx = TXQ_INACTIVE;
}
void siwifi_txq_vif_init(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                       u8 status)
{
    struct siwifi_txq *txq;
    int idx;
    txq = siwifi_txq_vif_get(siwifi_vif, NX_BCMC_TXQ_TYPE);
    idx = siwifi_txq_vif_idx(siwifi_vif, NX_BCMC_TXQ_TYPE);
#if DEBUG_ARRAY_CHECK
    BUG_ON(siwifi_vif->ap.bcmc_index >= NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX);
#endif
    siwifi_txq_init(txq, idx, status, &siwifi_hw->hwq[SIWIFI_HWQ_BE],
                  &siwifi_hw->sta_table[siwifi_vif->ap.bcmc_index], siwifi_vif->ndev, siwifi_hw);
 txq->credits = 32;
    txq = siwifi_txq_vif_get(siwifi_vif, NX_UNK_TXQ_TYPE);
    idx = siwifi_txq_vif_idx(siwifi_vif, NX_UNK_TXQ_TYPE);
    siwifi_txq_init(txq, idx, status, &siwifi_hw->hwq[SIWIFI_HWQ_VO],
                  NULL, siwifi_vif->ndev, siwifi_hw);
}
void siwifi_txq_vif_deinit(struct siwifi_hw * siwifi_hw, struct siwifi_vif *siwifi_vif)
{
    struct siwifi_txq *txq;
    txq = siwifi_txq_vif_get(siwifi_vif, NX_BCMC_TXQ_TYPE);
    siwifi_txq_deinit(siwifi_hw, txq);
    txq = siwifi_txq_vif_get(siwifi_vif, NX_UNK_TXQ_TYPE);
    siwifi_txq_deinit(siwifi_hw, txq);
}
void siwifi_txq_sta_init(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta,
                       u8 status)
{
    struct siwifi_txq *txq;
    int tid, idx;
    struct siwifi_vif *siwifi_vif = siwifi_hw->vif_table[siwifi_sta->vif_idx];
    idx = siwifi_txq_sta_idx(siwifi_sta, 0);
#if DEBUG_ARRAY_CHECK
    BUG_ON(siwifi_sta->vif_idx >= NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX);
#endif
    foreach_sta_txq(siwifi_sta, txq, tid, siwifi_hw) {
        siwifi_txq_init(txq, idx, status, &siwifi_hw->hwq[siwifi_tid2hwq[tid]],
                      siwifi_sta, siwifi_vif->ndev, siwifi_hw);
        txq->ps_id = siwifi_sta->uapsd_tids & (1 << tid) ? UAPSD_ID : LEGACY_PS_ID;
        idx++;
    }
}
void siwifi_txq_sta_deinit(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta)
{
    struct siwifi_txq *txq;
    int tid;
    foreach_sta_txq(siwifi_sta, txq, tid, siwifi_hw) {
        siwifi_txq_deinit(siwifi_hw, txq);
    }
}
void siwifi_txq_unk_vif_init(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_hw *siwifi_hw = siwifi_vif->siwifi_hw;
    struct siwifi_txq *txq;
    int idx;
    txq = siwifi_txq_vif_get(siwifi_vif, NX_UNK_TXQ_TYPE);
    idx = siwifi_txq_vif_idx(siwifi_vif, NX_UNK_TXQ_TYPE);
    siwifi_txq_init(txq, idx, 0, &siwifi_hw->hwq[SIWIFI_HWQ_VO], NULL, siwifi_vif->ndev, siwifi_hw);
}
void siwifi_txq_unk_vif_deinit(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_txq *txq;
    txq = siwifi_txq_vif_get(siwifi_vif, NX_UNK_TXQ_TYPE);
    siwifi_txq_deinit(siwifi_vif->siwifi_hw, txq);
}
void siwifi_txq_offchan_init(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_hw *siwifi_hw = siwifi_vif->siwifi_hw;
    struct siwifi_txq *txq;
    txq = &siwifi_hw->txq[NX_OFF_CHAN_TXQ_IDX];
    siwifi_txq_init(txq, NX_OFF_CHAN_TXQ_IDX, SIWIFI_TXQ_STOP_CHAN,
                  &siwifi_hw->hwq[SIWIFI_HWQ_VO], NULL, siwifi_vif->ndev, siwifi_hw);
}
void siwifi_txq_offchan_deinit(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_txq *txq;
    txq = &siwifi_vif->siwifi_hw->txq[NX_OFF_CHAN_TXQ_IDX];
    siwifi_txq_deinit(siwifi_vif->siwifi_hw, txq);
}
void siwifi_txq_tdls_vif_init(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_hw *siwifi_hw = siwifi_vif->siwifi_hw;
    if (!(siwifi_hw->wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS))
        return;
    siwifi_txq_unk_vif_init(siwifi_vif);
}
void siwifi_txq_tdls_vif_deinit(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_hw *siwifi_hw = siwifi_vif->siwifi_hw;
    if (!(siwifi_hw->wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS))
        return;
    siwifi_txq_unk_vif_deinit(siwifi_vif);
}
void siwifi_txq_add_to_stuck_check_list(struct siwifi_txq *txq)
{
    if (!(txq->status & SIWIFI_TXQ_IN_STUCK_CHECK_LIST))
    {
        WARN_ON(txq->status & SIWIFI_TXQ_IN_HWQ_LIST);
        list_add_tail(&txq->sched_list, &txq->siwifi_hw->stuck_check_list);
        txq->status |= SIWIFI_TXQ_IN_STUCK_CHECK_LIST;
        txq->check_stuck_start = local_clock();
    }
}
void siwifi_txq_del_from_stuck_check_list(struct siwifi_txq *txq)
{
    if (txq->status & SIWIFI_TXQ_IN_STUCK_CHECK_LIST) {
        list_del(&txq->sched_list);
        txq->status &= ~SIWIFI_TXQ_IN_STUCK_CHECK_LIST;
    }
}
void siwifi_txq_add_to_hw_list(struct siwifi_txq *txq)
{
    if (!(txq->status & SIWIFI_TXQ_IN_HWQ_LIST)) {
        trace_txq_add_to_hw(txq);
        siwifi_txq_del_from_stuck_check_list(txq);
        txq->status |= SIWIFI_TXQ_IN_HWQ_LIST;
        list_add_tail(&txq->sched_list, &txq->hwq->list);
        txq->hwq->need_processing = true;
    }
}
void siwifi_txq_del_from_hw_list(struct siwifi_txq *txq)
{
    if (txq->status & SIWIFI_TXQ_IN_HWQ_LIST) {
        trace_txq_del_from_hw(txq);
        txq->status &= ~SIWIFI_TXQ_IN_HWQ_LIST;
        list_del(&txq->sched_list);
        if (siwifi_txq_has_data(txq)) {
            siwifi_txq_add_to_stuck_check_list(txq);
        }
    }
}
static inline bool siwifi_txq_skb_ready(struct siwifi_txq *txq)
{
#ifdef CONFIG_MAC80211_TXQ
    if (txq->nb_ready_mac80211 != NOT_MAC80211_TXQ)
        return ((txq->nb_ready_mac80211 > 0) || !skb_queue_empty(&txq->sk_list));
    else
#endif
#ifdef CONFIG_BRIDGE_ACCELERATE
    return (!skb_queue_empty(&txq->sk_list) || (!skb_queue_empty(&txq->accel_sk_list)));
#else
    return !skb_queue_empty(&txq->sk_list);
#endif
}
void siwifi_txq_start(struct siwifi_txq *txq, u16 reason)
{
    BUG_ON(txq == NULL);
    if (txq->idx != TXQ_INACTIVE && (txq->status & reason))
    {
        trace_txq_start(txq, reason);
        txq->status &= ~reason;
        if (!siwifi_txq_is_stopped(txq) && siwifi_txq_skb_ready(txq))
            siwifi_txq_add_to_hw_list(txq);
    }
}
void siwifi_txq_stop(struct siwifi_txq *txq, u16 reason, u16 pos)
{
    BUG_ON( txq== NULL);
    if (txq->idx != TXQ_INACTIVE)
    {
        trace_txq_stop(txq, reason);
        txq->status |= reason;
        txq->last_stop_pos = pos;
        siwifi_txq_del_from_hw_list(txq);
    }
}
void siwifi_txq_sta_start(struct siwifi_sta *siwifi_sta, u16 reason, struct siwifi_hw *siwifi_hw)
{
    struct siwifi_txq *txq;
    int tid;
    trace_txq_sta_start(siwifi_sta->sta_idx);
    foreach_sta_txq(siwifi_sta, txq, tid, siwifi_hw) {
        siwifi_txq_start(txq, reason);
    }
}
void siwifi_txq_sta_stop(struct siwifi_sta *siwifi_sta, u16 reason, struct siwifi_hw *siwifi_hw)
{
    struct siwifi_txq *txq;
    int tid;
    if (!siwifi_sta)
        return;
    trace_txq_sta_stop(siwifi_sta->sta_idx);
    foreach_sta_txq(siwifi_sta, txq, tid, siwifi_hw) {
        siwifi_txq_stop(txq, reason, SIWIFI_TXQ_STOP_POS_STATION_STOP);
    }
}
void siwifi_txq_tdls_sta_start(struct siwifi_vif *siwifi_vif, u16 reason,
                             struct siwifi_hw *siwifi_hw)
{
    trace_txq_vif_start(siwifi_vif->vif_index);
    spin_lock_bh(&siwifi_hw->tx_lock);
    if (siwifi_vif->sta.tdls_sta)
        siwifi_txq_sta_start(siwifi_vif->sta.tdls_sta, reason, siwifi_hw);
    spin_unlock_bh(&siwifi_hw->tx_lock);
}
void siwifi_txq_tdls_sta_stop(struct siwifi_vif *siwifi_vif, u16 reason,
                            struct siwifi_hw *siwifi_hw)
{
    trace_txq_vif_stop(siwifi_vif->vif_index);
    spin_lock_bh(&siwifi_hw->tx_lock);
    if (siwifi_vif->sta.tdls_sta)
        siwifi_txq_sta_stop(siwifi_vif->sta.tdls_sta, reason, siwifi_hw);
    spin_unlock_bh(&siwifi_hw->tx_lock);
}
static inline
void siwifi_txq_vif_for_each_sta(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif,
                               void (*f)(struct siwifi_sta *, u16, struct siwifi_hw *),
                               u16 reason)
{
    switch (SIWIFI_VIF_TYPE(siwifi_vif)) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
    {
        if (siwifi_vif->tdls_status == TDLS_LINK_ACTIVE)
            f(siwifi_vif->sta.tdls_sta, reason, siwifi_hw);
  if (siwifi_vif->sta.ap)
            f(siwifi_vif->sta.ap, reason, siwifi_hw);
        break;
    }
    case NL80211_IFTYPE_AP_VLAN:
        siwifi_vif = siwifi_vif->ap_vlan.master;
        fallthrough;
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_MESH_POINT:
    case NL80211_IFTYPE_P2P_GO:
    {
        struct siwifi_sta *sta;
        list_for_each_entry(sta, &siwifi_vif->ap.sta_list, list) {
            f(sta, reason, siwifi_hw);
        }
        break;
    }
    default:
        BUG();
        break;
    }
}
void siwifi_txq_vif_start(struct siwifi_vif *siwifi_vif, u16 reason,
                        struct siwifi_hw *siwifi_hw)
{
    struct siwifi_txq *txq;
    trace_txq_vif_start(siwifi_vif->vif_index);
    spin_lock_bh(&siwifi_hw->tx_lock);
    if (siwifi_vif->wdev.iftype == NL80211_IFTYPE_MONITOR)
        goto end;
    if (siwifi_vif->roc_tdls && siwifi_vif->sta.tdls_sta && siwifi_vif->sta.tdls_sta->tdls.chsw_en) {
        siwifi_txq_sta_start(siwifi_vif->sta.tdls_sta, reason, siwifi_hw);
    }
    if (!siwifi_vif->roc_tdls) {
        siwifi_txq_vif_for_each_sta(siwifi_hw, siwifi_vif, siwifi_txq_sta_start, reason);
    }
    txq = siwifi_txq_vif_get(siwifi_vif, NX_BCMC_TXQ_TYPE);
    siwifi_txq_start(txq, reason);
    txq = siwifi_txq_vif_get(siwifi_vif, NX_UNK_TXQ_TYPE);
    siwifi_txq_start(txq, reason);
end:
    spin_unlock_bh(&siwifi_hw->tx_lock);
}
void siwifi_txq_vif_stop(struct siwifi_vif *siwifi_vif, u16 reason,
                       struct siwifi_hw *siwifi_hw)
{
    struct siwifi_txq *txq;
    trace_txq_vif_stop(siwifi_vif->vif_index);
    spin_lock_bh(&siwifi_hw->tx_lock);
    if (siwifi_vif->wdev.iftype == NL80211_IFTYPE_MONITOR)
        goto end;
    siwifi_txq_vif_for_each_sta(siwifi_hw, siwifi_vif, siwifi_txq_sta_stop, reason);
    txq = siwifi_txq_vif_get(siwifi_vif, NX_BCMC_TXQ_TYPE);
    siwifi_txq_stop(txq, reason, SIWIFI_TXQ_STOP_POS_VIF_STOP);
    txq = siwifi_txq_vif_get(siwifi_vif, NX_UNK_TXQ_TYPE);
    siwifi_txq_stop(txq, reason, SIWIFI_TXQ_STOP_POS_VIF_STOP);
end:
    spin_unlock_bh(&siwifi_hw->tx_lock);
}
void siwifi_txq_offchan_start(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_txq *txq;
    spin_lock_bh(&siwifi_hw->tx_lock);
    txq = &siwifi_hw->txq[NX_OFF_CHAN_TXQ_IDX];
    siwifi_txq_start(txq, SIWIFI_TXQ_STOP_CHAN);
    spin_unlock_bh(&siwifi_hw->tx_lock);
}
void siwifi_txq_sta_switch_vif(struct siwifi_sta *sta, struct siwifi_vif *old_vif,
                             struct siwifi_vif *new_vif)
{
    struct siwifi_hw *siwifi_hw = new_vif->siwifi_hw;
    struct siwifi_txq *txq;
    int i;
    if (!netif_carrier_ok(new_vif->ndev))
        netif_carrier_on(new_vif->ndev);
    txq = siwifi_txq_sta_get(sta, 0, siwifi_hw);
    for (i = 0; i < NX_NB_TID_PER_STA; i++, txq++) {
        txq->ndev = new_vif->ndev;
        netif_wake_subqueue(txq->ndev, txq->ndev_idx);
    }
}
static bool siwifi_txq_check_flow_ctrl(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq, bool retry)
{
    bool stop_ndev_queue = false;
    struct siwifi_vif *vif;
    BUG_ON(txq->ndev == NULL);
    vif = netdev_priv(txq->ndev);
    if (siwifi_hw->lm_ctrl_enable &&
            ((vif->lm_ctl[txq->ndev_idx].tx_cnt + vif->lm_ctl[txq->ndev_idx].amsdu_tx_cnt) > vif->lm_ctl[txq->ndev_idx].tx_cnt_limit ||
            (vif->lm_ctl[txq->ndev_idx].tx_memory_usage + vif->lm_ctl[txq->ndev_idx].amsdu_tx_memory_usage) > vif->lm_ctl[txq->ndev_idx].tx_memory_limit) &&
            !(txq->status & SIWIFI_TXQ_MEMORY_CTRL))
    {
        SIWIFI_TRACE_TXQ_STOP(4, txq, vif);
        txq->memctl_balance ++;
        txq->status |= SIWIFI_TXQ_MEMORY_CTRL;
        if (!(txq->status & SIWIFI_TXQ_NDEV_FLOW_CTRL)) {
            txq->status |= SIWIFI_TXQ_NDEV_FLOW_CTRL;
            netif_stop_subqueue(txq->ndev, txq->ndev_idx);
            trace_txq_flowctrl_stop(txq);
            stop_ndev_queue = true;
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
            siwifi_hw->stats.queues_stops++;
#endif
        }
    }
    BUG_ON(txq->ndev_idx >= siwifi_hw->tx_queue_num);
    if (skb_queue_len(&txq->sk_list) > SIWIFI_NDEV_FLOW_CTRL_STOP
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
            && !siwifi_hw->ate_env.tx_frame_start
#endif
         && !(txq->status & SIWIFI_TXQ_NDEV_FLOW_CTRL)
        ) {
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
#endif
        SIWIFI_TRACE_TXQ_STOP(5, txq, vif);
        txq->status |= SIWIFI_TXQ_NDEV_FLOW_CTRL;
        stop_ndev_queue = true;
    }
    return stop_ndev_queue;
}
int siwifi_txq_queue_skb(struct sk_buff *skb, struct siwifi_txq *txq,
                       struct siwifi_hw *siwifi_hw, bool retry)
{
    struct siwifi_vif *vif;
    if (unlikely(txq->sta && txq->sta->ps.active)) {
#if DEBUG_ARRAY_CHECK
        BUG_ON(txq->ps_id >= 2);
#endif
        if (txq->sta->ps.pkt_ready[txq->ps_id] > SIWIFI_NDEV_PS_ACTIVE_DROP)
        {
            struct siwifi_sw_txhdr *hdr = ((struct siwifi_txhdr *)skb->data)->sw_hdr;
            if (!hdr->mgmt_frame) {
                txq->ps_active_drop ++;
                siwifi_txq_free_skb(siwifi_hw, skb);
                goto out;
            }
        }
        txq->sta->ps.pkt_ready[txq->ps_id]++;
        trace_ps_queue(txq->sta);
        if (txq->sta->ps.pkt_ready[txq->ps_id] == 1) {
            siwifi_set_traffic_status(siwifi_hw, txq->sta, true, txq->ps_id);
        }
    }
    if (!retry) {
#ifdef CONFIG_BRIDGE_ACCELERATE
        if (siwifi_skb_is_acceled(skb))
            skb_queue_tail(&txq->accel_sk_list, skb);
        else
#endif
        {
            if (skb->cb[PING_CB_POSITION] == PING_CB_CODE) {
                if (txq->last_retry_skb)
                    skb_append(txq->last_retry_skb, skb, &txq->sk_list);
                else
                    skb_queue_head(&txq->sk_list, skb);
            }
            else
                skb_queue_tail(&txq->sk_list, skb);
        }
    } else {
        if (txq->last_retry_skb)
            skb_append(txq->last_retry_skb, skb, &txq->sk_list);
        else
            skb_queue_head(&txq->sk_list, skb);
        txq->last_retry_skb = skb;
        txq->nb_retry++;
    }
    trace_txq_queue_skb(skb, txq, retry);
    vif = netdev_priv(txq->ndev);
    vif->lm_ctl[txq->ndev_idx].tx_memory_usage += skb->truesize;
    vif->lm_ctl[txq->ndev_idx].tx_cnt ++;
    siwifi_txq_check_flow_ctrl(siwifi_hw, txq, retry);
out:
    if (!siwifi_txq_is_stopped(txq)) {
        siwifi_txq_add_to_hw_list(txq);
        return 1;
    } else if (siwifi_txq_has_data(txq) &&
            !(txq->status & SIWIFI_TXQ_IN_HWQ_LIST)) {
        siwifi_txq_add_to_stuck_check_list(txq);
    }
    return 0;
}
void siwifi_txq_confirm_any(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq,
                          struct siwifi_hwq *hwq, struct siwifi_sw_txhdr *sw_txhdr, int release_hwq)
{
    int user = 0;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    int group_id;
    user = SIWIFI_MUMIMO_INFO_POS_ID(sw_txhdr->desc.host.mumimo_info);
    group_id = SIWIFI_MUMIMO_INFO_GROUP_ID(sw_txhdr->desc.host.mumimo_info);
    if (txq && (txq->idx != TXQ_INACTIVE) &&
        (txq->pkt_pushed[user] == 1) &&
        (txq->status & SIWIFI_TXQ_STOP_MU_POS))
        siwifi_txq_start(txq, SIWIFI_TXQ_STOP_MU_POS);
#endif
    if (txq && txq->pkt_pushed[user])
        txq->pkt_pushed[user]--;
    if (release_hwq) {
     hwq->credits[user]++;
     hwq->need_processing = true;
        hwq->cfm_cnt ++;
    }
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    siwifi_hw->stats.cfm_balance[hwq->id]--;
#endif
}
static inline
bool siwifi_txq_take_mu_lock(struct siwifi_hw *siwifi_hw)
{
    bool res = false;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    if (siwifi_hw->mod_params->mutx)
        res = (down_trylock(&siwifi_hw->mu.lock) == 0);
#endif
    return res;
}
static inline
void siwifi_txq_release_mu_lock(struct siwifi_hw *siwifi_hw)
{
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    up(&siwifi_hw->mu.lock);
#endif
}
static inline
void siwifi_txq_set_mu_info(struct siwifi_hw *siwifi_hw, struct siwifi_txq *txq,
                          int group_id, int pos)
{
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    trace_txq_select_mu_group(txq, group_id, pos);
    if (group_id) {
        txq->mumimo_info = group_id | (pos << 6);
        siwifi_mu_set_active_group(siwifi_hw, group_id);
    } else
        txq->mumimo_info = 0;
#endif
}
static inline
s16 siwifi_txq_get_credits(struct siwifi_txq *txq)
{
    s16 cred = txq->credits;
    if (txq->push_limit && (cred > txq->push_limit)) {
        cred = txq->push_limit;
    }
    return cred;
}
static inline void skb_queue_extract(struct sk_buff_head *list,
                                     struct sk_buff_head *head, int nb_elt)
{
    int i;
    struct sk_buff *first, *last, *ptr;
    first = ptr = list->next;
    for (i = 0; i < nb_elt; i++) {
        ptr = ptr->next;
    }
    last = ptr->prev;
    list->qlen -= nb_elt;
    list->next = ptr;
    ptr->prev = (struct sk_buff *)list;
    head->qlen += nb_elt;
    last->next = (struct sk_buff *)head;
    head->prev->next = first;
    first->prev = head->prev;
    head->prev = last;
}
#ifdef CONFIG_MAC80211_TXQ
static bool siwifi_txq_mac80211_dequeue(struct siwifi_hw *siwifi_hw,
                                      struct sk_buff_head *sk_list,
                                      struct siwifi_txq *txq, int max)
{
    struct ieee80211_txq *mac_txq;
    struct sk_buff *skb;
    unsigned long mac_txq_len;
    if (txq->nb_ready_mac80211 == NOT_MAC80211_TXQ)
        return true;
    mac_txq = container_of((void *)txq, struct ieee80211_txq, drv_priv);
    for (; max > 0; max--) {
        skb = siwifi_tx_dequeue_prep(siwifi_hw, mac_txq);
        if (skb == NULL)
            return true;
        __skb_queue_tail(sk_list, skb);
    }
    ieee80211_txq_get_depth(mac_txq, &mac_txq_len, NULL);
    if (txq->nb_ready_mac80211 > mac_txq_len)
        trace_txq_drop(txq, txq->nb_ready_mac80211 - mac_txq_len);
    txq->nb_ready_mac80211 = mac_txq_len;
    return (txq->nb_ready_mac80211 == 0);
}
#endif
static
bool siwifi_txq_get_skb_to_push(struct siwifi_hw *siwifi_hw, struct siwifi_hwq *hwq,
                              struct siwifi_txq *txq, int user,
                              struct sk_buff_head *sk_list_push)
{
    bool res = false;
    int nb_ready = skb_queue_len(&txq->sk_list);
#ifdef CONFIG_BRIDGE_ACCELERATE
    int nb_accel = skb_queue_len(&txq->accel_sk_list);
#endif
#ifdef NEW_SCHEDULE
    int credits = hwq->max_skb_per_desc[user];
#else
    int credits = min_t(int, siwifi_txq_get_credits(txq), hwq->credits[user]);
#ifdef TOKEN_ENABLE
 credits = min_t(int, hwq->size / NUM_TX_DESCS_PER_AC, credits);
#endif
#endif
    __skb_queue_head_init(sk_list_push);
    if (credits <= 0) {
        if (nb_ready == 0
#ifdef CONFIG_BRIDGE_ACCELERATE
                && nb_accel == 0
#endif
                )
            return true;
        return false;
    }
#ifdef CONFIG_BRIDGE_ACCELERATE
    if (credits >= (nb_ready + nb_accel)) {
        skb_queue_splice_init(&txq->sk_list, sk_list_push);
        skb_queue_splice_tail_init(&txq->accel_sk_list, sk_list_push);
        res = true;
        credits = (nb_ready + nb_accel);
    } else {
        if (credits > nb_ready) {
            skb_queue_splice_init(&txq->sk_list, sk_list_push);
            skb_queue_extract(&txq->accel_sk_list, sk_list_push, credits - nb_ready);
        } else if (credits == nb_ready) {
            skb_queue_splice_init(&txq->sk_list, sk_list_push);
        } else {
            skb_queue_extract(&txq->sk_list, sk_list_push, credits);
        }
    }
    if (!res) {
        if (txq->push_limit && (credits == txq->push_limit))
            res = true;
    }
#else
    if (credits >= nb_ready) {
        skb_queue_splice_init(&txq->sk_list, sk_list_push);
#ifdef CONFIG_MAC80211_TXQ
        res = siwifi_txq_mac80211_dequeue(siwifi_hw, sk_list_push, txq, credits - nb_ready);
        credits = skb_queue_len(sk_list_push);
#else
        res = true;
        credits = nb_ready;
#endif
    } else {
        skb_queue_extract(&txq->sk_list, sk_list_push, credits);
        if (txq->push_limit && (credits == txq->push_limit))
            res = true;
    }
#endif
    siwifi_mu_set_active_sta(siwifi_hw, siwifi_txq_2_sta(txq), credits);
    return res;
}
static
bool siwifi_txq_select_user(struct siwifi_hw *siwifi_hw, bool mu_lock,
                          struct siwifi_txq *txq, struct siwifi_hwq *hwq, int *user)
{
    int pos = 0;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    int id, group_id = 0;
    struct siwifi_sta *sta = siwifi_txq_2_sta(txq);
    if (!sta || !sta->group_info.cnt)
        goto end;
    if (!siwifi_hw->mod_params->mutx_on || !mu_lock)
        goto check_user;
    group_id = sta->group_info.group;
    if (group_id > 0)
        pos = siwifi_mu_group_sta_get_pos(siwifi_hw, sta, group_id);
  check_user:
#if CONFIG_USER_MAX == 2
    id = (pos + 1) & 0x1;
    if (txq->pkt_pushed[id]) {
        siwifi_txq_stop(txq, SIWIFI_TXQ_STOP_MU_POS, SIWIFI_TXQ_STOP_POS_SELECT_USER);
        return false;
    }
#else
    for (id = 0 ; id < CONFIG_USER_MAX ; id++) {
        if (id != pos && txq->pkt_pushed[id]) {
            siwifi_txq_stop(txq, SIWIFI_TXQ_STOP_MU_POS, SIWIFI_TXQ_STOP_POS_SELECT_USER);
            return false;
        }
    }
#endif
  end:
    siwifi_txq_set_mu_info(siwifi_hw, txq, group_id, pos);
#endif
    *user = pos;
    return true;
}
#ifdef TOKEN_ENABLE
static int siwifi_hwq_get_token_id(struct siwifi_hw *siwifi_hw, struct siwifi_hwq *hwq, struct siwifi_txq *txq, int *token)
{
    int i;
    *token = -1;
    if (hwq->outstanding_tokens >= NUM_TX_DESCS_PER_AC)
        return -1;
    for (i = 0; i < NUM_TX_DESCS_PER_AC; i++)
    {
        if (hwq->token_status[i] == 0)
        {
            *token = i;
            break;
        }
    }
    if (*token == -1)
        return -1;
    if (txq->atf.record_rateinfo != 0 && txq->atf.enable && siwifi_hw->atf.enable) {
        txq->atf.addup_rateinfo += txq->atf.record_rateinfo;
        if (txq->atf.addup_rateinfo < siwifi_hw->atf.max_rateinfo){
            *token == -1;
            txq->atf.debug_skip_token_cnt++;
            return 1;
        } else {
            txq->atf.addup_rateinfo -= siwifi_hw->atf.max_rateinfo;
            txq->atf.debug_get_token_cnt++;
            if (txq->atf.have_sent == 0) {
                txq->atf.have_sent = 1;
                siwifi_hw->atf.txq_nosent_cnt--;
            }
        }
    }
    hwq->token_status[*token] = 1;
    hwq->outstanding_tokens++;
    txq->token_pkt_num[hwq->id][*token] = 0;
    return 0;
}
static void siwifi_txq_tokens_record(struct siwifi_hwq *hwq, struct siwifi_txq *txq, int token_id)
{
#if DEBUG_ARRAY_CHECK
    BUG_ON(hwq->id >= NX_TXQ_CNT);
    BUG_ON(token_id >= NUM_TX_DESCS_PER_AC);
    BUG_ON(txq->token_pkt_num[hwq->id][token_id] > 32);
    BUG_ON(hwq->current_record_num >= NUM_PER_HWQ_RECORD);
#endif
    hwq->ampdu_num[txq->token_pkt_num[hwq->id][token_id] - 1]++;
    hwq->record_txq[hwq->current_record_num] = txq->idx;
    hwq->record_txq_ready[hwq->current_record_num] = skb_queue_len(&txq->sk_list);
#ifdef CONFIG_BRIDGE_ACCELERATE
    hwq->record_accel_txq_ready[hwq->current_record_num] = skb_queue_len(&txq->accel_sk_list);
#endif
    hwq->record_txq_ready[hwq->current_record_num] = skb_queue_len(&txq->sk_list);
    hwq->record_ampdu_num[hwq->current_record_num] = txq->token_pkt_num[hwq->id][token_id];
    hwq->record_hwq_credit[hwq->current_record_num] = hwq->credits[0];
    hwq->record_txq_credit[hwq->current_record_num] = siwifi_txq_get_credits(txq);
    hwq->current_record_num++;
    if (hwq->current_record_num == NUM_PER_HWQ_RECORD)
        hwq->current_record_num = 0;
}
#endif
#define ALL_HWQ_MASK ((1 << CONFIG_USER_MAX) - 1)
void siwifi_hwq_process(struct siwifi_hw *siwifi_hw, struct siwifi_hwq *hwq)
{
    struct siwifi_txq *txq, *next;
    int user, credit_map = 0;
    bool mu_enable;
#ifndef NEW_SCHEDULE
    int push_success = 0;
    int pushed_total = 0;
#endif
#ifdef NEW_SCHEDULE
    int i = 0;
    uint32_t total_credits = 0;
#endif
    trace_process_hw_queue(hwq);
#ifdef NEW_SCHEDULE
    for (; i < CONFIG_USER_MAX; i++)
        total_credits |= hwq->credits[i];
    if (!total_credits)
        return;
#endif
    hwq->need_processing = false;
    mu_enable = siwifi_txq_take_mu_lock(siwifi_hw);
    if (!mu_enable)
        credit_map = ALL_HWQ_MASK - 1;
    if (siwifi_hw->atf.enable) {
        uint32_t hwq_max_rateinfo = 0;
        list_for_each_entry_safe(txq, next, &hwq->list, sched_list) {
            if (txq->atf.enable == 0)
                continue;
            if (txq->atf.record_rateinfo > hwq_max_rateinfo)
                hwq_max_rateinfo = txq->atf.record_rateinfo;
        }
        siwifi_hw->atf.max_rateinfo = hwq_max_rateinfo;
    }
 list_for_each_entry_safe(txq, next, &hwq->list, sched_list) {
  struct sk_buff_head sk_list_push;
#ifndef NEW_SCHEDULE
        struct siwifi_txhdr *txhdr = NULL;
  struct sk_buff *skb;
#endif
#ifdef TOKEN_ENABLE
        int token_id;
        int ret;
#endif
  bool txq_empty;
        struct siwifi_vif *vif;
  trace_process_txq(txq);
  BUG_ON(!(txq->status & SIWIFI_TXQ_IN_HWQ_LIST));
  BUG_ON(txq->idx == TXQ_INACTIVE);
  BUG_ON(txq->credits <= 0);
  BUG_ON(!siwifi_txq_skb_ready(txq));
        vif = netdev_priv(txq->ndev);
  if (!siwifi_txq_select_user(siwifi_hw, mu_enable, txq, hwq, &user))
   continue;
  if (!hwq->credits[user]) {
   credit_map |= BIT(user);
   if (credit_map == ALL_HWQ_MASK)
    break;
   continue;
  }
#ifdef TOKEN_ENABLE
#if DEBUG_ARRAY_CHECK
        BUG_ON(txq->hwq->id >= NX_TXQ_CNT);
#endif
        ret = siwifi_hwq_get_token_id(siwifi_hw, &siwifi_hw->hwq[txq->hwq->id], txq, &token_id);
        if (ret < 0) {
            pushed_total = 0;
            break;
        }
        if (ret == 1)
            continue;
#endif
        BUG_ON(hwq != txq->hwq);
  txq_empty = siwifi_txq_get_skb_to_push(siwifi_hw, hwq, txq, user,
    &sk_list_push);
#ifdef NEW_SCHEDULE
        if (skb_queue_len(&sk_list_push) > 0) {
            if (siwifi_tx_push_burst(siwifi_hw, hwq, txq, &sk_list_push)) {
                printk("siwifi_tx_push_burst failed");
            } else {
                hwq->credits[user]--;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
                if (txhdr->sw_hdr->amsdu.nb > 0)
                    traffic_detect_be_edca(siwifi_hw, txhdr->sw_hdr->siwifi_vif, hwq->id, txhdr->sw_hdr->amsdu.nb);
                else
                    traffic_detect_be_edca(siwifi_hw, txhdr->sw_hdr->siwifi_vif, hwq->id, 1);
#else
                traffic_detect_be_edca(siwifi_hw, txhdr->sw_hdr->siwifi_vif, hwq->id, 1);
#endif
            }
        }
#else
  while ((skb = __skb_dequeue(&sk_list_push)) != NULL) {
   txhdr = (struct siwifi_txhdr *)skb->data;
   BUG_ON(skb != txhdr->sw_hdr->skb);
   pushed_total++;
            vif->lm_ctl[txq->ndev_idx].tx_memory_usage -= skb->truesize;
            vif->lm_ctl[txq->ndev_idx].tx_cnt --;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
            if (txhdr->sw_hdr->amsdu.nb > 0)
                traffic_detect_be_edca(siwifi_hw, txhdr->sw_hdr->siwifi_vif, hwq->id, txhdr->sw_hdr->amsdu.nb);
            else
                traffic_detect_be_edca(siwifi_hw, txhdr->sw_hdr->siwifi_vif, hwq->id, 1);
#else
            traffic_detect_be_edca(siwifi_hw, txhdr->sw_hdr->siwifi_vif, hwq->id, 1);
#endif
            if (!siwifi_tx_push(siwifi_hw, txhdr, 0))
            {
#ifdef TOKEN_ENABLE
    txhdr->token_id = token_id;
#if DEBUG_ARRAY_CHECK
                BUG_ON(txhdr->sw_hdr->txq->hwq->id >= NX_TXQ_CNT);
                BUG_ON(token_id >= NUM_TX_DESCS_PER_AC);
#endif
    txq->token_pkt_num[txhdr->sw_hdr->txq->hwq->id][token_id] ++;
#endif
    txhdr->sw_hdr->txq->hwq->push_cnt ++;
    push_success++;
   }
  }
#ifdef TOKEN_ENABLE
        siwifi_txq_tokens_record(hwq, txq, token_id);
#endif
#endif
  if (txq_empty) {
   siwifi_txq_del_from_hw_list(txq);
   txq->pkt_sent = 0;
        } else if ((hwq->credits[user] == 0) &&
                   siwifi_txq_is_scheduled(txq)) {
            if (txq->pkt_sent > hwq->size) {
                txq->pkt_sent = 0;
                list_rotate_left(&hwq->list);
            }
        }
        if (txq->push_limit && txq->sta) {
            if (txq->ps_id == LEGACY_PS_ID) {
                txq->sta->ps.sp_cnt[txq->ps_id] -= txq->push_limit;
                txq->push_limit = 0;
            }
        }
        if ((txq->status & (SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL))
                    == (SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL)) {
            if ((vif->lm_ctl[txq->ndev_idx].tx_memory_usage + vif->lm_ctl[txq->ndev_idx].amsdu_tx_memory_usage) <= (vif->lm_ctl[txq->ndev_idx].tx_memory_limit * LM_TXM_RQ_RATIO) &&
                    (vif->lm_ctl[txq->ndev_idx].tx_cnt + vif->lm_ctl[txq->ndev_idx].amsdu_tx_cnt) <= (vif->lm_ctl[txq->ndev_idx].tx_cnt_limit * LM_TXC_RQ_RATIO))
            {
                SIWIFI_TRACE_TXQ_STOP(6, txq, vif);
                txq->status &= ~(SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL);
                txq->memctl_balance --;
                netif_wake_subqueue(txq->ndev, txq->ndev_idx);
                trace_txq_flowctrl_restart(txq);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
                siwifi_hw->stats.queues_starts++;
#endif
            }
            else if (!siwifi_txq_has_data(txq)) {
                SIWIFI_TRACE_TXQ_STOP(7, txq, vif);
                txq->status &= ~(SIWIFI_TXQ_NDEV_FLOW_CTRL | SIWIFI_TXQ_MEMORY_CTRL);
                txq->memctl_balance --;
                netif_wake_subqueue(txq->ndev, txq->ndev_idx);
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
                siwifi_hw->stats.queues_starts++;
#endif
            }
        }
        if (unlikely(txq->status & SIWIFI_TXQ_NDEV_FLOW_CTRL) &&
            skb_queue_len(&txq->sk_list) < SIWIFI_NDEV_FLOW_CTRL_RESTART
            && !(txq->status & SIWIFI_TXQ_MEMORY_CTRL)
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
            && !siwifi_hw->ate_env.tx_frame_start
#endif
            ) {
            SIWIFI_TRACE_TXQ_STOP(8, txq, vif);
            txq->status &= ~SIWIFI_TXQ_NDEV_FLOW_CTRL;
            trace_txq_flowctrl_restart(txq);
        }
    }
#ifndef NEW_SCHEDULE
    if (pushed_total != 0 && push_success == 0) {
        SIWIFI_DBG("set need_processing(%d) case all pushed fail pushed_total=%d\n",
                siwifi_hw->mod_params->is_hb, pushed_total);
        hwq->need_processing = true;
    }
#endif
    if (mu_enable)
        siwifi_txq_release_mu_lock(siwifi_hw);
}
#define SIWIFI_STUCK_CHECK_MAX_TIME (40000000000)
#define SWIFI_STUCK_CHECK_TIMES (512)
void siwifi_txq_stuck_list_process(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_txq *txq, *next;
    u64 now = local_clock();
    list_for_each_entry_safe(txq, next, &siwifi_hw->stuck_check_list, sched_list) {
        WARN_ON(!(txq->status & SIWIFI_TXQ_IN_STUCK_CHECK_LIST));
        WARN_ON((txq->status & SIWIFI_TXQ_IN_HWQ_LIST));
        if (!siwifi_txq_has_data(txq)) {
            siwifi_txq_del_from_stuck_check_list(txq);
            continue;
        }
        if (!time_after64(now, txq->check_stuck_start))
        {
            printk("time range over 64\n");
            txq->check_stuck_start = 0;
        }
        if ((now - txq->check_stuck_start) > SIWIFI_STUCK_CHECK_MAX_TIME)
        {
            siwifi_txq_flush(siwifi_hw, txq);
            siwifi_txq_del_from_stuck_check_list(txq);
            txq->stuck_time ++;
        }
    }
}
void siwifi_hwq_process_all(struct siwifi_hw *siwifi_hw)
{
    int id;
    siwifi_mu_group_sta_select(siwifi_hw);
    for (id = ARRAY_SIZE(siwifi_hw->hwq) - 1; id >= 0 ; id--) {
        if (siwifi_hw->hwq[id].need_processing) {
            siwifi_hwq_process(siwifi_hw, &siwifi_hw->hwq[id]);
        }
    }
    if (((++siwifi_hw->stuck_check_time) % SWIFI_STUCK_CHECK_TIMES) == 0
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
     && likely(!siwifi_hw->ate_env.tx_frame_start)
#endif
        )
        siwifi_txq_stuck_list_process(siwifi_hw);
}
void siwifi_hwq_init(struct siwifi_hw *siwifi_hw)
{
    int i, j;
#ifdef CONFIG_DEBUG_TXQ_STOP
    memset(g_txq_record, 0 ,sizeof(g_txq_record));
#endif
    for (i = 0; i < ARRAY_SIZE(siwifi_hw->hwq); i++) {
        struct siwifi_hwq *hwq = &siwifi_hw->hwq[i];
        for (j = 0 ; j < CONFIG_USER_MAX; j++) {
            hwq->credits[j] = nx_txdesc_cnt[i];
#ifdef NEW_SCHEDULE
            hwq->max_skb_per_desc[j] = nx_max_skb_txdesc_cnt[i] ;
#endif
        }
        hwq->id = i;
        hwq->size = nx_txdesc_cnt[i];
        INIT_LIST_HEAD(&hwq->list);
  siwifi_hw->hwq_credits_dec[i] = 0;
#ifdef TOKEN_ENABLE
  hwq->outstanding_tokens = 0;
  for (j = 0; j < NUM_TX_DESCS_PER_AC; j++)
   hwq->token_status[j] = 0;
  for (j = 0; j < NUM_PER_HWQ_RECORD; j++) {
   hwq->record_txq[j] = 0;
            hwq->record_txq_ready[j] = 0;
        }
  hwq->current_record_num = 0;
#endif
  hwq->cfm_cnt = 0;
  hwq->push_cnt = 0;
    }
}
int siwifi_adjust_hwq_credits(struct siwifi_hw *siwifi_hw,s16 credit_dec)
{
#ifdef NEW_SCHEDULE
    return 0;
#else
    int ret = 0;
    int i, j;
    s16 dec_credit = 0;
    if (credit_dec < -1)
        return -1;
    for (i = 0; i < ARRAY_SIZE(siwifi_hw->hwq); i++) {
        struct siwifi_hwq *hwq = &siwifi_hw->hwq[i];
  if (credit_dec == -1) {
   dec_credit = nx_txdesc_cnt[i] - 1;
     } else {
   dec_credit = (credit_dec < (nx_txdesc_cnt[i] - 1)) ? credit_dec : (nx_txdesc_cnt[i] - 1);
  }
        for (j = 0; j < CONFIG_USER_MAX; j++)
            hwq->credits[j] -= (dec_credit - siwifi_hw->hwq_credits_dec[i]);
  siwifi_hw->hwq_credits_dec[i] = dec_credit;
    }
    return ret;
#endif
}
