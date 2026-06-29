#ifndef _IPC_SHARED_H_
#define _IPC_SHARED_H_ 
#include "ipc_compat.h"
#include "lmac_types.h"
#include "lmac_mac.h"
#include "lmac_msg.h"
#define CO_BIT(pos) (1U<<(pos))
#define IPC_TXQUEUE_CNT NX_TXQ_CNT
#ifdef CONFIG_WIFI_LITE_MEMORY
#define IPC_RXBUF_CNT 192
#define RX_LB_RXSKB_CNT 128
#elif defined(CFG_DEAGG)
#define IPC_RXBUF_CNT 704
#define RX_LB_RXSKB_CNT 512
#else
#define IPC_RXBUF_CNT 128
#define RX_LB_RXSKB_CNT IPC_RXBUF_CNT
#endif
#define RX_HB_RXSKB_CNT IPC_RXBUF_CNT
#ifdef CFG_DEAGG
#define IPC_RXDESC_CNT 704
#else
#define IPC_RXDESC_CNT 128
#endif
#ifdef CFG_DEAGG
#define RX_AMSDU_CNT_MAX CONFIG_SIWIFI_MAX_MSDU_PER_RX
#define RX_AMSDU_MAX_SKB_SIZE 2560
#endif
#define IPC_RADARBUF_CNT 16
#define IPC_UNSUPRXVECBUF_CNT 8
#define IPC_RXVEC_SIZE 16
#define IPC_MSGE2A_BUF_CNT 64
#ifdef CONFIG_WIFI_LITE_MEMORY
#define IPC_DBGBUF_CNT 32
#else
#define IPC_DBGBUF_CNT 32
#endif
#define IPC_A2E_MSG_BUF_SIZE 127
#define IPC_E2A_MSG_PARAM_SIZE 256
#ifdef CONFIG_WIFI_LITE_MEMORY
#define IPC_DBG_PARAM_SIZE 128
#else
#define IPC_DBG_PARAM_SIZE 256
#endif
#define RX_DMA_OVER_PATTERN 0xAAAAAA00
#define IPC_MSGE2A_VALID_PATTERN 0xADDEDE2A
#define IPC_DBG_VALID_PATTERN 0x000CACA0
#define DMA_HDR_PHYVECT_LEN 36
#define NX_TX_PAYLOAD_MAX 6
#define MSG_API_VER 5
#define IPC_TXDESC_CONFIRMED 0xAAAA
#define IPC_TXDESC_READY 0xFFFF
#define IPC_TXDESC_FREE 0x0000
struct hostdesc
{
#ifdef CONFIG_SIWIFI_SPLIT_TX_BUF
    u32_l packet_addr[NX_TX_PAYLOAD_MAX];
    u16_l packet_len[NX_TX_PAYLOAD_MAX];
    u8_l packet_cnt;
#else
    u32_l packet_addr;
    u16_l packet_len;
#endif
    u32_l status_desc_addr;
    struct mac_addr eth_dest_addr;
    struct mac_addr eth_src_addr;
    u16_l ethertype;
    u16_l pn[4];
    u16_l sn;
    u16_l timestamp;
    u8_l tid;
    u8_l vif_idx;
    u8_l staid;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
    u8_l mumimo_info;
#endif
    u16_l flags;
};
struct umacdesc
{
#ifdef CONFIG_SIWIFI_AGG_TX
    u16_l sn_win;
    u32_l flags;
    u32_l phy_flags;
#endif
};
struct txdesc_api
{
    struct hostdesc host;
};
struct txdesc_host
{
    u16_l ready;
    u16_l wmm_param;
#ifdef NEW_SCHEDULE
    u32_l length;
    u32_l phy_addr;
#else
    struct txdesc_api api;
#endif
};
struct dma_desc
{
    u32_l src;
    u32_l dest;
    u16_l length;
    u16_l ctrl;
    u32_l next;
};
#define LA_CONF_LEN 10
struct la_conf_tag
{
    u32_l conf[LA_CONF_LEN];
    u32_l trace_len;
    u32_l diag_conf;
 u32_l diag_conf1;
};
#ifdef CONFIG_SF16A18_WIFI_LB_LA_ENABLE
#define LA_MEM_LEN (192 * 1024)
#endif
#ifdef CONFIG_SF16A18_WIFI_HB_LA_ENABLE
#define LA_MEM_LEN (128 * 1024)
#endif
enum
{
    DBG_ERROR_RECOVERABLE = 0,
    DBG_ERROR_FATAL,
 DBG_ERROR_RFCMD_FAIL
};
#define DBG_SW_DIAG_MAX_LEN 1024
#define DBG_ERROR_TRACE_SIZE 256
#define DBG_DIAGS_MAC_MAX 48
#define DBG_DIAGS_PHY_MAX 32
#define DBG_RHD_MEM_LEN (5 * 1024)
#define DBG_RBD_MEM_LEN (5 * 1024)
#define DBG_RF_INFO_SIZE (1 * 1024)
#define DBG_THD_MEM_LEN (10 * 1024)
struct phy_channel_info
{
    u32_l info1;
    u32_l info2;
};
struct dbg_debug_info_tag
{
    u32_l error_type;
    u32_l rhd;
    u32_l rhd_len;
    u32_l rbd;
    u32_l rbd_len;
    u32_l thd[NX_TXQ_CNT];
    u32_l thd_len[NX_TXQ_CNT];
    u32_l mac_cur_diag;
    u32_l hdm_cur_diag;
    u32_l riu_cur_diag;
    u32_l error[DBG_ERROR_TRACE_SIZE/4];
    u32_l sw_diag_len;
    u32_l sw_diag[DBG_SW_DIAG_MAX_LEN/4];
    struct phy_channel_info chan_info;
    u32_l rf_diag[DBG_RF_INFO_SIZE/4];
    u32_l rf_diag_len;
    struct la_conf_tag la_conf;
    u16_l diags_mac[DBG_DIAGS_MAC_MAX];
    u16_l diags_phy[DBG_DIAGS_PHY_MAX];
    u32_l rhd_hw_ptr;
    u32_l rbd_hw_ptr;
};
struct dbg_debug_dump_tag
{
    struct dbg_debug_info_tag dbg_info;
    u32_l rhd_mem[DBG_RHD_MEM_LEN/4];
    u32_l rbd_mem[DBG_RBD_MEM_LEN/4];
    u32_l thd_mem[NX_TXQ_CNT][DBG_THD_MEM_LEN/4];
#ifdef CFG_LA_ENABLE
 u32_l la_mem_valid;
 u32_l la_mem_dma_addr;
#endif
};
#ifdef CONFIG_SIWIFI_RADAR
#define RADAR_PULSE_MAX 4
struct radar_pulse_array_desc
{
    struct phy_radar_pulse pulse[RADAR_PULSE_MAX];
    u32_l idx;
    u32_l cnt;
};
struct radar_pulse {
    s32_l freq:6;
    u32_l fom:4;
    u32_l len:6;
    u32_l rep:16;
};
#endif
struct rx_vector_desc
{
    struct phy_channel_info phy_info;
    u32_l rx_vect1[IPC_RXVEC_SIZE/4];
    u32_l pattern;
};
struct rxdesc_tag
{
    u32_l host_id;
    u32_l frame_len;
    u16_l status;
    u16_l amsdu_num;
};
struct ipc_header
{
    u16_l type;
    u16_l size;
};
struct ipc_msg_elt
{
    struct ipc_header header __ALIGN4;
};
struct ipc_e2a_msg
{
    u16_l id;
    u16_l dummy_dest_id;
    u16_l dummy_src_id;
    u16_l param_len;
    u32_l param[IPC_E2A_MSG_PARAM_SIZE];
    u32_l pattern;
};
struct ipc_dbg_msg
{
    u32_l string[IPC_DBG_PARAM_SIZE/4];
    u32_l pattern;
};
struct ipc_a2e_msg
{
    u32_l dummy_word;
    u32_l msg[IPC_A2E_MSG_BUF_SIZE];
};
struct ipc_shared_rx_buf
{
    u32_l hostid;
    u32_l dma_addr;
};
struct ipc_shared_rx_desc
{
    u32_l dma_addr;
};
struct compatibility_tag {
    u16_l ipc_shared_size;
    u16_l msg_api;
    u8_l ipc_shared_version;
    u8_l msge2a_buf_cnt;
    u8_l dbgbuf_cnt;
    u8_l radarbuf_cnt;
    u8_l unsuprxvecbuf_cnt;
    u16_l rxdesc_cnt;
    u16_l rxbuf_cnt;
    u8_l bk_txq;
    u8_l be_txq;
    u8_l vi_txq;
    u8_l vo_txq;
    u8_l bcn_txq;
    uint32_t fexception_base;
};
enum {
    DEEP_DEBUG_LA = (1 << 0),
    DEEP_DEBUG_IQDUMP = (1 << 1),
    DEEP_DEBUG_MAX
};
#define MAX_DBG_EVT_CNT 20
struct siwifi_firmware_debug_info
{
    volatile uint32_t lmac_irq_running;
    volatile uint32_t lmac_irq_count;
    volatile uint32_t lmac_irq;
    volatile uint32_t lmac_evt_running;
    volatile uint32_t lmac_evt_count;
    volatile uint32_t lmac_evt;
    volatile uint32_t lmac_msg_id;
    volatile uint32_t lmac_dest_id;
    volatile uint32_t lmac_src_id;
    volatile uint32_t lmac_evt_record[MAX_DBG_EVT_CNT];
    volatile uint32_t lmac_evt_idx;
};
struct ipc_shared_env_tag
{
    volatile struct compatibility_tag comp_info;
    volatile struct ipc_a2e_msg msg_a2e_buf;
#ifndef CONFIG_SIWIFI_IRQ_OPT
    volatile struct ipc_e2a_msg msg_e2a_buf;
    volatile struct dma_desc msg_dma_desc;
#endif
    volatile u32_l msg_e2a_hostbuf_addr [IPC_MSGE2A_BUF_CNT];
#ifndef CONFIG_SIWIFI_IRQ_OPT
    volatile struct ipc_dbg_msg dbg_buf;
    volatile struct dma_desc dbg_dma_desc;
#endif
    volatile u32_l dbg_hostbuf_addr [IPC_DBGBUF_CNT];
    volatile u32_l la_dbginfo_addr;
    volatile u32_l pattern_addr;
    volatile u32_l dying_gasp_addr;
#ifdef CONFIG_SIWIFI_RADAR
    volatile u32_l radarbuf_hostbuf [IPC_RADARBUF_CNT];
#endif
    volatile u32_l unsuprxvecbuf_hostbuf [IPC_UNSUPRXVECBUF_CNT];
    volatile struct txdesc_host txdesc0[CONFIG_USER_MAX][NX_TXDESC_CNT0];
    volatile struct txdesc_host txdesc1[CONFIG_USER_MAX][NX_TXDESC_CNT1];
    volatile struct txdesc_host txdesc2[CONFIG_USER_MAX][NX_TXDESC_CNT2];
    volatile struct txdesc_host txdesc3[CONFIG_USER_MAX][NX_TXDESC_CNT3];
    #if NX_TXQ_CNT == 5
    volatile struct txdesc_host txdesc4[1][NX_TXDESC_CNT4];
    #endif
    volatile struct ipc_shared_rx_desc host_rxdesc[IPC_RXDESC_CNT];
    volatile struct ipc_shared_rx_buf host_rxbuf[IPC_RXBUF_CNT];
    volatile uint16_t trace_pattern;
    volatile uint32_t trace_start;
    volatile uint32_t trace_end;
    volatile uint32_t trace_size;
    volatile uint32_t trace_offset;
    volatile uint32_t trace_nb_compo;
    volatile uint32_t trace_offset_compo;
    volatile uint32_t user_data;
    volatile uint32_t user_reply;
    volatile struct siwifi_firmware_debug_info debug_info;
};
extern struct ipc_shared_env_tag ipc_shared_env;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
#if CONFIG_USER_MAX > 3
#define IPC_IRQ_A2E_USER_MSK 0xF
#elif CONFIG_USER_MAX > 2
#define IPC_IRQ_A2E_USER_MSK 0x7
#else
#define IPC_IRQ_A2E_USER_MSK 0x3
#endif
#define IPC_IRQ_A2E_AC0_OFT 8
#define IPC_IRQ_A2E_AC0_MSK (IPC_IRQ_A2E_USER_MSK << IPC_IRQ_A2E_AC0_OFT)
#define IPC_IRQ_A2E_AC1_OFT (IPC_IRQ_A2E_AC0_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_A2E_AC1_MSK (IPC_IRQ_A2E_USER_MSK << IPC_IRQ_A2E_AC1_OFT)
#define IPC_IRQ_A2E_AC2_OFT (IPC_IRQ_A2E_AC1_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_A2E_AC2_MSK (IPC_IRQ_A2E_USER_MSK << IPC_IRQ_A2E_AC2_OFT)
#define IPC_IRQ_A2E_AC3_OFT (IPC_IRQ_A2E_AC2_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_A2E_AC3_MSK (IPC_IRQ_A2E_USER_MSK << IPC_IRQ_A2E_AC3_OFT)
#define IPC_IRQ_A2E_BCN_OFT (IPC_IRQ_A2E_AC3_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_A2E_BCN_MSK CO_BIT(IPC_IRQ_A2E_BCN_OFT)
#define IPC_IRQ_A2E_AC_TXDESC (IPC_IRQ_A2E_AC0_MSK | IPC_IRQ_A2E_AC1_MSK | \
                                   IPC_IRQ_A2E_AC2_MSK | IPC_IRQ_A2E_AC3_MSK)
