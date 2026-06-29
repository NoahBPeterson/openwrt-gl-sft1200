#ifndef _SIWIFI_IPC_UTILS_H_
#define _SIWIFI_IPC_UTILS_H_ 
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/skbuff.h>
#include "lmac_msg.h"
#ifdef CONFIG_SIWIFI_DBG
#define SIWIFI_DBG printk
#else
#define SIWIFI_DBG(a...) do {} while (0)
#endif
#define SIWIFI_DBG_EARLY_SKB(a...) do {} while (0)
#ifdef CONFIG_SIWIFI_DBG_ASSERT
#define SIWIFI_DBG_ASSERT_BLOCK(x) do { \
    if(!x){WARN_ON(1);while(1){}} while (0)
#else
#define SIWIFI_DBG_ASSERT_BLOCK(x) do {} while (0)
#endif
#define SIWIFI_FN_ENTRY_STR ">>> %s(%d)\n", __func__ ,__LINE__
#define SIWIFI_FN_EXIT_STR "<<< %s(%d)\n", __func__ ,__LINE__
enum siwifi_dev_flag {
    SIWIFI_DEV_STACK_RESTARTING,
    SIWIFI_DEV_STARTED,
#ifdef CONFIG_SIWIFI_RF_RECALI
    SIWIFI_DEV_CALIBRATING,
    SIWIFI_DEV_HW_DEAD
#endif
};
struct siwifi_hw;
struct siwifi_ipc_elem {
    void *addr;
    dma_addr_t dma_addr;
};
struct siwifi_ipc_elem_pool {
    int nb;
    struct siwifi_ipc_elem *buf;
    struct dma_pool *pool;
};
struct siwifi_ipc_elem_var {
    void *addr;
    void *real_addr;
    dma_addr_t dma_addr;
    size_t size;
    size_t real_size;
};
#ifdef CONFIG_ERROR_DUMP
#define ERROR_BUF_MAX_SIZE 512
#define ERROR_INFO_MAX_NUM 20
#define MAX_TIME_NUM 10
struct error_info {
 char *buf;
 u8 len;
 u8 count;
 u64 time[MAX_TIME_NUM];
};
#endif
struct siwifi_ipc_dbgdump_elem {
    struct mutex mutex;
    struct siwifi_ipc_elem_var buf;
#ifdef CONFIG_ERROR_DUMP
 struct error_info *errorinfo[ERROR_INFO_MAX_NUM];
 u32 err_total_count;
 u32 err_cur_idx;
 spinlock_t err_lock;
#endif
#ifdef CONFIG_SF16A18_WIFI_LA_ENABLE
 void *la_mem_addr;
 u32 la_len;
#endif
};
static const u32 siwifi_rxbuff_pattern = 0xCAFEFADE;
#define RADIOTAP_HDR_VEND_MAX_LEN 16
#define RADIOTAP_HDR_MAX_LEN 80
#define UNSUP_RX_VEC_DATA_LEN 2
struct siwifi_ipc_skb_elem {
    struct sk_buff *skb;
    dma_addr_t dma_addr;
};
#define SIWIFI_RXBUFF_MAX (64 * NX_REMOTE_STA_MAX)
struct siwifi_ipc_rxbuf_elems {
    struct sk_buff *skb[SIWIFI_RXBUFF_MAX];
    int idx;
};
struct siwifi_skb_cb {
    dma_addr_t dma_addr;
    uint32_t pattern;
    uint32_t idx;
};
#define SIWIFI_RXBUFF_DMA_ADDR_SET(skbuff,addr) \
    ((struct siwifi_skb_cb *)(skbuff->cb))->dma_addr = addr
#define SIWIFI_RXBUFF_DMA_ADDR_GET(skbuff) \
    ((struct siwifi_skb_cb *)(skbuff->cb))->dma_addr
#define SIWIFI_RXBUFF_PATTERN_SET(skbuff,pat) \
    ((struct siwifi_skb_cb *)(skbuff->cb))->pattern = pat
#define SIWIFI_RXBUFF_PATTERN_GET(skbuff) \
    ((struct siwifi_skb_cb *)(skbuff->cb))->pattern
