#include <net/mac80211.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include "siwifi_utils.h"
#include "siwifi_defs.h"
#include "siwifi_rx.h"
#include "siwifi_tx.h"
#include "siwifi_msg_rx.h"
#include "siwifi_msg_tx.h"
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
#include "siwifi_debugfs.h"
#endif
#include "siwifi_prof.h"
#include "ipc_host.h"
#include "siwifi_mem.h"
extern void siwifi_reprobe_driver(struct siwifi_hw *siwifi_hw);
static int siwifi_ipc_elem_pool_allocs(struct siwifi_hw *siwifi_hw,
                                     struct siwifi_ipc_elem_pool *pool,
                                     int nb, size_t elem_size, char *pool_name,
                                     int (*push)(struct ipc_host_env_tag *,
                                                 void *, uint32_t))
{
    struct siwifi_ipc_elem *buf;
    int i;
    pool->nb = 0;
    pool->buf = siwifi_kmalloc(nb * sizeof(struct siwifi_ipc_elem), GFP_KERNEL);
    if (!pool->buf) {
        dev_err(siwifi_hw->dev, "Allocation of buffer array for %s failed\n",
                pool_name);
        return -ENOMEM;
    }
    pool->pool = dma_pool_create(pool_name, siwifi_hw->dev, elem_size,
                                 cache_line_size(), 0);
    if (!pool->pool) {
        dev_err(siwifi_hw->dev, "Allocation of dma pool %s failed\n",
                pool_name);
        return -ENOMEM;
    }
    for (i = 0, buf = pool->buf; i < nb; buf++, i++) {
        buf->addr = dma_pool_alloc(pool->pool, GFP_KERNEL, &buf->dma_addr);
        if (!buf->addr) {
            dev_err(siwifi_hw->dev, "Allocation of block %d/%d in %s failed\n",
                    (i + 1), nb, pool_name);
            return -ENOMEM;
        }
        pool->nb++;
        memset(buf->addr, 0, elem_size);
        if (push) push(siwifi_hw->ipc_env, buf, (uint32_t)buf->dma_addr);
    }
    return 0;
}
static void siwifi_ipc_elem_pool_deallocs(struct siwifi_ipc_elem_pool *pool)
{
    struct siwifi_ipc_elem *buf;
    int i;
    for (i = 0, buf = pool->buf; i < pool->nb ; buf++, i++) {
        dma_pool_free(pool->pool, buf->addr, buf->dma_addr);
    }
    pool->nb = 0;
    if (pool->pool)
        dma_pool_destroy(pool->pool);
    pool->pool = NULL;
    if (pool->buf)
        siwifi_kfree(pool->buf);
    pool->buf = NULL;
}
int siwifi_ipc_elem_var_allocs(struct siwifi_hw *siwifi_hw,
                             struct siwifi_ipc_elem_var *elem, size_t elem_size,
                             enum dma_data_direction dir,
                             void *buf, const void *init,
                             void (*push)(struct ipc_host_env_tag *, uint32_t),
                             void *real_addr)
{
    if (buf) {
        elem->addr = buf;
        elem->real_addr = real_addr;
    } else {
        elem->real_size = elem_size + 2 * cache_line_size();
        elem->real_addr = siwifi_kmalloc(elem->real_size, GFP_KERNEL);
        elem->addr = (void*)((uint32_t)elem->real_addr + cache_line_size());
        if (!elem->real_addr) {
            dev_err(siwifi_hw->dev, "Allocation of ipc buffer failed size=%d\n", elem_size);
            return -ENOMEM;
        }
    }
    elem->size = elem_size;
    if ((dir == DMA_TO_DEVICE) && init) {
        memcpy(elem->addr, init, elem_size);
    }
    elem->dma_addr = dma_map_single(siwifi_hw->dev, elem->addr, elem_size, dir);
    if (dma_mapping_error(siwifi_hw->dev, elem->dma_addr)) {
        dev_err(siwifi_hw->dev, "DMA mapping failed\n");
        siwifi_kfree(elem->real_addr);
        elem->addr = NULL;
        elem->real_addr = NULL;
        return -EIO;
    }
    if (push)
        push(siwifi_hw->ipc_env, elem->dma_addr);
    return 0;
}
void siwifi_ipc_elem_var_deallocs(struct siwifi_hw *siwifi_hw,
                                struct siwifi_ipc_elem_var *elem)
{
    if (!elem->real_addr)
        return;
    dma_unmap_single(siwifi_hw->dev, elem->dma_addr, elem->size, DMA_TO_DEVICE);
    siwifi_kfree(elem->real_addr);
    elem->addr = NULL;
    elem->real_addr = NULL;
}
int siwifi_ipc_skb_elem_allocs(struct siwifi_hw *siwifi_hw,
                                 struct siwifi_ipc_skb_elem *elem, size_t skb_size,
                                 enum dma_data_direction dir,
                                 int (*push)(struct ipc_host_env_tag *,
                                             void *, uint32_t))
{
    elem->skb = siwifi_dev_alloc_rxskb(skb_size);
    if (unlikely(!elem->skb)) {
        dev_err(siwifi_hw->dev, "Allocation of ipc skb failed\n");
        return -ENOMEM;
    }
    elem->dma_addr = dma_map_single(siwifi_hw->dev, elem->skb->data, skb_size, dir);
    if (unlikely(dma_mapping_error(siwifi_hw->dev, elem->dma_addr))) {
        dev_err(siwifi_hw->dev, "DMA mapping failed\n");
        dev_kfree_skb(elem->skb);
        elem->skb = NULL;
        return -EIO;
    }
    if (push){
        push(siwifi_hw->ipc_env, elem, elem->dma_addr);
    }
    return 0;
}
static void siwifi_ipc_skb_elem_deallocs(struct siwifi_hw *siwifi_hw,
                                       struct siwifi_ipc_skb_elem *elem,
                                       size_t skb_size, enum dma_data_direction dir) {
    if (elem->skb) {
        dma_unmap_single(siwifi_hw->dev, elem->dma_addr, skb_size, dir);
        dev_kfree_skb(elem->skb);
        elem->skb = NULL;
    }
}
int siwifi_ipc_unsup_rx_vec_elem_allocs(struct siwifi_hw *siwifi_hw,
                                      struct siwifi_ipc_skb_elem *elem)
{
    struct rx_vector_desc *rxdesc;
    if (siwifi_ipc_skb_elem_allocs(siwifi_hw, elem,
            siwifi_hw->ipc_env->unsuprxvec_bufsz, DMA_FROM_DEVICE, NULL))
        return -ENOMEM;
    rxdesc = (struct rx_vector_desc *) elem->skb->data;
    rxdesc->pattern = 0;
    dma_sync_single_for_device(siwifi_hw->dev,
                        elem->dma_addr + offsetof(struct rx_vector_desc, pattern),
                        sizeof(rxdesc->pattern), DMA_BIDIRECTIONAL);
    ipc_host_unsup_rx_vec_buf_push(siwifi_hw->ipc_env, elem, (u32) elem->dma_addr);
    return 0;
}
static void siwifi_ipc_unsup_rx_vec_elems_deallocs(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_ipc_skb_elem *elem;
    int i, nb = siwifi_hw->ipc_env->unsuprxvec_bufnb;
    if (!siwifi_hw->e2aunsuprxvec_elems)
        return;
    for (i = 0, elem = siwifi_hw->e2aunsuprxvec_elems; i < nb; i++, elem++) {
        siwifi_ipc_skb_elem_deallocs(siwifi_hw, elem, siwifi_hw->ipc_env->unsuprxvec_bufsz, DMA_FROM_DEVICE);
    }
    siwifi_kfree(siwifi_hw->e2aunsuprxvec_elems);
    siwifi_hw->e2aunsuprxvec_elems = NULL;
}
static int siwifi_ipc_unsup_rx_vec_elems_allocs(struct siwifi_hw *siwifi_hw)
{
   struct siwifi_ipc_skb_elem *elem;
   int i, nb = siwifi_hw->ipc_env->unsuprxvec_bufnb;
   siwifi_hw->e2aunsuprxvec_elems = siwifi_kzalloc(nb * sizeof(struct siwifi_ipc_skb_elem),
                                   GFP_KERNEL);
   if (!siwifi_hw->e2aunsuprxvec_elems) {
       dev_err(siwifi_hw->dev, "Failed to allocate unsuprxvec_elems\n");
       return -ENOMEM;
   }
   for (i = 0, elem = siwifi_hw->e2aunsuprxvec_elems; i < nb; i++, elem++)
   {
       if (siwifi_ipc_unsup_rx_vec_elem_allocs(siwifi_hw, elem)) {
           dev_err(siwifi_hw->dev, "Failed to allocate unsuprxvec buf %d/%d\n",
                   i + 1, nb);
           return -ENOMEM;
       }
   }
   return 0;
}
#ifdef CONFIG_ERROR_DUMP
int siwifi_errorinfo_allocs(struct siwifi_hw *siwifi_hw)
{
    struct error_info *info[ERROR_INFO_MAX_NUM];
    int i,ret;
 ret = 0;
 for (i = 0; i < ERROR_INFO_MAX_NUM; i++){
  info[i] = siwifi_kzalloc(sizeof(struct error_info),GFP_KERNEL);
  if (!info[i]) {
   printk(KERN_CRIT "%s:%d: error_info alloc of size %u failed\n\n",
               __func__, __LINE__, sizeof(struct error_info));
   ret = -ENOMEM;
   goto ERROR;
  }
  siwifi_hw->dbgdump_elem.errorinfo[i] = info[i];
  siwifi_hw->dbgdump_elem.errorinfo[i]->count = 0;
  siwifi_hw->dbgdump_elem.errorinfo[i]->buf = NULL;
 }
 siwifi_hw->dbgdump_elem.err_total_count = 0;
 siwifi_hw->dbgdump_elem.err_cur_idx = 0;
 spin_lock_init(&siwifi_hw->dbgdump_elem.err_lock);
 return 0;
ERROR:
    for(i = 0; i < ERROR_INFO_MAX_NUM; i++) {
  if(info[i])
   siwifi_kfree(info[i]);
 }
    return ret;
}
void siwifi_errorinfo_deallocs(struct siwifi_hw *siwifi_hw)
{
 int i;
 printk("siwifi_errorinfo_deallocs\n");
 for(i = 0; i < ERROR_INFO_MAX_NUM; i++){
  if (siwifi_hw->dbgdump_elem.errorinfo[i]) {
   if(siwifi_hw->dbgdump_elem.errorinfo[i]->buf){
    siwifi_kfree(siwifi_hw->dbgdump_elem.errorinfo[i]->buf);
    siwifi_hw->dbgdump_elem.errorinfo[i]->buf = NULL;
   }
   siwifi_kfree(siwifi_hw->dbgdump_elem.errorinfo[i]);
   siwifi_hw->dbgdump_elem.errorinfo[i] = NULL;
  }
 }
}
#endif
void siwifi_ipc_rxdesc_elem_repush(struct siwifi_hw *siwifi_hw,
                                 struct siwifi_ipc_elem *elem)
{
    struct rxdesc_tag *rxdesc = elem->addr;
    rxdesc->status = 0;
    ipc_host_rxdesc_push(siwifi_hw->ipc_env, elem, (u32)elem->dma_addr);
}
#ifdef DEBUG_CONFIG_PRIV_RX_BUFFER_POOL
uint32_t g_debug_alloc_time_cnt = 0;
u64 g_debug_alloc_start, g_debug_alloc_stop, g_debug_alloc_time_sum;
#endif
int siwifi_ipc_rxbuf_elem_allocs(struct siwifi_hw *siwifi_hw)
{
    struct sk_buff *skb;
    struct hw_rxhdr *hw_rxhdr;
    dma_addr_t dma_addr;
    int size = siwifi_hw->ipc_env->rx_bufsz;
    int nb, idx;
#ifdef DEBUG_CONFIG_PRIV_RX_BUFFER_POOL
    g_debug_alloc_start = local_clock();
#endif
#ifdef CONFIG_SF_SKB_POOL
    skb = __netdev_alloc_skb_from_pool(NULL, size, GFP_ATOMIC, siwifi_hw->skb_pool_dev_param);
#else
    unsigned long long free_mem;
    free_mem = global_zone_page_state(NR_FREE_PAGES) << (PAGE_SHIFT -10);
    if (free_mem < SIWIFI_RX_LOW_MEM_SIZE) {
        siwifi_hw->rx_skb_alloc_fail_cnt++;
        return 1;
    }
    skb = siwifi_dev_alloc_rxskb(size);
#endif
    if (unlikely(!skb)) {
        siwifi_hw->rx_skb_alloc_fail_cnt++;
        return -ENOMEM;
    }
#ifdef DEBUG_CONFIG_PRIV_RX_BUFFER_POOL
    g_debug_alloc_stop = local_clock();
    g_debug_alloc_time_sum += (g_debug_alloc_stop - g_debug_alloc_start);
    g_debug_alloc_time_cnt ++;
    if (g_debug_alloc_time_cnt == 10000) {
        printk("10000 alloc take time : %lld\n", g_debug_alloc_time_sum);
        g_debug_alloc_time_cnt = 0;
        g_debug_alloc_time_sum = 0;
    }
#endif
    dma_addr = dma_map_single(siwifi_hw->dev, skb->data, size, DMA_FROM_DEVICE);
    if (unlikely(dma_mapping_error(siwifi_hw->dev, dma_addr))) {
        dev_err(siwifi_hw->dev, "Failed to map rx buffer\n");
        siwifi_hw->rx_skb_alloc_fail_cnt++;
        goto err_skb;
    }
    hw_rxhdr = (struct hw_rxhdr *)skb->data;
    hw_rxhdr->pattern = 0;
    dma_sync_single_for_device(siwifi_hw->dev,
                               dma_addr + offsetof(struct hw_rxhdr, pattern),
                               sizeof(hw_rxhdr->pattern), DMA_BIDIRECTIONAL);
    nb = 0;
    idx = siwifi_hw->rxbuf_elems.idx;
#if DEBUG_ARRAY_CHECK
    BUG_ON(idx >= SIWIFI_RXBUFF_MAX);
#endif
    while (siwifi_hw->rxbuf_elems.skb[idx] && nb < SIWIFI_RXBUFF_MAX) {
        idx = ( idx + 1 ) % SIWIFI_RXBUFF_MAX;
        nb++;
    }
    if (nb == SIWIFI_RXBUFF_MAX)
        printk("siwifi_hw->reord_alloc_cnt = %d, siwifi_hw->reord_release_cnt = %d\n", siwifi_hw->reord_alloc_cnt, siwifi_hw->reord_release_cnt);
    if (WARN((nb == SIWIFI_RXBUFF_MAX), "No more free space for rxbuff")) {
        siwifi_hw->rx_skb_alloc_fail_cnt++;
        goto err_dma;
    }
    siwifi_hw->rxbuf_elems.skb[idx] = skb;
    SIWIFI_RXBUFF_DMA_ADDR_SET(skb, dma_addr);
    SIWIFI_RXBUFF_PATTERN_SET(skb, siwifi_rxbuff_pattern);
    SIWIFI_RXBUFF_IDX_SET(skb, idx);
    ipc_host_rxbuf_push(siwifi_hw->ipc_env, SIWIFI_RXBUFF_IDX_TO_HOSTID(idx),
                        dma_addr);
    siwifi_hw->rxbuf_elems.idx = ( idx + 1 ) % SIWIFI_RXBUFF_MAX;
    return 0;
  err_dma:
    dma_unmap_single(siwifi_hw->dev, dma_addr, size, DMA_FROM_DEVICE);
  err_skb:
    dev_kfree_skb(skb);
    return -ENOMEM;
}
void siwifi_ipc_rxbuf_elem_repush(struct siwifi_hw *siwifi_hw,
                                struct sk_buff *skb)
{
    dma_addr_t dma_addr;
    struct hw_rxhdr *hw_rxhdr = (struct hw_rxhdr *)skb->data;
    int idx;
    hw_rxhdr->pattern = 0;
    dma_addr = SIWIFI_RXBUFF_DMA_ADDR_GET(skb);
    dma_sync_single_for_device(siwifi_hw->dev,
                               dma_addr + offsetof(struct hw_rxhdr, pattern),
                               sizeof(hw_rxhdr->pattern), DMA_BIDIRECTIONAL);
    idx = SIWIFI_RXBUFF_IDX_GET(skb);
    ipc_host_rxbuf_push(siwifi_hw->ipc_env, SIWIFI_RXBUFF_IDX_TO_HOSTID(idx),
                        dma_addr);
}
static int siwifi_ipc_rxbuf_elems_allocs(struct siwifi_hw *siwifi_hw)
{
    int i, nb = siwifi_hw->ipc_env->rx_bufnb;
    for (i = 0; i < SIWIFI_RXBUFF_MAX; i++) {
        siwifi_hw->rxbuf_elems.skb[i] = NULL;
    }
    siwifi_hw->rxbuf_elems.idx = 0;
    for (i = 0; i < nb; i++) {
        if (siwifi_ipc_rxbuf_elem_allocs(siwifi_hw)) {
            return -ENOMEM;
        }
    }
    siwifi_hw->rx_skb_alloc_fail_cnt = 0;
    return 0;
}
static void siwifi_ipc_rxbuf_elems_deallocs(struct siwifi_hw *siwifi_hw)
{
    struct sk_buff *skb;
    int i;
    for (i = 0; i < SIWIFI_RXBUFF_MAX; i++) {
        if (siwifi_hw->rxbuf_elems.skb[i]) {
            skb = siwifi_hw->rxbuf_elems.skb[i];
            dma_unmap_single(siwifi_hw->dev, SIWIFI_RXBUFF_DMA_ADDR_GET(skb),
                             siwifi_hw->ipc_env->rx_bufsz, DMA_FROM_DEVICE);
            dev_kfree_skb(skb);
            siwifi_hw->rxbuf_elems.skb[i] = NULL;
        }
    }
}
void siwifi_ipc_rxbuf_elem_pull_inv(struct siwifi_hw *siwifi_hw, struct sk_buff *skb, bool inv)
{
    unsigned int idx = SIWIFI_RXBUFF_IDX_GET(skb);
    if (SIWIFI_RXBUFF_VALID_IDX(idx) && (siwifi_hw->rxbuf_elems.skb[idx] == skb)) {
        dma_addr_t dma_addr = SIWIFI_RXBUFF_DMA_ADDR_GET(skb);
        siwifi_hw->rxbuf_elems.skb[idx] = NULL;
        dma_unmap_single(siwifi_hw->dev, dma_addr,
                         siwifi_hw->ipc_env->rx_bufsz, DMA_FROM_DEVICE);
        if (inv) {
            dma_sync_single_for_device(siwifi_hw->dev, dma_addr,
                                   siwifi_hw->ipc_env->rx_bufsz, DMA_FROM_DEVICE);
        }
    } else {
        WARN(1, "Incorrect rxbuff idx skb=%p table[%u]=%p", skb, idx,
             idx < SIWIFI_RXBUFF_MAX ? siwifi_hw->rxbuf_elems.skb[idx] : NULL);
    }
    SIWIFI_RXBUFF_PATTERN_SET(skb, 0);
    SIWIFI_RXBUFF_IDX_SET(skb, SIWIFI_RXBUFF_MAX);
}
void siwifi_ipc_rxbuf_elem_pull(struct siwifi_hw *siwifi_hw, struct sk_buff *skb)
{
    siwifi_ipc_rxbuf_elem_pull_inv(siwifi_hw, skb, false);
}
void siwifi_ipc_rxbuf_elem_sync(struct siwifi_hw *siwifi_hw, struct sk_buff *skb,
                              int len)
{
    unsigned int idx = SIWIFI_RXBUFF_IDX_GET(skb);
    if (SIWIFI_RXBUFF_VALID_IDX(idx) && (siwifi_hw->rxbuf_elems.skb[idx] == skb)) {
        dma_addr_t dma_addr = SIWIFI_RXBUFF_DMA_ADDR_GET(skb);
        dma_sync_single_for_cpu(siwifi_hw->dev, dma_addr, len, DMA_FROM_DEVICE);
    } else {
        WARN(1, "Incorrect rxbuff idx skb=%p table[%u]=%p", skb, idx,
             idx < SIWIFI_RXBUFF_MAX ? siwifi_hw->rxbuf_elems.skb[idx] : NULL);
    }
}
#ifdef NEW_SCHEDULE
#define SIWIFI_IPC_A2ETXDESC_POOL(a,b) "#siwifi_ipc_a2erxdesc_pool##a_##b#"
static void siwifi_ipc_txdesc_deallocs(struct siwifi_hw *siwifi_hw)
{
    int i, j;
    for (i = 0; i < CONFIG_USER_MAX; i++) {
        for (j = 0; j < NX_TXQ_CNT; j++) {
            siwifi_ipc_elem_pool_deallocs(&siwifi_hw->a2etxdesc_pool[i][j]);
        }
    }
}
static int siwifi_ipc_txdesc_allocs(struct siwifi_hw *siwifi_hw)
{
    int i, j, k;
    struct siwifi_ipc_elem_pool *pool;
    for (i = 0; i < CONFIG_USER_MAX; i++) {
        for (j = 0; j < NX_TXQ_CNT; j++) {
            pool = &siwifi_hw->a2etxdesc_pool[i][j];
            if (siwifi_ipc_elem_pool_allocs(siwifi_hw, pool,
                    nx_txdesc_cnt[j],
                    sizeof(struct txdesc_api) * MAX_SKBS_PER_TOKEN,
                    SIWIFI_IPC_A2ETXDESC_POOL(i,j),
                    NULL)) {
                printk("can not allocate txdesc\n");
                return -1;
            }
            for (k = 0; k < nx_txdesc_cnt[j]; k++) {
                ipc_host_txdesc_map(siwifi_hw->ipc_env, i, j, k, pool->buf[k].dma_addr);
            }
        }
    }
    return 0;
}
#endif
static void siwifi_elems_deallocs(struct siwifi_hw *siwifi_hw)
{
    siwifi_ipc_rxbuf_elems_deallocs(siwifi_hw);
    siwifi_ipc_unsup_rx_vec_elems_deallocs(siwifi_hw);
#ifdef NEW_SCHEDULE
    siwifi_ipc_txdesc_deallocs(siwifi_hw);
#endif
    siwifi_ipc_elem_pool_deallocs(&siwifi_hw->e2arxdesc_pool);
    siwifi_ipc_elem_var_deallocs(siwifi_hw, &siwifi_hw->dbg_mgmt_info_elem);
    siwifi_ipc_elem_var_deallocs(siwifi_hw, &siwifi_hw->dbg_ctrl_info_elem);
    siwifi_ipc_elem_pool_deallocs(&siwifi_hw->e2amsgs_pool);
    siwifi_ipc_elem_pool_deallocs(&siwifi_hw->dbgmsgs_pool);
#ifdef CONFIG_SIWIFI_RADAR
    siwifi_ipc_elem_pool_deallocs(&siwifi_hw->e2aradars_pool);
#endif
    siwifi_ipc_elem_var_deallocs(siwifi_hw, &siwifi_hw->pattern_elem);
#ifdef CFG_DBGDUMP
    siwifi_ipc_elem_var_deallocs(siwifi_hw, &siwifi_hw->dbgdump_elem.buf);
#ifdef CONFIG_SF16A18_WIFI_LA_ENABLE
 if(siwifi_hw->dbgdump_elem.la_mem_addr)
  dma_unmap_single(siwifi_hw->dev, (dma_addr_t)siwifi_hw->dbgdump_elem.la_mem_addr, siwifi_hw->dbgdump_elem.la_len, DMA_TO_DEVICE);
    siwifi_hw->dbgdump_elem.la_mem_addr = NULL;
#endif
#endif
#ifdef CONFIG_SIWIFI_TRX_STAT
    siwifi_ipc_elem_var_deallocs(siwifi_hw, &siwifi_hw->dbg_trx_stat_elem);
#endif
}
#ifdef CONFIG_SF16A18_WIFI_LA_ENABLE
 u32_l g_la_mem[LA_MEM_LEN/4];
