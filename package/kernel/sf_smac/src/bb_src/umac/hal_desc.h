#ifndef _HAL_DESC_H_
#define _HAL_DESC_H_ 
#include "lmac_types.h"
#define N_CCK 8
#define N_OFDM 8
#define N_HT (8 * 2 * 2 * 4)
#define N_VHT (10 * 4 * 2 * 8)
#define N_HE (12 * 4 * 3 * 8)
#define N_RATE (N_CCK + N_OFDM + N_HT + N_VHT + N_HE)
#define __CHBW_CBW20 0
#define __CHBW_CBW40 1
#define __CHBW_CBW80 2
#define __CHBW_CBW160 3
#define FORMATMOD_NON_HT 0
#define FORMATMOD_NON_HT_DUP_OFDM 1
#define FORMATMOD_HT_MF 2
#define FORMATMOD_HT_GF 3
#define FORMATMOD_VHT 4
#define FORMATMOD_HE_SU 5
#define NAV_PROT_NO_PROT_BIT 0
#define NAV_PROT_SELF_CTS_BIT 1
#define NAV_PROT_RTS_CTS_BIT 2
#define NAV_PROT_RTS_CTS_WITH_QAP_BIT 3
#define NAV_PROT_STBC_BIT 4
#define WHICHDESC_OFT 19
#define WHICHDESC_MSK (0x07 << WHICHDESC_OFT)
#define WHICHDESC_UNFRAGMENTED_MSDU (0x00 << WHICHDESC_OFT)
#define WHICHDESC_FRAGMENTED_MSDU_FIRST (0x01 << WHICHDESC_OFT)
#define WHICHDESC_FRAGMENTED_MSDU_INT (0x02 << WHICHDESC_OFT)
#define WHICHDESC_FRAGMENTED_MSDU_LAST (0x03 << WHICHDESC_OFT)
#define WHICHDESC_AMPDU_EXTRA (0x04 << WHICHDESC_OFT)
#define WHICHDESC_AMPDU_FIRST (0x05 << WHICHDESC_OFT)
#define WHICHDESC_AMPDU_INT (0x06 << WHICHDESC_OFT)
#define WHICHDESC_AMPDU_LAST (0x07 << WHICHDESC_OFT)
#define AMPDU_OFT 21
#define AMPDU_BIT CO_BIT(AMPDU_OFT)
union siwifi_mcs_index {
    struct {
        u32 mcs : 3;
        u32 nss : 2;
    } ht;
    struct {
        u32 mcs : 4;
        u32 nss : 3;
    } vht;
    struct {
        u32 mcs : 4;
        u32 nss : 3;
    } he;
    u32 legacy : 7;
};
union siwifi_rate_ctrl_info {
    struct {
        u32 mcsIndexTx : 7;
        u32 bwTx : 2;
        u32 giAndPreTypeTx : 2;
        u32 formatModTx : 3;
        u32 navProtFrmEx : 3;
        u32 mcsIndexProtTx : 7;
        u32 bwProtTx : 2;
        u32 formatModProtTx : 3;
        u32 nRetry : 3;
    };
    u32 value;
};
struct siwifi_power_ctrl_info {
    u32 txPwrLevelPT : 8;
    u32 txPwrLevelProtPT : 8;
    u32 reserved :16;
};
union siwifi_pol_phy_ctrl_info_1 {
    struct {
        u32 rsvd1 : 3;
        u32 bfFrmEx : 1;
        u32 numExtnSS : 2;
        u32 fecCoding : 1;
        u32 stbc : 2;
        u32 rsvd2 : 5;
        u32 nTx : 3;
        u32 nTxProt : 3;
    };
    u32 value;
};
union siwifi_pol_phy_ctrl_info_2 {
    struct {
        u32 antennaSet : 8;
        u32 smmIndex : 8;
        u32 beamFormed : 1;
    };
    u32 value;
};
union siwifi_pol_mac_ctrl_info_1 {
    struct {
        u32 keySRamIndex : 10;
        u32 keySRamIndexRA : 10;
    };
    u32 value;
};
union siwifi_pol_mac_ctrl_info_2 {
    struct {
        u32 longRetryLimit : 8;
        u32 shortRetryLimit : 8;
        u32 rtsThreshold : 12;
    };
    u32 value;
};
#define POLICY_TABLE_PATTERN 0xBADCAB1E
struct tx_policy_tbl {
    u32 upatterntx;
    union siwifi_pol_phy_ctrl_info_1 phyctrlinfo_1;
    union siwifi_pol_phy_ctrl_info_2 phyctrlinfo_2;
    union siwifi_pol_mac_ctrl_info_1 macctrlinfo_1;
    union siwifi_pol_mac_ctrl_info_2 macctrlinfo_2;
    union siwifi_rate_ctrl_info ratectrlinfos[NX_TX_MAX_RATES];
    struct siwifi_power_ctrl_info powerctrlinfos[NX_TX_MAX_RATES];
};
union siwifi_hw_txstatus {
    struct {
        u32 tx_done : 1;
        u32 retry_required : 1;
        u32 sw_retry_required : 1;
        u32 tx_successful : 1;
  u32 single_mpdu_retry : 1;
  u32 sw_flush : 1;
  u32 sw_discard : 1;
  u32 hw_tx : 1;
        u32 reserved :24;
    };
    u32 value;
};
struct tx_cfm_tag
{
    u16_l pn[4];
    u16_l sn;
    u16_l timestamp;
    s16_l credits;
    u8_l ampdu_size;
#ifdef CONFIG_SIWIFI_SPLIT_TX_BUF
    u16_l amsdu_size;
#endif
    uint32_t rateinfo;
    uint16_t rate_config;
    union siwifi_hw_txstatus status;
};
struct siwifi_hw_txhdr {
    struct tx_cfm_tag cfm;
};
#define MDM_PHY_CONFIG_TRIDENT 0
#define MDM_PHY_CONFIG_ELMA 1
#ifndef CONFIG_SF16A18_FULL_MASK
#define MDM_PHY_CONFIG_AETNENSIS 2
#define MDM_PHY_CONFIG_BEGONIA 4
#define MDM_PHY_CONFIG_BEGONIA_FULLMASK 5
#else
#define MDM_PHY_CONFIG_KARST 2
#define MDM_PHY_CONFIG_AETNENSIS 3
#endif
#define MDM_MUMIMOTX_BIT ((u32)0x80000000)
#define MDM_MUMIMOTX_POS 31
#define MDM_MUMIMORX_BIT ((u32)0x40000000)
#define MDM_MUMIMORX_POS 30
#define MDM_BFMER_BIT ((u32)0x20000000)
#define MDM_BFMER_POS 29
#define MDM_BFMEE_BIT ((u32)0x10000000)
#define MDM_BFMEE_POS 28
#define MDM_LDPCDEC_BIT ((u32)0x08000000)
#define MDM_LDPCDEC_POS 27
#define MDM_LDPCENC_BIT ((u32)0x04000000)
#define MDM_LDPCENC_POS 26
#define MDM_CHBW_MASK ((u32)0x03000000)
#define MDM_CHBW_LSB 24
#define MDM_CHBW_WIDTH ((u32)0x00000002)
#define MDM_DSSSCCK_BIT ((u32)0x00800000)
#define MDM_DSSSCCK_POS 23
#define MDM_VHT_BIT ((u32)0x00400000)
#define MDM_VHT_POS 22
#define MDM_HE_BIT ((u32)0x00200000)
#define MDM_HE_POS 21
#define MDM_ESS_BIT ((u32)0x00100000)
#define MDM_ESS_POS 20
#define MDM_RFMODE_MASK ((u32)0x000F0000)
#define MDM_RFMODE_LSB 16
#define MDM_RFMODE_WIDTH ((u32)0x00000004)
#define MDM_NSTS_MASK ((u32)0x0000F000)
#define MDM_NSTS_LSB 12
#define MDM_NSTS_WIDTH ((u32)0x00000004)
#define MDM_NSS_MASK ((u32)0x00000F00)
#define MDM_NSS_LSB 8
#define MDM_NSS_WIDTH ((u32)0x00000004)
#define MDM_NTX_MASK ((u32)0x000000F0)
#define MDM_NTX_LSB 4
#define MDM_NTX_WIDTH ((u32)0x00000004)
#define MDM_NRX_MASK ((u32)0x0000000F)
#define MDM_NRX_LSB 0
#define MDM_NRX_WIDTH ((u32)0x00000004)
#define __MDM_PHYCFG_FROM_VERS(v) (((v) & MDM_RFMODE_MASK) >> MDM_RFMODE_LSB)
#define RIU_FCU_PRESENT_MASK ((u32)0xFF000000)
#define RIU_FCU_PRESENT_LSB 24
#define __RIU_FCU_PRESENT(v) (((v) & RIU_FCU_PRESENT_MASK) >> RIU_FCU_PRESENT_LSB == 5)
#define RIU_AGC_LOAD_MASK ((u32)0x00C00000)
#define RIU_AGC_LOAD_LSB 22
#define __RIU_AGCLOAD_FROM_VERS(v) (((v) & RIU_AGC_LOAD_MASK) >> RIU_AGC_LOAD_LSB)
#define __FPGA_TYPE(v) (((v) & 0xFFFF0000) >> 16)
#define __MDM_MAJOR_VERSION(v) (((v) & 0xFF000000) >> 24)
#define __MDM_MINOR_VERSION(v) (((v) & 0x00FF0000) >> 16)
#endif