#if NX_TXQ_CNT < 5
#define IPC_IRQ_A2E_BCN_TXDESC 0
#else
#define IPC_IRQ_A2E_BCN_TXDESC (0x01 << IPC_IRQ_A2E_BCN_OFT)
#endif
#define IPC_IRQ_A2E_TXDESC (IPC_IRQ_A2E_AC_TXDESC | IPC_IRQ_A2E_BCN_TXDESC)
#else
#define IPC_IRQ_A2E_TXDESC 0xFF00
#endif
#define IPC_IRQ_A2E_TXDESC_FIRSTBIT (8)
#define IPC_IRQ_A2E_RXBUF_BACK CO_BIT(5)
#define IPC_IRQ_A2E_RXDESC_BACK CO_BIT(4)
#define IPC_IRQ_A2E_MSG CO_BIT(1)
#define IPC_IRQ_A2E_DBG CO_BIT(0)
#define IPC_IRQ_A2E_ALL (IPC_IRQ_A2E_TXDESC|IPC_IRQ_A2E_MSG|IPC_IRQ_A2E_DBG)
#define IPC_IRQ_E2A_TXCFM_POS 7
#ifdef CONFIG_SIWIFI_MUMIMO_TX
#if CONFIG_USER_MAX > 3
#define IPC_IRQ_E2A_USER_MSK 0xF
#elif CONFIG_USER_MAX > 2
#define IPC_IRQ_E2A_USER_MSK 0x7
#else
#define IPC_IRQ_E2A_USER_MSK 0x3
#endif
#define IPC_IRQ_E2A_AC0_OFT IPC_IRQ_E2A_TXCFM_POS
#define IPC_IRQ_E2A_AC0_MSK (IPC_IRQ_E2A_USER_MSK << IPC_IRQ_E2A_AC0_OFT)
#define IPC_IRQ_E2A_AC1_OFT (IPC_IRQ_E2A_AC0_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_E2A_AC1_MSK (IPC_IRQ_E2A_USER_MSK << IPC_IRQ_E2A_AC1_OFT)
#define IPC_IRQ_E2A_AC2_OFT (IPC_IRQ_E2A_AC1_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_E2A_AC2_MSK (IPC_IRQ_E2A_USER_MSK << IPC_IRQ_E2A_AC2_OFT)
#define IPC_IRQ_E2A_AC3_OFT (IPC_IRQ_E2A_AC2_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_E2A_AC3_MSK (IPC_IRQ_E2A_USER_MSK << IPC_IRQ_E2A_AC3_OFT)
#define IPC_IRQ_E2A_BCN_OFT (IPC_IRQ_E2A_AC3_OFT + CONFIG_USER_MAX)
#define IPC_IRQ_E2A_BCN_MSK CO_BIT(IPC_IRQ_E2A_BCN_OFT)
#define IPC_IRQ_E2A_AC_TXCFM (IPC_IRQ_E2A_AC0_MSK | IPC_IRQ_E2A_AC1_MSK | \
                                   IPC_IRQ_E2A_AC2_MSK | IPC_IRQ_E2A_AC3_MSK)