#endif
static int siwifi_elems_allocs(struct siwifi_hw *siwifi_hw)
{
    SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);
    if (dma_set_coherent_mask(siwifi_hw->dev, DMA_BIT_MASK(32)))
        goto err_alloc;
    if (siwifi_ipc_elem_pool_allocs(siwifi_hw, &siwifi_hw->e2amsgs_pool,
                                  siwifi_hw->ipc_env->ipc_e2amsg_bufnb,
                                  siwifi_hw->ipc_env->ipc_e2amsg_bufsz,
                                  "siwifi_ipc_e2amsgs_pool",
                                  ipc_host_msgbuf_push))
        goto err_alloc;
    if (siwifi_ipc_elem_pool_allocs(siwifi_hw, &siwifi_hw->dbgmsgs_pool,
                                  siwifi_hw->ipc_env->ipc_dbg_bufnb,
                                  siwifi_hw->ipc_env->ipc_dbg_bufsz,
                                  "siwifi_ipc_dbgmsgs_pool",
                                  ipc_host_dbgbuf_push))
        goto err_alloc;
#ifdef CONFIG_SIWIFI_RADAR
    if (siwifi_ipc_elem_pool_allocs(siwifi_hw, &siwifi_hw->e2aradars_pool,
                                  siwifi_hw->ipc_env->radar_bufnb,
                                  siwifi_hw->ipc_env->radar_bufsz,
                                  "siwifi_ipc_e2aradars_pool",
                                  ipc_host_radarbuf_push))
        goto err_alloc;
