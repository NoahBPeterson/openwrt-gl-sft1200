#ifndef _IPC_HOST_H_
#define _IPC_HOST_H_ 
#include "ipc_shared.h"
#ifndef __KERNEL__
#include "arch.h"
#else
#include "ipc_compat.h"
#endif
enum ipc_host_desc_status
{
    IPC_HOST_DESC_IDLE = 0,
    IPC_HOST_DESC_FORWARD,
    IPC_HOST_DESC_KEEP,
    IPC_HOST_DESC_DELETE,
    IPC_HOST_DESC_LEN_UPDATE,
};
struct ipc_host_cb_tag
{
#ifdef NEW_SCHEDULE
    int (*send_data_cfm_burst)(void *pthis, void *host_id, uint32_t burst_length);
#else
    int (*send_data_cfm)(void *pthis, void *host_id);
#endif
    uint8_t (*recv_data_ind)(void *pthis, void *host_id);
    uint8_t (*recv_radar_ind)(void *pthis, void *host_id);
    uint8_t (*recv_unsup_rx_vec_ind)(void *pthis, void *host_id);
    uint8_t (*recv_msg_ind)(void *pthis, void *host_id);
    uint8_t (*recv_msgack_ind)(void *pthis, void *host_id);
    uint8_t (*recv_dbg_ind)(void *pthis, void *host_id);
    void (*prim_tbtt_ind)(void *pthis);
    void (*sec_tbtt_ind)(void *pthis);
};
struct ipc_hostbuf
{
    void *hostid;
    uint32_t dma_addr;
};
struct siwifi_mod_params;
struct ipc_host_env_tag
{
    struct ipc_host_cb_tag cb;
    void* reg_base;
    struct ipc_shared_env_tag *shared;
    struct ipc_hostbuf ipc_host_rxdesc_array[IPC_RXDESC_CNT];
    uint16_t ipc_host_rxdesc_idx;
    uint16_t rxdesc_nb;
    uint16_t ipc_host_rxbuf_idx;
    uint32_t rx_bufnb;
    uint32_t rx_bufsz;
    uint32_t rx_pd_offset;
    uint32_t max_rx_process_time;
    uint32_t ipc_pending_status;
#ifdef CONFIG_SIWIFI_RADAR
    struct ipc_hostbuf ipc_host_radarbuf_array[IPC_RADARBUF_CNT];
    uint8_t ipc_host_radarbuf_idx;
    uint32_t radar_bufnb;
    uint32_t radar_bufsz;
#endif
    struct siwifi_mod_params *mod_params;
    struct ipc_hostbuf ipc_host_unsuprxvecbuf_array[IPC_UNSUPRXVECBUF_CNT];
    uint8_t ipc_host_unsuprxvecbuf_idx;
    uint32_t unsuprxvec_bufnb;
    uint32_t unsuprxvec_bufsz;
    uint32_t txdesc_free_idx[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    uint32_t txdesc_used_idx[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    void *tx_host_id0[CONFIG_USER_MAX][NX_TXDESC_CNT0];
    void *tx_host_id1[CONFIG_USER_MAX][NX_TXDESC_CNT1];
    void *tx_host_id2[CONFIG_USER_MAX][NX_TXDESC_CNT2];
    void *tx_host_id3[CONFIG_USER_MAX][NX_TXDESC_CNT3];
    #if NX_TXQ_CNT == 5
    void *tx_host_id4[1][NX_TXDESC_CNT4];
    #endif
    void **tx_host_id[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    volatile struct txdesc_host *txdesc[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    struct ipc_hostbuf ipc_host_msgbuf_array[IPC_MSGE2A_BUF_CNT];
    uint8_t ipc_host_msge2a_idx;
    uint32_t ipc_e2amsg_bufnb;
    uint32_t ipc_e2amsg_bufsz;
    uint8_t msga2e_cnt;
    void *msga2e_hostid;
    struct ipc_hostbuf ipc_host_dbgbuf_array[IPC_DBGBUF_CNT];
    uint8_t ipc_host_dbg_idx;
    uint32_t ipc_dbg_bufnb;
    uint32_t ipc_dbg_bufsz;
    void *pthis;
};
extern const int nx_txdesc_cnt[];
extern const int nx_max_skb_txdesc_cnt[];
extern const int nx_txuser_cnt[];
__INLINE bool ipc_host_queue_full(struct ipc_host_env_tag *env,
                                  const int queue_idx)
{
    return (env->txdesc_free_idx[queue_idx] ==
                      (env->txdesc_used_idx[queue_idx] + nx_txdesc_cnt[queue_idx]));
}
void ipc_host_rxdesc_handler(struct ipc_host_env_tag *env);
#ifdef NEW_SCHEDULE
void ipc_host_deinit(struct ipc_host_env_tag *env);
#endif
void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb,
                  struct ipc_shared_env_tag *shared_env_ptr,
                  void *reg_base,
                  void *pthis);
volatile struct txdesc_host *ipc_host_txdesc_get(struct ipc_host_env_tag *env,
                                                 const int queue_idx,
                                                 const int user_pos);
#ifdef NEW_SCHEDULE
void ipc_host_txdesc_map(struct ipc_host_env_tag *env,
                            const int user,
                            const int hwq_index,
                            const int pos,
                            uint32_t phy_addr);
#endif
void ipc_host_txdesc_push(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void *host_id);
void ipc_host_txdesc_push_burst(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void (*store_host_id)(void *target, void *src),
                          void *host_id);
bool ipc_host_tx_frames_pending(struct ipc_host_env_tag *env);
void *ipc_host_tx_flush(struct ipc_host_env_tag *env, const int queue_idx,
                        const int user_pos);
void ipc_host_patt_addr_push(struct ipc_host_env_tag *env, uint32_t addr);
int ipc_host_rxbuf_push(struct ipc_host_env_tag *env,
                        uint32_t hostid,
                        uint32_t hostbuf);
int ipc_host_rxdesc_push(struct ipc_host_env_tag *env, void *hostid,
                         uint32_t hostbuf);
#ifdef CONFIG_SIWIFI_RADAR
int ipc_host_radarbuf_push(struct ipc_host_env_tag *env, void *hostid,
                           uint32_t hostbuf);
#endif
int ipc_host_unsup_rx_vec_buf_push(struct ipc_host_env_tag *env, void *hostid,
                                    uint32_t hostbuf);
int ipc_host_msgbuf_push(struct ipc_host_env_tag *env, void *hostid,
                         uint32_t hostbuf);
int ipc_host_dbgbuf_push(struct ipc_host_env_tag *env, void *hostid,
                         uint32_t hostbuf);
void ipc_host_dbginfobuf_push(struct ipc_host_env_tag *env, uint32_t infobuf);
void ipc_host_dying_gasp_addr_push(struct ipc_host_env_tag *env, uint32_t addr);
void ipc_host_status_clear(struct ipc_host_env_tag *env, uint32_t status);
void ipc_host_irq(struct ipc_host_env_tag *env, uint32_t status);
int ipc_host_msg_push(struct ipc_host_env_tag *env, void *msg_buf, uint16_t len);
void ipc_host_enable_irq(struct ipc_host_env_tag *env, uint32_t value);
void ipc_host_disable_irq(struct ipc_host_env_tag *env, uint32_t value);
uint32_t ipc_host_get_status(struct ipc_host_env_tag *env);
uint32_t ipc_host_get_rawstatus(struct ipc_host_env_tag *env);
int32_t ipc_process_hostids(struct ipc_host_env_tag *env, uint32_t queue_idx,const int user_pos ,
  void (*process_host_id)(void *host_id, uint32_t hwq_index, uint32_t data), uint32_t data);
#endif