#define SIWIFI_RXBUFF_IDX_SET(skbuff,val) \
    ((struct siwifi_skb_cb *)(skbuff->cb))->idx = val
#define SIWIFI_RXBUFF_IDX_GET(skbuff) \
    ((struct siwifi_skb_cb *)(skbuff->cb))->idx
#define SIWIFI_RXBUFF_VALID_IDX(idx) ((idx) < SIWIFI_RXBUFF_MAX)
#define SIWIFI_RXBUFF_IDX_TO_HOSTID(idx) ((idx) + 1)
#define SIWIFI_RXBUFF_HOSTID_TO_IDX(hostid) ((hostid) - 1)
int siwifi_ipc_rxbuf_elem_allocs(struct siwifi_hw *siwifi_hw);
void siwifi_ipc_rxbuf_elem_pull(struct siwifi_hw *siwifi_hw, struct sk_buff *skb);
void siwifi_ipc_rxbuf_elem_pull_inv(struct siwifi_hw *siwifi_hw, struct sk_buff *skb, bool inv);
void siwifi_ipc_rxbuf_elem_sync(struct siwifi_hw *siwifi_hw, struct sk_buff *skb,
                              int len);
void siwifi_ipc_rxdesc_elem_repush(struct siwifi_hw *siwifi_hw,
                                 struct siwifi_ipc_elem *elem);
void siwifi_ipc_rxbuf_elem_repush(struct siwifi_hw *siwifi_hw,
                                struct sk_buff *skb);
void siwifi_ipc_msg_push(struct siwifi_hw *siwifi_hw, void *msg_buf, uint16_t len);
#ifndef NEW_SCHEDULE
void siwifi_ipc_txdesc_push(struct siwifi_hw *siwifi_hw, void *tx_desc,
                          void *hostid, int hw_queue, int user, uint16_t wmm_param);
#endif
void *siwifi_ipc_fw_trace_desc_get(struct siwifi_hw *siwifi_hw);
int siwifi_ipc_rxbuf_init(struct siwifi_hw *siwifi_hw, uint32_t rx_bufsz);
int siwifi_ipc_init(struct siwifi_hw *siwifi_hw, u8 *shared_ram, void *reg_base);
void siwifi_ipc_deinit(struct siwifi_hw *siwifi_hw);
void siwifi_ipc_start(struct siwifi_hw *siwifi_hw);
void siwifi_ipc_stop(struct siwifi_hw *siwifi_hw);
void siwifi_ipc_tx_drain(struct siwifi_hw *siwifi_hw);
bool siwifi_ipc_tx_pending(struct siwifi_hw *siwifi_hw);
struct ipc_host_env_tag;
int siwifi_ipc_elem_var_allocs(struct siwifi_hw *siwifi_hw,
                             struct siwifi_ipc_elem_var *elem, size_t elem_size,
                             enum dma_data_direction dir,
                             void *buf, const void *init,
                             void (*push)(struct ipc_host_env_tag *, uint32_t),
                             void *real_addr);
void siwifi_ipc_elem_var_deallocs(struct siwifi_hw *siwifi_hw,
                                struct siwifi_ipc_elem_var *elem);
int siwifi_ipc_unsup_rx_vec_elem_allocs(struct siwifi_hw *siwifi_hw,
                                      struct siwifi_ipc_skb_elem *elem);
void siwifi_trigger_restart(struct siwifi_hw *siwifi_hw);
void siwifi_error_ind(struct siwifi_hw *siwifi_hw, bool restart, bool dump_en);
void siwifi_umh_done(struct siwifi_hw *siwifi_hw);
#ifdef CONFIG_ERROR_DUMP
int siwifi_errorinfo_allocs(struct siwifi_hw *siwifi_hw);
void siwifi_errorinfo_deallocs(struct siwifi_hw *siwifi_hw);
void siwifi_save_error_info(struct siwifi_hw *siwifi_hw, char *msg, u8 len);
#endif
bool siwifi_check_skb_is_icmp(struct sk_buff *skb);
void siwifi_ipc_rxbuf_elem_fail_check(struct siwifi_hw *siwifi_hw);
#endif