#endif
    if (siwifi_ipc_unsup_rx_vec_elems_allocs(siwifi_hw))
        goto err_alloc;
    if (siwifi_ipc_elem_var_allocs(siwifi_hw, &siwifi_hw->pattern_elem,
                                 sizeof(u32), DMA_TO_DEVICE,
                                 NULL, &siwifi_rxbuff_pattern,
                                 ipc_host_patt_addr_push, NULL))
        goto err_alloc;
#ifdef CFG_DBGDUMP
    if (siwifi_ipc_elem_var_allocs(siwifi_hw, &siwifi_hw->dbgdump_elem.buf,
                                 sizeof(struct dbg_debug_dump_tag),
                                 DMA_FROM_DEVICE, NULL, NULL,
                                 ipc_host_dbginfobuf_push, NULL))
 {
        goto err_alloc;
 }
#ifdef CONFIG_SF16A18_WIFI_LA_ENABLE
 else
 {
        struct dbg_debug_dump_tag *dbg_debug_dump;
  dbg_debug_dump = (struct dbg_debug_dump_tag *)(siwifi_hw->dbgdump_elem.buf.addr);
  siwifi_hw->dbgdump_elem.la_mem_addr = (void *)&g_la_mem;
  siwifi_hw->dbgdump_elem.la_len = (u32)sizeof(g_la_mem);
  dbg_debug_dump->la_mem_dma_addr = (u32_l)(uint32_t)dma_map_single(siwifi_hw->dev, (siwifi_hw->dbgdump_elem.la_mem_addr),
    sizeof(g_la_mem), DMA_TO_DEVICE);
  if (dma_mapping_error(siwifi_hw->dev, (dma_addr_t)dbg_debug_dump->la_mem_dma_addr)) {
   dev_err(siwifi_hw->dev, "DMA mapping failed\n");
   siwifi_hw->dbgdump_elem.la_mem_addr = NULL;
   dbg_debug_dump->la_mem_dma_addr = 0;
   goto err_alloc;
  }
 }