#if NX_TXQ_CNT < 5
#define IPC_IRQ_E2A_BCN_TXCFM 0
#else
#define IPC_IRQ_E2A_BCN_TXCFM (0x01 << IPC_IRQ_E2A_BCN_OFT)
#endif
#define IPC_IRQ_E2A_TXCFM (IPC_IRQ_E2A_AC_TXCFM | IPC_IRQ_E2A_BCN_TXCFM)
#else
#define IPC_IRQ_E2A_TXCFM ((1 << NX_TXQ_CNT) - 1 ) << IPC_IRQ_E2A_TXCFM_POS
#endif
#define IPC_IRQ_E2A_UNSUP_RX_VEC CO_BIT(7)
#define IPC_IRQ_E2A_RADAR CO_BIT(6)
#define IPC_IRQ_E2A_TBTT_SEC CO_BIT(5)
#define IPC_IRQ_E2A_TBTT_PRIM CO_BIT(4)
#define IPC_IRQ_E2A_RXDESC CO_BIT(3)
#define IPC_IRQ_E2A_MSG_ACK CO_BIT(2)
#define IPC_IRQ_E2A_MSG CO_BIT(1)
#define IPC_IRQ_E2A_DBG CO_BIT(0)
#define IPC_IRQ_E2A_ALL ( IPC_IRQ_E2A_TXCFM \
                                | IPC_IRQ_E2A_RXDESC \
                                | IPC_IRQ_E2A_MSG_ACK \
                                | IPC_IRQ_E2A_MSG \
                                | IPC_IRQ_E2A_DBG \
                                | IPC_IRQ_E2A_TBTT_PRIM \
                                | IPC_IRQ_E2A_TBTT_SEC \
                                | IPC_IRQ_E2A_RADAR \
                                | IPC_IRQ_E2A_UNSUP_RX_VEC)
#define IPC_RX_FORWARD CO_BIT(1)
#define IPC_RX_INTRABSS CO_BIT(0)
enum
{
    IPC_MSG_NONE = 0,
    IPC_MSG_WRAP,
    IPC_MSG_KMSG,
    IPC_DBG_STRING,
};
#endif