#endif
#endif
    if (siwifi_ipc_elem_pool_allocs(siwifi_hw, &siwifi_hw->e2arxdesc_pool,
                                  siwifi_hw->ipc_env->rxdesc_nb,
                                  sizeof(struct rxdesc_tag),
                                  "siwifi_ipc_e2arxdesc_pool",
                                  ipc_host_rxdesc_push))
        goto err_alloc;
    if (siwifi_ipc_elem_var_allocs(siwifi_hw, &siwifi_hw->dbg_mgmt_info_elem,
                                    sizeof(struct dbg_mgmt_info),
                                    DMA_FROM_DEVICE,
                                    NULL, NULL, NULL, NULL))
        goto err_alloc;
    if (siwifi_ipc_elem_var_allocs(siwifi_hw, &siwifi_hw->dbg_ctrl_info_elem,
                                    sizeof(struct dbg_ctrl_info),
                                    DMA_FROM_DEVICE,
                                    NULL, NULL, NULL, NULL))
        goto err_alloc;
#ifdef NEW_SCHEDULE
    if (siwifi_ipc_txdesc_allocs(siwifi_hw))
        goto err_alloc;
#endif
    if (siwifi_hw->dying_gasp_info_elem.dma_addr) {
        ipc_host_dying_gasp_addr_push(siwifi_hw->ipc_env, siwifi_hw->dying_gasp_info_elem.dma_addr);
    }
#ifdef CONFIG_SIWIFI_TRX_STAT
    if (siwifi_ipc_elem_var_allocs(siwifi_hw, &siwifi_hw->dbg_trx_stat_elem,
                                    max(sizeof(struct dbg_tx_stat), sizeof(struct dbg_rx_stat)),
                                    DMA_FROM_DEVICE,
                                    NULL, NULL, NULL, NULL))
        goto err_alloc;
#endif
    return 0;
err_alloc:
    siwifi_elems_deallocs(siwifi_hw);
    return -ENOMEM;
}
void siwifi_ipc_msg_push(struct siwifi_hw *siwifi_hw, void *msg_buf, uint16_t len)
{
    ipc_host_msg_push(siwifi_hw->ipc_env, msg_buf, len);
}
#ifndef NEW_SCHEDULE
void siwifi_ipc_txdesc_push(struct siwifi_hw *siwifi_hw, void *tx_desc,
                          void *hostid, int hw_queue, int user, u16 wmm_param)
{
    volatile struct txdesc_host *txdesc_host;
    txdesc_host = ipc_host_txdesc_get(siwifi_hw->ipc_env, hw_queue, user);
    BUG_ON(!txdesc_host);
#if 0
    memcpy(&txdesc_host->api, tx_desc, sizeof(*desc));
#else
    {
        u32 *src, *dst;
        int i;
        dst = (typeof(dst))&txdesc_host->api;
        src = (typeof(src))tx_desc;
        for (i = 0; i < sizeof(txdesc_host->api) / sizeof(*src); i++)
            *dst++ = *src++;
    }
    txdesc_host->wmm_param = wmm_param;
#endif
    wmb();
    ipc_host_txdesc_push(siwifi_hw->ipc_env, hw_queue, user, hostid);
}
#endif
void *siwifi_ipc_fw_trace_desc_get(struct siwifi_hw *siwifi_hw)
{
    return (void *)&(siwifi_hw->ipc_env->shared->trace_pattern);
}
static u8 siwifi_msgind(void *pthis, void *hostid)
{
    struct siwifi_hw *siwifi_hw = pthis;
    struct siwifi_ipc_elem *elem = hostid;
    struct ipc_e2a_msg *msg = elem->addr;
    u8 ret = 0;
    REG_SW_SET_PROFILING(siwifi_hw, SW_PROF_MSGIND);
    if (msg->pattern != IPC_MSGE2A_VALID_PATTERN) {
        ret = -1;
        goto msg_no_push;
    }
    siwifi_rx_handle_msg(siwifi_hw, msg);
    msg->pattern = 0;
    wmb();
    ipc_host_msgbuf_push(siwifi_hw->ipc_env, elem, (u32)elem->dma_addr);
msg_no_push:
    REG_SW_CLEAR_PROFILING(siwifi_hw, SW_PROF_MSGIND);
    return ret;
}
static u8 siwifi_msgackind(void *pthis, void *hostid)
{
    struct siwifi_hw *siwifi_hw = (struct siwifi_hw *)pthis;
    siwifi_hw->cmd_mgr.llind(&siwifi_hw->cmd_mgr, (struct siwifi_cmd *)hostid);
    return -1;
}
static u8 siwifi_radarind(void *pthis, void *hostid)
{
#ifdef CONFIG_SIWIFI_RADAR
    struct siwifi_hw *siwifi_hw = pthis;
    struct siwifi_ipc_elem *elem = hostid;
    struct radar_pulse_array_desc *pulses = elem->addr;
    u8 ret = 0;
    int i;
    if (pulses->cnt == 0) {
        ret = -1;
        goto radar_no_push;
    }
    if (siwifi_radar_detection_is_enable(&siwifi_hw->radar, pulses->idx)) {
        for (i = 0; i < pulses->cnt; i++) {
            struct siwifi_radar_pulses *p = &siwifi_hw->radar.pulses[pulses->idx];
#if 1
            struct radar_pulse *rp = (struct radar_pulse*)&pulses->pulse[i];
            if (rp->rep <= 1)
                continue;
#endif
            p->buffer[p->index] = pulses->pulse[i];
            p->index = (p->index + 1) % SIWIFI_RADAR_PULSE_MAX;
            if (p->count < SIWIFI_RADAR_PULSE_MAX)
                p->count++;
        }
        if (! work_pending(&siwifi_hw->radar.detection_work))
            schedule_work(&siwifi_hw->radar.detection_work);
    }
    pulses->cnt = 0;
    wmb();
    ipc_host_radarbuf_push(siwifi_hw->ipc_env, elem, (u32)elem->dma_addr);
radar_no_push:
    return ret;
#else
    return -1;
#endif
}
static void siwifi_prim_tbtt_ind(void *pthis)
{
    return;
}
static void siwifi_sec_tbtt_ind(void *pthis)
{
}
#ifdef CONFIG_ERROR_DUMP
void siwifi_save_error_info(struct siwifi_hw *siwifi_hw, char *msg, u8 len){
 u32 idx = 0;
 u32 time_idx = 0;
 u16 length = len;
 unsigned long now = jiffies - 0xFFFF8AF5;
 char *buf = NULL;
  if(len > ERROR_BUF_MAX_SIZE)
   length = ERROR_BUF_MAX_SIZE;
  spin_lock(&siwifi_hw->dbgdump_elem.err_lock);
  idx = siwifi_hw->dbgdump_elem.err_cur_idx;
  if(siwifi_hw->dbgdump_elem.errorinfo[idx]){
   if(siwifi_hw->dbgdump_elem.errorinfo[idx]->buf &&
    (!strncmp(msg, siwifi_hw->dbgdump_elem.errorinfo[idx]->buf, length)) &&
    (siwifi_hw->dbgdump_elem.errorinfo[idx]->count < MAX_TIME_NUM)){
    time_idx = siwifi_hw->dbgdump_elem.errorinfo[idx]->count;
    siwifi_hw->dbgdump_elem.errorinfo[idx]->time[time_idx] = now;
    siwifi_hw->dbgdump_elem.errorinfo[idx]->count ++;
   }
   else{
    if(siwifi_hw->dbgdump_elem.errorinfo[idx]->buf){
     printk("siwifi_hw->dbgdump_elem.err_cur_idx %d,%d,%p,%d,%d\n",idx,siwifi_hw->dbgdump_elem.err_cur_idx,siwifi_hw->dbgdump_elem.errorinfo[idx]->buf,
      strncmp(msg, siwifi_hw->dbgdump_elem.errorinfo[idx]->buf, length),
      siwifi_hw->dbgdump_elem.errorinfo[idx]->count);
    }
    if((siwifi_hw->dbgdump_elem.err_cur_idx >= (ERROR_INFO_MAX_NUM - 1)) || (siwifi_hw->dbgdump_elem.err_total_count == 0))
     siwifi_hw->dbgdump_elem.err_cur_idx = 0;
    else
     siwifi_hw->dbgdump_elem.err_cur_idx ++;
    idx = siwifi_hw->dbgdump_elem.err_cur_idx;
    if(siwifi_hw->dbgdump_elem.errorinfo[idx]->buf){
     siwifi_kfree(siwifi_hw->dbgdump_elem.errorinfo[idx]->buf);
     siwifi_hw->dbgdump_elem.errorinfo[idx]->len = 0;
     siwifi_hw->dbgdump_elem.errorinfo[idx]->count = 0;
     siwifi_hw->dbgdump_elem.errorinfo[idx]->buf = NULL;
    }
    if(!siwifi_hw->dbgdump_elem.errorinfo[idx]->buf){
     buf = siwifi_kzalloc(length, GFP_ATOMIC);
     if (!buf) {
      printk(KERN_CRIT "%s:%d: error_info alloc of size %u failed\n\n",
        __func__, __LINE__, sizeof(struct error_info));
      spin_unlock(&siwifi_hw->dbgdump_elem.err_lock);
      return;
     }
     siwifi_hw->dbgdump_elem.errorinfo[idx]->buf = buf;
     siwifi_hw->dbgdump_elem.errorinfo[idx]->len = length;
    }
    memcpy(siwifi_hw->dbgdump_elem.errorinfo[idx]->buf, msg, length);
    time_idx = siwifi_hw->dbgdump_elem.errorinfo[idx]->count;
    siwifi_hw->dbgdump_elem.errorinfo[idx]->time[time_idx] = now;
    siwifi_hw->dbgdump_elem.errorinfo[idx]->count ++;
   }
   siwifi_hw->dbgdump_elem.err_total_count ++;
   spin_unlock(&siwifi_hw->dbgdump_elem.err_lock);
  }
  else{
   spin_unlock(&siwifi_hw->dbgdump_elem.err_lock);
   printk(KERN_CRIT "errorinfo[%d] is null\n",idx);
  }
}
#endif
static u8 siwifi_dbgind(void *pthis, void *hostid)
{
    struct siwifi_hw *siwifi_hw = (struct siwifi_hw *)pthis;
    struct siwifi_ipc_elem *elem = hostid;
    struct ipc_dbg_msg *dbg_msg = elem->addr;
    u8 ret = 0;
    REG_SW_SET_PROFILING(siwifi_hw, SW_PROF_DBGIND);
    if (dbg_msg->pattern != IPC_DBG_VALID_PATTERN) {
        ret = -1;
        goto dbg_no_push;
    }
    printk("lmac[%d] %s", siwifi_hw->mod_params->is_hb, (char *)dbg_msg->string);
#ifdef CONFIG_ERROR_DUMP
 if(!strncmp((char *)dbg_msg->string,"ASSERT", 6)){
  siwifi_save_error_info(siwifi_hw,(char *)dbg_msg->string, strlen((char *)dbg_msg->string));
 }
#endif
    dbg_msg->pattern = 0;
    wmb();
    ipc_host_dbgbuf_push(siwifi_hw->ipc_env, elem, (u32)elem->dma_addr);
dbg_no_push:
    REG_SW_CLEAR_PROFILING(siwifi_hw, SW_PROF_DBGIND);
    return ret;
}
int siwifi_ipc_rxbuf_init(struct siwifi_hw *siwifi_hw, uint32_t rx_bufsz)
{
    siwifi_hw->ipc_env->rx_bufsz = rx_bufsz;
    siwifi_hw->ipc_env->rx_pd_offset =
        max(SIWIFI_RX_HEADROOM, (uint32_t)max(SIWIFI_TX_HEADROOM, SIWIFI_TX_AMSDU_HEADROOM));
    printk("tx headroom : %d, tx amsdu headroom : %d, rx headroom : %d, rx pd offset %d\n",
            SIWIFI_TX_HEADROOM, SIWIFI_TX_AMSDU_HEADROOM, SIWIFI_RX_HEADROOM, siwifi_hw->ipc_env->rx_pd_offset);
    return(siwifi_ipc_rxbuf_elems_allocs(siwifi_hw));
}
int siwifi_ipc_init(struct siwifi_hw *siwifi_hw, u8 *shared_ram, void *reg_base)
{
    struct ipc_host_cb_tag cb;
    SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);
    cb.recv_data_ind = siwifi_rxdataind;
    cb.recv_radar_ind = siwifi_radarind;
    cb.recv_msg_ind = siwifi_msgind;
    cb.recv_msgack_ind = siwifi_msgackind;
    cb.recv_dbg_ind = siwifi_dbgind;
#ifdef NEW_SCHEDULE
    cb.send_data_cfm_burst = siwifi_txdatacfm_burst;
#else
    cb.send_data_cfm = siwifi_txdatacfm;
#endif
    cb.prim_tbtt_ind = siwifi_prim_tbtt_ind;
    cb.sec_tbtt_ind = siwifi_sec_tbtt_ind;
    cb.recv_unsup_rx_vec_ind = siwifi_unsup_rx_vec_ind;
    siwifi_hw->ipc_env = (struct ipc_host_env_tag *)
                       siwifi_kzalloc(sizeof(struct ipc_host_env_tag), GFP_KERNEL);
    if (!siwifi_hw->ipc_env) {
       dev_err(siwifi_hw->dev, "Failed to allocate ipc_env\n");
       return -ENOMEM;
    }
    siwifi_hw->ipc_env->mod_params = siwifi_hw->mod_params;
    ipc_host_init(siwifi_hw->ipc_env, &cb,
                  (struct ipc_shared_env_tag *)shared_ram, reg_base, siwifi_hw);
    siwifi_cmd_mgr_init(&siwifi_hw->cmd_mgr);
    return siwifi_elems_allocs(siwifi_hw);
}
void siwifi_ipc_deinit(struct siwifi_hw *siwifi_hw)
{
    SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);
    siwifi_ipc_tx_drain(siwifi_hw);
    siwifi_cmd_mgr_deinit(&siwifi_hw->cmd_mgr);
    siwifi_elems_deallocs(siwifi_hw);
#ifdef NEW_SCHEDULE
    ipc_host_deinit(siwifi_hw->ipc_env);
#endif
    siwifi_kfree(siwifi_hw->ipc_env);
    siwifi_hw->ipc_env = NULL;
}
void siwifi_ipc_start(struct siwifi_hw *siwifi_hw)
{
    ipc_host_enable_irq(siwifi_hw->ipc_env, IPC_IRQ_E2A_ALL);
}
void siwifi_ipc_stop(struct siwifi_hw *siwifi_hw)
{
    ipc_host_disable_irq(siwifi_hw->ipc_env, IPC_IRQ_E2A_ALL);
}
void siwifi_ipc_tx_drain(struct siwifi_hw *siwifi_hw)
{
    int i, j;
    SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);
    if (!siwifi_hw->ipc_env) {
        printk(KERN_CRIT "%s: bypassing (restart must have failed)\n", __func__);
        return;
    }
    for (i = 0; i < SIWIFI_HWQ_NB; i++) {
        for (j = 0; j < nx_txuser_cnt[i]; j++) {
            struct sk_buff *skb;
#ifdef NEW_SCHEDULE
            struct sk_buff_head *skb_list;
            while((skb_list = (struct sk_buff_head *)ipc_host_tx_flush(siwifi_hw->ipc_env, i, j))) {
                while ((skb = __skb_dequeue(skb_list)) != NULL) {
                    struct siwifi_sw_txhdr *sw_txhdr =
                        ((struct siwifi_txhdr *)skb->data)->sw_hdr;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
                    if (sw_txhdr->desc.host.flags & TXU_CNTRL_AMSDU) {
                        siwifi_free_amsdu(siwifi_hw, sw_txhdr);
                    }
#endif
                    dma_unmap_single(siwifi_hw->dev, sw_txhdr->dma_addr,
                                     sw_txhdr->map_len, DMA_TO_DEVICE);
                    skb_pull(skb, sw_txhdr->headroom);
                    siwifi_free_swtxhdr(siwifi_hw, sw_txhdr);
                    dev_kfree_skb_any(skb);
                }
            }
#else
            while ((skb = (struct sk_buff *)ipc_host_tx_flush(siwifi_hw->ipc_env, i, j))) {
                struct siwifi_sw_txhdr *sw_txhdr =
                    ((struct siwifi_txhdr *)skb->data)->sw_hdr;
#ifdef CONFIG_SIWIFI_AMSDUS_TX
                if (sw_txhdr->desc.host.flags & TXU_CNTRL_AMSDU) {
                    siwifi_free_amsdu(siwifi_hw, sw_txhdr);
                }
#endif
                dma_unmap_single(siwifi_hw->dev, sw_txhdr->dma_addr,
                                 sw_txhdr->map_len, DMA_TO_DEVICE);
                skb_pull(skb, sw_txhdr->headroom);
                siwifi_free_swtxhdr(siwifi_hw, sw_txhdr);
                dev_kfree_skb_any(skb);
            }
#endif
        }
    }
}
bool siwifi_ipc_tx_pending(struct siwifi_hw *siwifi_hw)
{
    return ipc_host_tx_frames_pending(siwifi_hw->ipc_env);
}
void siwifi_trigger_restart(struct siwifi_hw *siwifi_hw)
{
    set_bit(SIWIFI_DEV_STACK_RESTARTING, &siwifi_hw->drv_flags);
    clear_bit(SIWIFI_DEV_STARTED, &siwifi_hw->drv_flags);
#ifdef LOCAL_RECOVERY
    schedule_work(&siwifi_hw->restart_work);
#endif
}
void siwifi_error_ind(struct siwifi_hw *siwifi_hw, bool restart, bool dump_en)
{
#ifdef CFG_DBGDUMP
    struct siwifi_ipc_elem_var *elem = &siwifi_hw->dbgdump_elem.buf;
#endif
#ifdef CONFIG_SF16A18_WIFI_LA_ENABLE
    struct dbg_debug_dump_tag *dump = elem->addr;
#endif
#ifdef CFG_DBGDUMP
    dma_sync_single_for_device(siwifi_hw->dev, elem->dma_addr, elem->size,
                    DMA_FROM_DEVICE);
#ifdef CONFIG_SF16A18_WIFI_LA_ENABLE
 dma_sync_single_for_device(siwifi_hw->dev, (dma_addr_t)dump->la_mem_dma_addr,
                        siwifi_hw->dbgdump_elem.la_len, DMA_FROM_DEVICE);
#endif
#endif
    if (
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
        siwifi_hw->debugfs.recovery &&
#endif
        restart) {
        if (!test_bit(SIWIFI_DEV_STARTED, &siwifi_hw->drv_flags)) {
            siwifi_reprobe_driver(siwifi_hw);
        } else if (test_bit(SIWIFI_DEV_STACK_RESTARTING, &siwifi_hw->drv_flags)) {
            siwifi_reprobe_driver(siwifi_hw);
        } else if (list_empty(&siwifi_hw->vifs)) {
            SIWIFI_DBG("vifs is empty, we can't use restart_hw now\n");
            siwifi_reprobe_driver(siwifi_hw);
        } else {
#ifndef LOCAL_RECOVERY
            siwifi_reprobe_driver(siwifi_hw);
            return;
#endif
#if defined(CFG_DBGDUMP)
#if defined(CONFIG_SIWIFI_DEBUGFS) || defined(CONFIG_SIWIFI_PROCFS)
            if (dump_en) {
                siwifi_hw->debugfs.trace_prst = true;
                siwifi_trigger_um_helper(&siwifi_hw->debugfs);
            }
#endif
#endif
            siwifi_trigger_restart(siwifi_hw);
            return;
        }
    }
#if defined(CFG_DBGDUMP)
#if defined(CONFIG_SIWIFI_DEBUGFS) || defined(CONFIG_SIWIFI_PROCFS)
    else if (dump_en) {
        siwifi_hw->debugfs.trace_prst = true;
        siwifi_trigger_um_helper(&siwifi_hw->debugfs);
    }
#endif
#endif
}
#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
void siwifi_umh_done(struct siwifi_hw *siwifi_hw)
{
    if (!test_bit(SIWIFI_DEV_STARTED, &siwifi_hw->drv_flags))
        return;
    siwifi_hw->debugfs.trace_prst = false;
#ifdef CFG_DBGDUMP
    ipc_host_dbginfobuf_push(siwifi_hw->ipc_env, siwifi_hw->dbgdump_elem.buf.dma_addr);
#endif
}
#endif
void siwifi_ipc_rxbuf_elem_fail_check(struct siwifi_hw *siwifi_hw)
{
    while (siwifi_hw->rx_skb_alloc_fail_cnt) {
        siwifi_hw->rx_skb_alloc_fail_cnt--;
        if (siwifi_ipc_rxbuf_elem_allocs(siwifi_hw)) {
            printk("rx_skb_alloc_fail_check still cannot alloc skb! \n");
            break;
        }
    }
}
void siwifi_sta_hash_add(struct siwifi_vif *vif, struct siwifi_sta *sta)
{
 int hash_idx = STA_HASH(sta->mac_addr);
 if (vif->sta_hash[hash_idx]) {
  struct siwifi_sta *prev_sta = (struct siwifi_sta *)(vif->sta_hash[hash_idx]);
  vif->sta_hash[hash_idx] = (uint32_t)sta;
  sta->hnext = prev_sta;
 } else {
  sta->hnext = NULL;
  vif->sta_hash[hash_idx] = (uint32_t)sta;
 }
 printk("add sta(%pM) to hash table %d!\n", sta->mac_addr, hash_idx);
}
void siwifi_sta_hash_del(struct siwifi_vif *vif, struct siwifi_sta *sta)
{
    struct siwifi_sta *s;
    s = (struct siwifi_sta *)(vif->sta_hash[STA_HASH(sta->mac_addr)]);
    if (s == NULL) return;
    if (ether_addr_equal(s->mac_addr, sta->mac_addr)) {
        vif->sta_hash[STA_HASH(sta->mac_addr)] = (uint32_t)s->hnext;
        printk("remove STA %pM from hash table success\n", sta->mac_addr);
        return;
    }
    while (s->hnext != NULL && !ether_addr_equal(s->hnext->mac_addr, sta->mac_addr))
        s = s->hnext;
    if (s->hnext != NULL) {
        s->hnext = s->hnext->hnext;
        printk("remove STA %pM from hash table success\n", sta->mac_addr);
 } else {
        printk("could not remove STA %pM from hash table\n", sta->mac_addr);
 }
}
struct siwifi_sta *siwifi_sta_hash_get(struct siwifi_vif *vif, const u8 *addr)
{
 struct siwifi_sta *s = NULL;
 s = (struct siwifi_sta *)(vif->sta_hash[STA_HASH(addr)]);
 while (s != NULL && !ether_addr_equal(s->mac_addr, addr))
  s = s->hnext;
 return s;
}
void siwifi_src_filter_hash_del(struct siwifi_vif *vif, unsigned char *src_mac)
{
    struct siwifi_src_filter *src_filter = NULL;
    struct siwifi_src_filter *src_filter_tmp = NULL;
    src_filter = (struct siwifi_src_filter *)(vif->src_filter_hash[STA_HASH(src_mac)]);
    if (!src_filter) {
        return;
    }
    if (ether_addr_equal(src_filter->src_macaddr, src_mac)) {
        vif->src_filter_hash[STA_HASH(src_mac)] = (uint32_t)src_filter->next;
        siwifi_kfree(src_filter);
        return;
    }
    while (src_filter->next && !ether_addr_equal(src_filter->next->src_macaddr, src_mac))
        src_filter = src_filter->next;
    if (src_filter->next) {
        src_filter_tmp = src_filter->next;
        src_filter->next = src_filter->next->next;
        siwifi_kfree(src_filter_tmp);
    } else {
        printk("fail del src filter %pM\n", src_mac);
    }
    return;
}
void src_filter_aging(struct timer_list *timer)
{
    struct siwifi_vif *siwifi_vif = from_timer(siwifi_vif, timer, src_filter_timer);
    struct siwifi_src_filter *src_filter = NULL;
    struct siwifi_src_filter *src_filter_tmp = NULL;
    int i;
    spin_lock_bh(&siwifi_vif->src_filter_lock);
    for (i = 0; i < STA_HASH_SIZE; i++) {
        src_filter = (struct siwifi_src_filter *)(siwifi_vif->src_filter_hash[i]);
        while (src_filter) {
            src_filter_tmp = src_filter->next;
            if (time_after_eq(jiffies, src_filter->last_use + SRC_FILTER_AGING_TIME * HZ)) {
                siwifi_src_filter_hash_del(siwifi_vif, src_filter->src_macaddr);
            }
            src_filter = src_filter_tmp;
        }
    }
    spin_unlock_bh(&siwifi_vif->src_filter_lock);
    mod_timer(&(siwifi_vif->src_filter_timer), jiffies + SRC_FILTER_AGING_TIME * HZ);
}
void siwifi_src_filter_hash_free(struct siwifi_vif *siwifi_vif)
{
    struct siwifi_src_filter *src_filter = NULL;
    struct siwifi_src_filter *src_filter_tmp = NULL;
    int i;
    if (siwifi_vif == NULL)
        return;
    if (SIWIFI_VIF_TYPE(siwifi_vif) != NL80211_IFTYPE_STATION)
        return;
    spin_lock_bh(&siwifi_vif->src_filter_lock);
    for (i = 0; i < STA_HASH_SIZE; i++) {
        src_filter = (struct siwifi_src_filter *)(siwifi_vif->src_filter_hash[i]);
        while (src_filter) {
            src_filter_tmp = src_filter->next;
            siwifi_src_filter_hash_del(siwifi_vif, src_filter->src_macaddr);
            src_filter = src_filter_tmp;
        }
    }
    printk("free src filter hash table\n");
    spin_unlock_bh(&siwifi_vif->src_filter_lock);
    return;
}
struct siwifi_src_filter *siwifi_src_filter_hash_search(struct siwifi_vif *siwifi_vif, unsigned char *src_mac)
{
    struct siwifi_src_filter *src_filter = NULL;
    if (siwifi_vif == NULL || src_mac == NULL)
        return NULL;
    if (SIWIFI_VIF_TYPE(siwifi_vif) != NL80211_IFTYPE_STATION)
        return NULL;
    spin_lock_bh(&siwifi_vif->src_filter_lock);
    src_filter = (struct siwifi_src_filter *)(siwifi_vif->src_filter_hash[STA_HASH(src_mac)]);
    while (src_filter != NULL && !ether_addr_equal(src_filter->src_macaddr, src_mac))
        src_filter = src_filter->next;
    spin_unlock_bh(&siwifi_vif->src_filter_lock);
    return src_filter;
}
void siwifi_update_src_filter(struct siwifi_vif *siwifi_vif, unsigned char *src_mac)
{
    int hash_idx = STA_HASH(src_mac);
    struct siwifi_src_filter *src_filter = NULL;
    struct siwifi_src_filter *src_filter_prev = NULL;
    if (siwifi_vif == NULL || src_mac == NULL)
        return;
    if (SIWIFI_VIF_TYPE(siwifi_vif) != NL80211_IFTYPE_STATION)
        return;
    if ((src_filter = siwifi_src_filter_hash_search(siwifi_vif, src_mac))) {
        src_filter->last_use = jiffies;
        return;
    }
    src_filter = (struct siwifi_src_filter *)siwifi_kzalloc(sizeof(struct siwifi_src_filter), GFP_ATOMIC);
    if (src_filter)
        memcpy(src_filter->src_macaddr, src_mac, ETH_ALEN);
    else
        return;
    spin_lock_bh(&siwifi_vif->src_filter_lock);
    if (siwifi_vif->src_filter_hash[hash_idx]) {
        src_filter_prev = (struct siwifi_src_filter *)(siwifi_vif->src_filter_hash[hash_idx]);
        siwifi_vif->src_filter_hash[hash_idx] = (uint32_t)src_filter;
        src_filter->next = src_filter_prev;
    } else {
        src_filter->next = NULL;
        siwifi_vif->src_filter_hash[hash_idx] = (uint32_t)src_filter;
    }
    src_filter->last_use = jiffies;
    spin_unlock_bh(&siwifi_vif->src_filter_lock);
    return;
}
bool siwifi_check_skb_is_icmp(struct sk_buff *skb)
{
    struct ethhdr *eth;
    struct iphdr *ipdata;
    eth = (struct ethhdr *)(skb->data);
    if (skb->len > sizeof(struct ethhdr) + 19) {
        if (eth->h_proto == htons(ETH_P_IP))
        {
            ipdata = (struct iphdr *)((char *)eth + sizeof(struct ethhdr));
            if (ipdata->version == 4) {
                if (ipdata->protocol == 1) {
                    return true;
                }
            }
        }
    }
    return false;
}
int siwifi_check_skb_is_dhcp(struct sk_buff *skb)
{
    struct ethhdr *ethhdr = NULL;
    struct iphdr *iphdr = NULL;
    struct ipv6hdr *ipv6hdr = NULL;
    struct udphdr *udphdr = NULL;
    __be16 proto;
    unsigned int header_len = 0;
    ethhdr = (struct ethhdr *)skb->data;
    proto = ethhdr->h_proto;
    header_len += ETH_HLEN;
    switch (proto) {
        case htons(ETH_P_IP):
            if (!pskb_may_pull(skb, header_len + sizeof(*iphdr)))
                return 0;
            iphdr = (struct iphdr *)(skb->data + header_len);
            header_len += iphdr->ihl * 4;
            if (iphdr->protocol != IPPROTO_UDP)
                return 0;
            break;
        case htons(ETH_P_IPV6):
            if (!pskb_may_pull(skb, header_len + sizeof(*ipv6hdr)))
                return 0;
            ipv6hdr = (struct ipv6hdr *)(skb->data + header_len);
            header_len += sizeof(*ipv6hdr);
            if (ipv6hdr->nexthdr != IPPROTO_UDP)
                return 0;
            break;
        default:
            return 0;
    }
    if (!pskb_may_pull(skb, header_len + sizeof(*udphdr)))
        return 0;
    udphdr = (struct udphdr *)(skb->data + header_len);
    header_len += sizeof(*udphdr);
    switch (proto) {
        case htons(ETH_P_IP):
            if (udphdr->dest == htons(67) || udphdr->source == htons(67))
                return 1;
            break;
        case htons(ETH_P_IPV6):
            if (udphdr->dest == htons(547) || udphdr->source == htons(547))
                return 1;
            break;
    }
    return 0;
}
void set_repeater_status(struct siwifi_vif *siwifi_vif, u8 status)
{
    if (SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_STATION)
        siwifi_vif->repeater_status = status;
}
int siwifi_set_assoc_req_insert_info(struct siwifi_hw *siwifi_hw, char *insert_info, int insert_length)
{
    u32_l dma_addr;
    if (!siwifi_hw || !insert_info || !insert_length)
        return 0;
    if (siwifi_hw->assoc_req_insert_info)
        siwifi_kfree(siwifi_hw->assoc_req_insert_info);
    siwifi_hw->assoc_req_insert_info = (char *)siwifi_kzalloc(insert_length, GFP_KERNEL);
    if (!siwifi_hw->assoc_req_insert_info)
        return 0;
    memcpy(siwifi_hw->assoc_req_insert_info, insert_info, insert_length);
    siwifi_hw->assoc_insert.info_dmalength = insert_length;
    dma_addr = dma_map_single(siwifi_hw->dev, siwifi_hw->assoc_req_insert_info, siwifi_hw->assoc_insert.info_dmalength, DMA_BIDIRECTIONAL);
    if (dma_mapping_error(siwifi_hw->dev, dma_addr))
        return 0;
    siwifi_hw->assoc_insert.info_dmaaddr = dma_addr;
    siwifi_send_assoc_req_insert_info(siwifi_hw);
    return 1;
}
extern int siwifi_send_get_channel_info_req(struct siwifi_hw *siwifi_hw, struct mm_get_channel_info_cfm *cfm);
extern uint16_t siwifi_freq_to_channel(uint16_t freq);
int siwifi_channel_recovery_check(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_vif *vif = NULL;
    struct siwifi_vif *vif_sta = NULL;
    uint8_t channel_idx = 0;
    int channel_freq = 0;
    enum nl80211_chan_width channel_width = NL80211_CHAN_WIDTH_20;
    struct mm_get_channel_info_cfm cfm;
    struct siwifi_bcn *bcn = NULL;
    const uint8_t *ie_ds = NULL;
    uint8_t ie_offset = 0;
    uint8_t channel_num = 0;
    list_for_each_entry(vif, &siwifi_hw->vifs, list)
    {
        if (vif->up && (SIWIFI_VIF_TYPE(vif) == NL80211_IFTYPE_STATION))
        {
            vif_sta = vif;
            break;
        }
    }
    if (!vif_sta) {
        return 0;
    }
    memset(&cfm, 0, sizeof(struct mm_get_channel_info_cfm));
    if (!siwifi_send_get_channel_info_req(siwifi_hw, &cfm)) {
        channel_idx = cfm.channel_idx;
        channel_freq = cfm.channel_freq;
        switch (cfm.channel_width) {
            case 0:
                channel_width = NL80211_CHAN_WIDTH_20;
                break;
            case 1:
                channel_width = NL80211_CHAN_WIDTH_40;
                break;
            case 2:
                channel_width = NL80211_CHAN_WIDTH_80;
                break;
        }
    } else {
        printk("send siwifi_send_get_channel_info_req fail!\n");
        return 0;
    }
    spin_lock_bh(&siwifi_hw->cb_lock);
    if (!(siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.chan)) {
        printk("vif_sta %s [ch_idx %d].chan_def.chan is NULL\n", vif_sta->ndev->name, vif_sta->ch_index);
        spin_unlock_bh(&siwifi_hw->cb_lock);
        return 0;
    }
    if (channel_freq != siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.chan->center_freq ||
            channel_width != siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.width ||
            channel_idx != vif_sta->ch_index) {
        struct cfg80211_chan_def chandef;
        struct ieee80211_channel *chan;
        printk("ERROR: ch_freq %d width %d idx %d is different from vif_sta(%s) freq %d width %d idx %d \n",
                channel_freq, channel_width, channel_idx,
                vif_sta->ndev->name,
                siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.chan->center_freq,
                siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.width,
                vif_sta->ch_index);
        if(!channel_freq || (channel_idx == 255) ||
                !(siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.chan->center_freq)
                || (vif_sta->ch_index == 255))
        {
            spin_unlock_bh(&siwifi_hw->cb_lock);
            return 0;
        }
        siwifi_chanctx_unlink(vif_sta);
        chan = ieee80211_get_channel(siwifi_hw->wiphy, channel_freq);
        cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);
        if (!siwifi_hw->mod_params->ht_on)
            chandef.width = NL80211_CHAN_WIDTH_20_NOHT;
        else
            chandef.width = channel_width;
        chandef.center_freq1 = cfm.center_freq1;
        chandef.center_freq2 = cfm.center_freq2;
        siwifi_chanctx_link(vif_sta, channel_idx, &chandef);
    }
    list_for_each_entry(vif, &siwifi_hw->vifs, list)
    {
        struct siwifi_chanctx *ctxt_sta = NULL;
        uint8_t vif_sta_channel_num = 0;
        enum nl80211_chan_width vif_sta_channel_width = NL80211_CHAN_WIDTH_20;
        enum nl80211_chan_width vif_channel_width = NL80211_CHAN_WIDTH_20;
        if (!vif->up || (SIWIFI_VIF_TYPE(vif) != NL80211_IFTYPE_AP))
            continue;
        if (!siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.chan)
            continue;
        vif_sta_channel_num = siwifi_freq_to_channel(siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.chan->center_freq);
        vif_sta_channel_width = siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def.width;
        ctxt_sta = &vif_sta->siwifi_hw->chanctx_table[vif_sta->ch_index];
        vif_channel_width = siwifi_hw->chanctx_table[vif->ch_index].chan_def.width;
        bcn = &vif->ap.bcn;
        ie_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
        ie_ds = cfg80211_find_ie(WLAN_EID_DS_PARAMS, bcn->head + ie_offset, bcn->head_len - ie_offset);
        if (ie_ds) {
            channel_num = *(ie_ds + 2);
            if (channel_num != vif_sta_channel_num || vif_channel_width != vif_sta_channel_width) {
                printk("ERROR:vif(%s) hostapd beacon channel %d width %dis diff from channel %d width %d\n",
                        vif->ndev->name, channel_num, vif_channel_width, vif_sta_channel_num, vif_sta_channel_width);
                cfg80211_ch_switch_notify(vif->ndev, &siwifi_hw->chanctx_table[vif_sta->ch_index].chan_def, 0);
                siwifi_chanctx_unlink(vif);
                vif->ch_index = vif_sta->ch_index;
                ctxt_sta->count++;
                siwifi_txq_vif_start(vif, SIWIFI_TXQ_STOP_CHAN, vif->siwifi_hw);
            }
        }
    }
    spin_unlock_bh(&siwifi_hw->cb_lock);
    return 1;
}
