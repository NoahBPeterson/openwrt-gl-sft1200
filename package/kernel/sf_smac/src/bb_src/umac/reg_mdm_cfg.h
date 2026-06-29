#ifndef _REG_MDM_CFG_H_
#define _REG_MDM_CFG_H_ 
#include "reg_access.h"
#define RW_NX_LDPC_DEC 1
#define RW_MUMIMO_RX_EN 1
#define RW_BFMEE_EN 1
#define REG_MDM_CFG_COUNT 45
#define REG_MDM_CFG_DECODING_MASK 0x000000FF
#if 0
#define MDM_MDMCONF_ADDR (REG_MDM_CFG_BASE_ADDR+0x0800)
#define MDM_MDMCONF_OFFSET 0x00000000
#define MDM_MDMCONF_INDEX 0x00000000
#define MDM_MDMCONF_RESET 0x00000000
__INLINE uint32_t mdm_mdmconf_get(void)
{
    return REG_PL_RD(MDM_MDMCONF_ADDR);
}
__INLINE void mdm_mdmconf_set(uint32_t value)
{
    REG_PL_WR(MDM_MDMCONF_ADDR, value);
}
#define MDM_MDMCONF_MASK ((uint32_t)0x0000000F)
#define MDM_MDMCONF_LSB 0
#define MDM_MDMCONF_WIDTH ((uint32_t)0x00000004)
#define MDM_MDMCONF_RST 0x0
__INLINE uint8_t mdm_mdmconf_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_MDMCONF_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000000F)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_mdmconf_setf(uint8_t mdmconf)
{
    ASSERT_ERR((((uint32_t)mdmconf << 0) & ~((uint32_t)0x0000000F)) == 0);
    REG_PL_WR(MDM_MDMCONF_ADDR, (uint32_t)mdmconf << 0);
}
#define MDM_RXTDCTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0804)
#define MDM_RXTDCTRL0_OFFSET 0x00000004
#define MDM_RXTDCTRL0_INDEX 0x00000001
#define MDM_RXTDCTRL0_RESET 0x1240FC0D
__INLINE uint32_t mdm_rxtdctrl0_get(void)
{
    return REG_PL_RD(MDM_RXTDCTRL0_ADDR);
}
__INLINE void mdm_rxtdctrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_RXTDCTRL0_ADDR, value);
}
#define MDM_TDSYNCOFF4080_MASK ((uint32_t)0xFF000000)
#define MDM_TDSYNCOFF4080_LSB 24
#define MDM_TDSYNCOFF4080_WIDTH ((uint32_t)0x00000008)
#define MDM_TDSYNCOFF2080_MASK ((uint32_t)0x00FF0000)
#define MDM_TDSYNCOFF2080_LSB 16
#define MDM_TDSYNCOFF2080_WIDTH ((uint32_t)0x00000008)
#define MDM_TDSYNCOFF4020_MASK ((uint32_t)0x0000FF00)
#define MDM_TDSYNCOFF4020_LSB 8
#define MDM_TDSYNCOFF4020_WIDTH ((uint32_t)0x00000008)
#define MDM_TDSYNCOFF2040_MASK ((uint32_t)0x000000FF)
#define MDM_TDSYNCOFF2040_LSB 0
#define MDM_TDSYNCOFF2040_WIDTH ((uint32_t)0x00000008)
#define MDM_TDSYNCOFF4080_RST 0x12
#define MDM_TDSYNCOFF2080_RST 0x40
#define MDM_TDSYNCOFF4020_RST 0xFC
#define MDM_TDSYNCOFF2040_RST 0xD
__INLINE void mdm_rxtdctrl0_pack(uint8_t tdsyncoff4080, uint8_t tdsyncoff2080, uint8_t tdsyncoff4020, uint8_t tdsyncoff2040)
{
    ASSERT_ERR((((uint32_t)tdsyncoff4080 << 24) & ~((uint32_t)0xFF000000)) == 0);
    ASSERT_ERR((((uint32_t)tdsyncoff2080 << 16) & ~((uint32_t)0x00FF0000)) == 0);
    ASSERT_ERR((((uint32_t)tdsyncoff4020 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint32_t)tdsyncoff2040 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_RXTDCTRL0_ADDR, ((uint32_t)tdsyncoff4080 << 24) | ((uint32_t)tdsyncoff2080 << 16) | ((uint32_t)tdsyncoff4020 << 8) | ((uint32_t)tdsyncoff2040 << 0));
}
__INLINE void mdm_rxtdctrl0_unpack(uint8_t* tdsyncoff4080, uint8_t* tdsyncoff2080, uint8_t* tdsyncoff4020, uint8_t* tdsyncoff2040)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL0_ADDR);
    *tdsyncoff4080 = (localVal & ((uint32_t)0xFF000000)) >> 24;
    *tdsyncoff2080 = (localVal & ((uint32_t)0x00FF0000)) >> 16;
    *tdsyncoff4020 = (localVal & ((uint32_t)0x0000FF00)) >> 8;
    *tdsyncoff2040 = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_tdsyncoff4080_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL0_ADDR);
    return ((localVal & ((uint32_t)0xFF000000)) >> 24);
}
__INLINE void mdm_tdsyncoff4080_setf(uint8_t tdsyncoff4080)
{
    ASSERT_ERR((((uint32_t)tdsyncoff4080 << 24) & ~((uint32_t)0xFF000000)) == 0);
    REG_PL_WR(MDM_RXTDCTRL0_ADDR, (REG_PL_RD(MDM_RXTDCTRL0_ADDR) & ~((uint32_t)0xFF000000)) | ((uint32_t)tdsyncoff4080 << 24));
}
__INLINE uint8_t mdm_tdsyncoff2080_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00FF0000)) >> 16);
}
__INLINE void mdm_tdsyncoff2080_setf(uint8_t tdsyncoff2080)
{
    ASSERT_ERR((((uint32_t)tdsyncoff2080 << 16) & ~((uint32_t)0x00FF0000)) == 0);
    REG_PL_WR(MDM_RXTDCTRL0_ADDR, (REG_PL_RD(MDM_RXTDCTRL0_ADDR) & ~((uint32_t)0x00FF0000)) | ((uint32_t)tdsyncoff2080 << 16));
}
__INLINE uint8_t mdm_tdsyncoff4020_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x0000FF00)) >> 8);
}
__INLINE void mdm_tdsyncoff4020_setf(uint8_t tdsyncoff4020)
{
    ASSERT_ERR((((uint32_t)tdsyncoff4020 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    REG_PL_WR(MDM_RXTDCTRL0_ADDR, (REG_PL_RD(MDM_RXTDCTRL0_ADDR) & ~((uint32_t)0x0000FF00)) | ((uint32_t)tdsyncoff4020 << 8));
}
__INLINE uint8_t mdm_tdsyncoff2040_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_tdsyncoff2040_setf(uint8_t tdsyncoff2040)
{
    ASSERT_ERR((((uint32_t)tdsyncoff2040 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_RXTDCTRL0_ADDR, (REG_PL_RD(MDM_RXTDCTRL0_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)tdsyncoff2040 << 0));
}
#define MDM_FDOCTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0808)
#define MDM_FDOCTRL0_OFFSET 0x00000008
#define MDM_FDOCTRL0_INDEX 0x00000002
#define MDM_FDOCTRL0_RESET 0xF3C4FBFF
__INLINE uint32_t mdm_fdoctrl0_get(void)
{
    return REG_PL_RD(MDM_FDOCTRL0_ADDR);
}
__INLINE void mdm_fdoctrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_FDOCTRL0_ADDR, value);
}
#define MDM_CFGSTO4TDCOMP_BIT ((uint32_t)0x80000000)
#define MDM_CFGSTO4TDCOMP_POS 31
#define MDM_CFGCPEWALK_BIT ((uint32_t)0x40000000)
#define MDM_CFGCPEWALK_POS 30
#define MDM_CFGSTOREF_BIT ((uint32_t)0x20000000)
#define MDM_CFGSTOREF_POS 29
#define MDM_CFGCPEREF_BIT ((uint32_t)0x10000000)
#define MDM_CFGCPEREF_POS 28
#define MDM_CPESLOPELGHT_BIT ((uint32_t)0x02000000)
#define MDM_CPESLOPELGHT_POS 25
#define MDM_STOSLOPELGHT_BIT ((uint32_t)0x01000000)
#define MDM_STOSLOPELGHT_POS 24
#define MDM_CPEMODE_BIT ((uint32_t)0x00800000)
#define MDM_CPEMODE_POS 23
#define MDM_STOMODE_BIT ((uint32_t)0x00400000)
#define MDM_STOMODE_POS 22
#define MDM_STOCOMPTIME_MASK ((uint32_t)0x003E0000)
#define MDM_STOCOMPTIME_LSB 17
#define MDM_STOCOMPTIME_WIDTH ((uint32_t)0x00000005)
#define MDM_MAXNTDSYNCHOFF_MASK ((uint32_t)0x0001F800)
#define MDM_MAXNTDSYNCHOFF_LSB 11
#define MDM_MAXNTDSYNCHOFF_WIDTH ((uint32_t)0x00000006)
#define MDM_MAXPTDSYNCHOFF_MASK ((uint32_t)0x000007E0)
#define MDM_MAXPTDSYNCHOFF_LSB 5
#define MDM_MAXPTDSYNCHOFF_WIDTH ((uint32_t)0x00000006)
#define MDM_STOFDCOMPEN_BIT ((uint32_t)0x00000010)
#define MDM_STOFDCOMPEN_POS 4
#define MDM_CPESLOPEESTEN_BIT ((uint32_t)0x00000008)
#define MDM_CPESLOPEESTEN_POS 3
#define MDM_CPEFDCOMPEN_BIT ((uint32_t)0x00000004)
#define MDM_CPEFDCOMPEN_POS 2
#define MDM_STOSLOPEESTEN_BIT ((uint32_t)0x00000002)
#define MDM_STOSLOPEESTEN_POS 1
#define MDM_STOTDCOMPEN_BIT ((uint32_t)0x00000001)
#define MDM_STOTDCOMPEN_POS 0
#define MDM_CFGSTO4TDCOMP_RST 0x1
#define MDM_CFGCPEWALK_RST 0x1
#define MDM_CFGSTOREF_RST 0x1
#define MDM_CFGCPEREF_RST 0x1
#define MDM_CPESLOPELGHT_RST 0x1
#define MDM_STOSLOPELGHT_RST 0x1
#define MDM_CPEMODE_RST 0x1
#define MDM_STOMODE_RST 0x1
#define MDM_STOCOMPTIME_RST 0x2
#define MDM_MAXNTDSYNCHOFF_RST 0x1F
#define MDM_MAXPTDSYNCHOFF_RST 0x1F
#define MDM_STOFDCOMPEN_RST 0x1
#define MDM_CPESLOPEESTEN_RST 0x1
#define MDM_CPEFDCOMPEN_RST 0x1
#define MDM_STOSLOPEESTEN_RST 0x1
#define MDM_STOTDCOMPEN_RST 0x1
__INLINE void mdm_fdoctrl0_pack(uint8_t cfgsto4tdcomp, uint8_t cfgcpewalk, uint8_t cfgstoref, uint8_t cfgcperef, uint8_t cpeslopelght, uint8_t stoslopelght, uint8_t cpemode, uint8_t stomode, uint8_t stocomptime, uint8_t maxntdsynchoff, uint8_t maxptdsynchoff, uint8_t stofdcompen, uint8_t cpeslopeesten, uint8_t cpefdcompen, uint8_t stoslopeesten, uint8_t stotdcompen)
{
    ASSERT_ERR((((uint32_t)cfgsto4tdcomp << 31) & ~((uint32_t)0x80000000)) == 0);
    ASSERT_ERR((((uint32_t)cfgcpewalk << 30) & ~((uint32_t)0x40000000)) == 0);
    ASSERT_ERR((((uint32_t)cfgstoref << 29) & ~((uint32_t)0x20000000)) == 0);
    ASSERT_ERR((((uint32_t)cfgcperef << 28) & ~((uint32_t)0x10000000)) == 0);
    ASSERT_ERR((((uint32_t)cpeslopelght << 25) & ~((uint32_t)0x02000000)) == 0);
    ASSERT_ERR((((uint32_t)stoslopelght << 24) & ~((uint32_t)0x01000000)) == 0);
    ASSERT_ERR((((uint32_t)cpemode << 23) & ~((uint32_t)0x00800000)) == 0);
    ASSERT_ERR((((uint32_t)stomode << 22) & ~((uint32_t)0x00400000)) == 0);
    ASSERT_ERR((((uint32_t)stocomptime << 17) & ~((uint32_t)0x003E0000)) == 0);
    ASSERT_ERR((((uint32_t)maxntdsynchoff << 11) & ~((uint32_t)0x0001F800)) == 0);
    ASSERT_ERR((((uint32_t)maxptdsynchoff << 5) & ~((uint32_t)0x000007E0)) == 0);
    ASSERT_ERR((((uint32_t)stofdcompen << 4) & ~((uint32_t)0x00000010)) == 0);
    ASSERT_ERR((((uint32_t)cpeslopeesten << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)cpefdcompen << 2) & ~((uint32_t)0x00000004)) == 0);
    ASSERT_ERR((((uint32_t)stoslopeesten << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)stotdcompen << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, ((uint32_t)cfgsto4tdcomp << 31) | ((uint32_t)cfgcpewalk << 30) | ((uint32_t)cfgstoref << 29) | ((uint32_t)cfgcperef << 28) | ((uint32_t)cpeslopelght << 25) | ((uint32_t)stoslopelght << 24) | ((uint32_t)cpemode << 23) | ((uint32_t)stomode << 22) | ((uint32_t)stocomptime << 17) | ((uint32_t)maxntdsynchoff << 11) | ((uint32_t)maxptdsynchoff << 5) | ((uint32_t)stofdcompen << 4) | ((uint32_t)cpeslopeesten << 3) | ((uint32_t)cpefdcompen << 2) | ((uint32_t)stoslopeesten << 1) | ((uint32_t)stotdcompen << 0));
}
__INLINE void mdm_fdoctrl0_unpack(uint8_t* cfgsto4tdcomp, uint8_t* cfgcpewalk, uint8_t* cfgstoref, uint8_t* cfgcperef, uint8_t* cpeslopelght, uint8_t* stoslopelght, uint8_t* cpemode, uint8_t* stomode, uint8_t* stocomptime, uint8_t* maxntdsynchoff, uint8_t* maxptdsynchoff, uint8_t* stofdcompen, uint8_t* cpeslopeesten, uint8_t* cpefdcompen, uint8_t* stoslopeesten, uint8_t* stotdcompen)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    *cfgsto4tdcomp = (localVal & ((uint32_t)0x80000000)) >> 31;
    *cfgcpewalk = (localVal & ((uint32_t)0x40000000)) >> 30;
    *cfgstoref = (localVal & ((uint32_t)0x20000000)) >> 29;
    *cfgcperef = (localVal & ((uint32_t)0x10000000)) >> 28;
    *cpeslopelght = (localVal & ((uint32_t)0x02000000)) >> 25;
    *stoslopelght = (localVal & ((uint32_t)0x01000000)) >> 24;
    *cpemode = (localVal & ((uint32_t)0x00800000)) >> 23;
    *stomode = (localVal & ((uint32_t)0x00400000)) >> 22;
    *stocomptime = (localVal & ((uint32_t)0x003E0000)) >> 17;
    *maxntdsynchoff = (localVal & ((uint32_t)0x0001F800)) >> 11;
    *maxptdsynchoff = (localVal & ((uint32_t)0x000007E0)) >> 5;
    *stofdcompen = (localVal & ((uint32_t)0x00000010)) >> 4;
    *cpeslopeesten = (localVal & ((uint32_t)0x00000008)) >> 3;
    *cpefdcompen = (localVal & ((uint32_t)0x00000004)) >> 2;
    *stoslopeesten = (localVal & ((uint32_t)0x00000002)) >> 1;
    *stotdcompen = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_cfgsto4tdcomp_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x80000000)) >> 31);
}
__INLINE void mdm_cfgsto4tdcomp_setf(uint8_t cfgsto4tdcomp)
{
    ASSERT_ERR((((uint32_t)cfgsto4tdcomp << 31) & ~((uint32_t)0x80000000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x80000000)) | ((uint32_t)cfgsto4tdcomp << 31));
}
__INLINE uint8_t mdm_cfgcpewalk_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x40000000)) >> 30);
}
__INLINE void mdm_cfgcpewalk_setf(uint8_t cfgcpewalk)
{
    ASSERT_ERR((((uint32_t)cfgcpewalk << 30) & ~((uint32_t)0x40000000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x40000000)) | ((uint32_t)cfgcpewalk << 30));
}
__INLINE uint8_t mdm_cfgstoref_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x20000000)) >> 29);
}
__INLINE void mdm_cfgstoref_setf(uint8_t cfgstoref)
{
    ASSERT_ERR((((uint32_t)cfgstoref << 29) & ~((uint32_t)0x20000000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x20000000)) | ((uint32_t)cfgstoref << 29));
}
__INLINE uint8_t mdm_cfgcperef_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x10000000)) >> 28);
}
__INLINE void mdm_cfgcperef_setf(uint8_t cfgcperef)
{
    ASSERT_ERR((((uint32_t)cfgcperef << 28) & ~((uint32_t)0x10000000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x10000000)) | ((uint32_t)cfgcperef << 28));
}
__INLINE uint8_t mdm_cpeslopelght_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x02000000)) >> 25);
}
__INLINE void mdm_cpeslopelght_setf(uint8_t cpeslopelght)
{
    ASSERT_ERR((((uint32_t)cpeslopelght << 25) & ~((uint32_t)0x02000000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x02000000)) | ((uint32_t)cpeslopelght << 25));
}
__INLINE uint8_t mdm_stoslopelght_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x01000000)) >> 24);
}
__INLINE void mdm_stoslopelght_setf(uint8_t stoslopelght)
{
    ASSERT_ERR((((uint32_t)stoslopelght << 24) & ~((uint32_t)0x01000000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x01000000)) | ((uint32_t)stoslopelght << 24));
}
__INLINE uint8_t mdm_cpemode_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00800000)) >> 23);
}
__INLINE void mdm_cpemode_setf(uint8_t cpemode)
{
    ASSERT_ERR((((uint32_t)cpemode << 23) & ~((uint32_t)0x00800000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00800000)) | ((uint32_t)cpemode << 23));
}
__INLINE uint8_t mdm_stomode_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00400000)) >> 22);
}
__INLINE void mdm_stomode_setf(uint8_t stomode)
{
    ASSERT_ERR((((uint32_t)stomode << 22) & ~((uint32_t)0x00400000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00400000)) | ((uint32_t)stomode << 22));
}
__INLINE uint8_t mdm_stocomptime_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x003E0000)) >> 17);
}
__INLINE void mdm_stocomptime_setf(uint8_t stocomptime)
{
    ASSERT_ERR((((uint32_t)stocomptime << 17) & ~((uint32_t)0x003E0000)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x003E0000)) | ((uint32_t)stocomptime << 17));
}
__INLINE uint8_t mdm_maxntdsynchoff_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x0001F800)) >> 11);
}
__INLINE void mdm_maxntdsynchoff_setf(uint8_t maxntdsynchoff)
{
    ASSERT_ERR((((uint32_t)maxntdsynchoff << 11) & ~((uint32_t)0x0001F800)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x0001F800)) | ((uint32_t)maxntdsynchoff << 11));
}
__INLINE uint8_t mdm_maxptdsynchoff_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x000007E0)) >> 5);
}
__INLINE void mdm_maxptdsynchoff_setf(uint8_t maxptdsynchoff)
{
    ASSERT_ERR((((uint32_t)maxptdsynchoff << 5) & ~((uint32_t)0x000007E0)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x000007E0)) | ((uint32_t)maxptdsynchoff << 5));
}
__INLINE uint8_t mdm_stofdcompen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000010)) >> 4);
}
__INLINE void mdm_stofdcompen_setf(uint8_t stofdcompen)
{
    ASSERT_ERR((((uint32_t)stofdcompen << 4) & ~((uint32_t)0x00000010)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00000010)) | ((uint32_t)stofdcompen << 4));
}
__INLINE uint8_t mdm_cpeslopeesten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void mdm_cpeslopeesten_setf(uint8_t cpeslopeesten)
{
    ASSERT_ERR((((uint32_t)cpeslopeesten << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00000008)) | ((uint32_t)cpeslopeesten << 3));
}
__INLINE uint8_t mdm_cpefdcompen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}
__INLINE void mdm_cpefdcompen_setf(uint8_t cpefdcompen)
{
    ASSERT_ERR((((uint32_t)cpefdcompen << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00000004)) | ((uint32_t)cpefdcompen << 2));
}
__INLINE uint8_t mdm_stoslopeesten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE void mdm_stoslopeesten_setf(uint8_t stoslopeesten)
{
    ASSERT_ERR((((uint32_t)stoslopeesten << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00000002)) | ((uint32_t)stoslopeesten << 1));
}
__INLINE uint8_t mdm_stotdcompen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_stotdcompen_setf(uint8_t stotdcompen)
{
    ASSERT_ERR((((uint32_t)stotdcompen << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_FDOCTRL0_ADDR, (REG_PL_RD(MDM_FDOCTRL0_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)stotdcompen << 0));
}
#define MDM_FDOCTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x080C)
#define MDM_FDOCTRL1_OFFSET 0x0000000C
#define MDM_FDOCTRL1_INDEX 0x00000003
#define MDM_FDOCTRL1_RESET 0x00000000
__INLINE uint32_t mdm_fdoctrl1_get(void)
{
    return REG_PL_RD(MDM_FDOCTRL1_ADDR);
}
__INLINE void mdm_fdoctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_FDOCTRL1_ADDR, value);
}
#define MDM_STOSLOPEFORCED_MASK ((uint32_t)0x0003FFFF)
#define MDM_STOSLOPEFORCED_LSB 0
#define MDM_STOSLOPEFORCED_WIDTH ((uint32_t)0x00000012)
#define MDM_STOSLOPEFORCED_RST 0x0
__INLINE uint32_t mdm_stoslopeforced_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL1_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0003FFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_stoslopeforced_setf(uint32_t stoslopeforced)
{
    ASSERT_ERR((((uint32_t)stoslopeforced << 0) & ~((uint32_t)0x0003FFFF)) == 0);
    REG_PL_WR(MDM_FDOCTRL1_ADDR, (uint32_t)stoslopeforced << 0);
}
#define MDM_FDOCTRL2_ADDR (REG_MDM_CFG_BASE_ADDR+0x0810)
#define MDM_FDOCTRL2_OFFSET 0x00000010
#define MDM_FDOCTRL2_INDEX 0x00000004
#define MDM_FDOCTRL2_RESET 0x00000000
__INLINE uint32_t mdm_fdoctrl2_get(void)
{
    return REG_PL_RD(MDM_FDOCTRL2_ADDR);
}
__INLINE void mdm_fdoctrl2_set(uint32_t value)
{
    REG_PL_WR(MDM_FDOCTRL2_ADDR, value);
}
#define MDM_CPESLOPEFORCED_MASK ((uint32_t)0x0003FFFF)
#define MDM_CPESLOPEFORCED_LSB 0
#define MDM_CPESLOPEFORCED_WIDTH ((uint32_t)0x00000012)
#define MDM_CPESLOPEFORCED_RST 0x0
__INLINE uint32_t mdm_cpeslopeforced_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_FDOCTRL2_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0003FFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_cpeslopeforced_setf(uint32_t cpeslopeforced)
{
    ASSERT_ERR((((uint32_t)cpeslopeforced << 0) & ~((uint32_t)0x0003FFFF)) == 0);
    REG_PL_WR(MDM_FDOCTRL2_ADDR, (uint32_t)cpeslopeforced << 0);
}
#define MDM_EQUALCTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0814)
#define MDM_EQUALCTRL0_OFFSET 0x00000014
#define MDM_EQUALCTRL0_INDEX 0x00000005
#define MDM_EQUALCTRL0_RESET 0x4DA00189
__INLINE uint32_t mdm_equalctrl0_get(void)
{
    return REG_PL_RD(MDM_EQUALCTRL0_ADDR);
}
__INLINE void mdm_equalctrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, value);
}
#define MDM_FDDCWEIGHTP2_MASK ((uint32_t)0xE0000000)
#define MDM_FDDCWEIGHTP2_LSB 29
#define MDM_FDDCWEIGHTP2_WIDTH ((uint32_t)0x00000003)
#define MDM_FDDCWEIGHTP1_MASK ((uint32_t)0x1C000000)
#define MDM_FDDCWEIGHTP1_LSB 26
#define MDM_FDDCWEIGHTP1_WIDTH ((uint32_t)0x00000003)
#define MDM_FDDCWEIGHTM1_MASK ((uint32_t)0x03800000)
#define MDM_FDDCWEIGHTM1_LSB 23
#define MDM_FDDCWEIGHTM1_WIDTH ((uint32_t)0x00000003)
#define MDM_FDDCWEIGHTM2_MASK ((uint32_t)0x00700000)
#define MDM_FDDCWEIGHTM2_LSB 20
#define MDM_FDDCWEIGHTM2_WIDTH ((uint32_t)0x00000003)
#define MDM_NOISEVARADJUSTDB_MASK ((uint32_t)0x000FF000)
#define MDM_NOISEVARADJUSTDB_LSB 12
#define MDM_NOISEVARADJUSTDB_WIDTH ((uint32_t)0x00000008)
#define MDM_FDDCSNRTHRESHOLD_MASK ((uint32_t)0x00000FF0)
#define MDM_FDDCSNRTHRESHOLD_LSB 4
#define MDM_FDDCSNRTHRESHOLD_WIDTH ((uint32_t)0x00000008)
#define MDM_FDDCCOMPEN_BIT ((uint32_t)0x00000008)
#define MDM_FDDCCOMPEN_POS 3
#define MDM_EQNOISEVARESTEN_BIT ((uint32_t)0x00000001)
#define MDM_EQNOISEVARESTEN_POS 0
#define MDM_FDDCWEIGHTP2_RST 0x2
#define MDM_FDDCWEIGHTP1_RST 0x3
#define MDM_FDDCWEIGHTM1_RST 0x3
#define MDM_FDDCWEIGHTM2_RST 0x2
#define MDM_NOISEVARADJUSTDB_RST 0x0
#define MDM_FDDCSNRTHRESHOLD_RST 0x18
#define MDM_FDDCCOMPEN_RST 0x1
#define MDM_EQNOISEVARESTEN_RST 0x1
__INLINE void mdm_equalctrl0_pack(uint8_t fddcweightp2, uint8_t fddcweightp1, uint8_t fddcweightm1, uint8_t fddcweightm2, uint8_t noisevaradjustdb, uint8_t fddcsnrthreshold, uint8_t fddccompen, uint8_t eqnoisevaresten)
{
    ASSERT_ERR((((uint32_t)fddcweightp2 << 29) & ~((uint32_t)0xE0000000)) == 0);
    ASSERT_ERR((((uint32_t)fddcweightp1 << 26) & ~((uint32_t)0x1C000000)) == 0);
    ASSERT_ERR((((uint32_t)fddcweightm1 << 23) & ~((uint32_t)0x03800000)) == 0);
    ASSERT_ERR((((uint32_t)fddcweightm2 << 20) & ~((uint32_t)0x00700000)) == 0);
    ASSERT_ERR((((uint32_t)noisevaradjustdb << 12) & ~((uint32_t)0x000FF000)) == 0);
    ASSERT_ERR((((uint32_t)fddcsnrthreshold << 4) & ~((uint32_t)0x00000FF0)) == 0);
    ASSERT_ERR((((uint32_t)fddccompen << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)eqnoisevaresten << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, ((uint32_t)fddcweightp2 << 29) | ((uint32_t)fddcweightp1 << 26) | ((uint32_t)fddcweightm1 << 23) | ((uint32_t)fddcweightm2 << 20) | ((uint32_t)noisevaradjustdb << 12) | ((uint32_t)fddcsnrthreshold << 4) | ((uint32_t)fddccompen << 3) | ((uint32_t)eqnoisevaresten << 0));
}
__INLINE void mdm_equalctrl0_unpack(uint8_t* fddcweightp2, uint8_t* fddcweightp1, uint8_t* fddcweightm1, uint8_t* fddcweightm2, uint8_t* noisevaradjustdb, uint8_t* fddcsnrthreshold, uint8_t* fddccompen, uint8_t* eqnoisevaresten)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    *fddcweightp2 = (localVal & ((uint32_t)0xE0000000)) >> 29;
    *fddcweightp1 = (localVal & ((uint32_t)0x1C000000)) >> 26;
    *fddcweightm1 = (localVal & ((uint32_t)0x03800000)) >> 23;
    *fddcweightm2 = (localVal & ((uint32_t)0x00700000)) >> 20;
    *noisevaradjustdb = (localVal & ((uint32_t)0x000FF000)) >> 12;
    *fddcsnrthreshold = (localVal & ((uint32_t)0x00000FF0)) >> 4;
    *fddccompen = (localVal & ((uint32_t)0x00000008)) >> 3;
    *eqnoisevaresten = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_fddcweightp2_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0xE0000000)) >> 29);
}
__INLINE void mdm_fddcweightp2_setf(uint8_t fddcweightp2)
{
    ASSERT_ERR((((uint32_t)fddcweightp2 << 29) & ~((uint32_t)0xE0000000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0xE0000000)) | ((uint32_t)fddcweightp2 << 29));
}
__INLINE uint8_t mdm_fddcweightp1_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x1C000000)) >> 26);
}
__INLINE void mdm_fddcweightp1_setf(uint8_t fddcweightp1)
{
    ASSERT_ERR((((uint32_t)fddcweightp1 << 26) & ~((uint32_t)0x1C000000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x1C000000)) | ((uint32_t)fddcweightp1 << 26));
}
__INLINE uint8_t mdm_fddcweightm1_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x03800000)) >> 23);
}
__INLINE void mdm_fddcweightm1_setf(uint8_t fddcweightm1)
{
    ASSERT_ERR((((uint32_t)fddcweightm1 << 23) & ~((uint32_t)0x03800000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x03800000)) | ((uint32_t)fddcweightm1 << 23));
}
__INLINE uint8_t mdm_fddcweightm2_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00700000)) >> 20);
}
__INLINE void mdm_fddcweightm2_setf(uint8_t fddcweightm2)
{
    ASSERT_ERR((((uint32_t)fddcweightm2 << 20) & ~((uint32_t)0x00700000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x00700000)) | ((uint32_t)fddcweightm2 << 20));
}
__INLINE uint8_t mdm_noisevaradjustdb_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x000FF000)) >> 12);
}
__INLINE void mdm_noisevaradjustdb_setf(uint8_t noisevaradjustdb)
{
    ASSERT_ERR((((uint32_t)noisevaradjustdb << 12) & ~((uint32_t)0x000FF000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x000FF000)) | ((uint32_t)noisevaradjustdb << 12));
}
__INLINE uint8_t mdm_fddcsnrthreshold_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000FF0)) >> 4);
}
__INLINE void mdm_fddcsnrthreshold_setf(uint8_t fddcsnrthreshold)
{
    ASSERT_ERR((((uint32_t)fddcsnrthreshold << 4) & ~((uint32_t)0x00000FF0)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x00000FF0)) | ((uint32_t)fddcsnrthreshold << 4));
}
__INLINE uint8_t mdm_fddccompen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void mdm_fddccompen_setf(uint8_t fddccompen)
{
    ASSERT_ERR((((uint32_t)fddccompen << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x00000008)) | ((uint32_t)fddccompen << 3));
}
__INLINE uint8_t mdm_eqnoisevaresten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_eqnoisevaresten_setf(uint8_t eqnoisevaresten)
{
    ASSERT_ERR((((uint32_t)eqnoisevaresten << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_EQUALCTRL0_ADDR, (REG_PL_RD(MDM_EQUALCTRL0_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)eqnoisevaresten << 0));
}
#define MDM_SMOOTHCTRL_ADDR (REG_MDM_CFG_BASE_ADDR+0x0818)
#define MDM_SMOOTHCTRL_OFFSET 0x00000018
#define MDM_SMOOTHCTRL_INDEX 0x00000006
#define MDM_SMOOTHCTRL_RESET 0x01880C06
__INLINE uint32_t mdm_smoothctrl_get(void)
{
    return REG_PL_RD(MDM_SMOOTHCTRL_ADDR);
}
__INLINE void mdm_smoothctrl_set(uint32_t value)
{
    REG_PL_WR(MDM_SMOOTHCTRL_ADDR, value);
}
#define MDM_TDCYCROTVAL80_MASK ((uint32_t)0x0FF00000)
#define MDM_TDCYCROTVAL80_LSB 20
#define MDM_TDCYCROTVAL80_WIDTH ((uint32_t)0x00000008)
#define MDM_SMOOTHEN_BIT ((uint32_t)0x00080000)
#define MDM_SMOOTHEN_POS 19
#define MDM_TDCYCROTVAL40_MASK ((uint32_t)0x0000FF00)
#define MDM_TDCYCROTVAL40_LSB 8
#define MDM_TDCYCROTVAL40_WIDTH ((uint32_t)0x00000008)
#define MDM_TDCYCROTVAL20_MASK ((uint32_t)0x000000FF)
#define MDM_TDCYCROTVAL20_LSB 0
#define MDM_TDCYCROTVAL20_WIDTH ((uint32_t)0x00000008)
#define MDM_TDCYCROTVAL80_RST 0x18
#define MDM_SMOOTHEN_RST 0x1
#define MDM_TDCYCROTVAL40_RST 0xC
#define MDM_TDCYCROTVAL20_RST 0x6
__INLINE void mdm_smoothctrl_pack(uint8_t tdcycrotval80, uint8_t smoothen, uint8_t tdcycrotval40, uint8_t tdcycrotval20)
{
    ASSERT_ERR((((uint32_t)tdcycrotval80 << 20) & ~((uint32_t)0x0FF00000)) == 0);
    ASSERT_ERR((((uint32_t)smoothen << 19) & ~((uint32_t)0x00080000)) == 0);
    ASSERT_ERR((((uint32_t)tdcycrotval40 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint32_t)tdcycrotval20 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_SMOOTHCTRL_ADDR, ((uint32_t)tdcycrotval80 << 20) | ((uint32_t)smoothen << 19) | ((uint32_t)tdcycrotval40 << 8) | ((uint32_t)tdcycrotval20 << 0));
}
__INLINE void mdm_smoothctrl_unpack(uint8_t* tdcycrotval80, uint8_t* smoothen, uint8_t* tdcycrotval40, uint8_t* tdcycrotval20)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHCTRL_ADDR);
    *tdcycrotval80 = (localVal & ((uint32_t)0x0FF00000)) >> 20;
    *smoothen = (localVal & ((uint32_t)0x00080000)) >> 19;
    *tdcycrotval40 = (localVal & ((uint32_t)0x0000FF00)) >> 8;
    *tdcycrotval20 = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_tdcycrotval80_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHCTRL_ADDR);
    return ((localVal & ((uint32_t)0x0FF00000)) >> 20);
}
__INLINE void mdm_tdcycrotval80_setf(uint8_t tdcycrotval80)
{
    ASSERT_ERR((((uint32_t)tdcycrotval80 << 20) & ~((uint32_t)0x0FF00000)) == 0);
    REG_PL_WR(MDM_SMOOTHCTRL_ADDR, (REG_PL_RD(MDM_SMOOTHCTRL_ADDR) & ~((uint32_t)0x0FF00000)) | ((uint32_t)tdcycrotval80 << 20));
}
__INLINE uint8_t mdm_smoothen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHCTRL_ADDR);
    return ((localVal & ((uint32_t)0x00080000)) >> 19);
}
__INLINE void mdm_smoothen_setf(uint8_t smoothen)
{
    ASSERT_ERR((((uint32_t)smoothen << 19) & ~((uint32_t)0x00080000)) == 0);
    REG_PL_WR(MDM_SMOOTHCTRL_ADDR, (REG_PL_RD(MDM_SMOOTHCTRL_ADDR) & ~((uint32_t)0x00080000)) | ((uint32_t)smoothen << 19));
}
__INLINE uint8_t mdm_tdcycrotval40_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHCTRL_ADDR);
    return ((localVal & ((uint32_t)0x0000FF00)) >> 8);
}
__INLINE void mdm_tdcycrotval40_setf(uint8_t tdcycrotval40)
{
    ASSERT_ERR((((uint32_t)tdcycrotval40 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    REG_PL_WR(MDM_SMOOTHCTRL_ADDR, (REG_PL_RD(MDM_SMOOTHCTRL_ADDR) & ~((uint32_t)0x0000FF00)) | ((uint32_t)tdcycrotval40 << 8));
}
__INLINE uint8_t mdm_tdcycrotval20_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHCTRL_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_tdcycrotval20_setf(uint8_t tdcycrotval20)
{
    ASSERT_ERR((((uint32_t)tdcycrotval20 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_SMOOTHCTRL_ADDR, (REG_PL_RD(MDM_SMOOTHCTRL_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)tdcycrotval20 << 0));
}
#define MDM_DCESTIMCTRL_ADDR (REG_MDM_CFG_BASE_ADDR + 0x081C)
#define MDM_DCESTIMCTRL_OFFSET 0x0000001C
#define MDM_DCESTIMCTRL_INDEX 0x00000007
#define MDM_DCESTIMCTRL_RESET 0x00000F0F
__INLINE uint32_t mdm_dcestimctrl_get(void)
{
    return REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
}
__INLINE void mdm_dcestimctrl_set(uint32_t value)
{
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, value);
}
#define MDM_STARTHTDC_MASK ((uint32_t)0xF0000000)
#define MDM_STARTHTDC_LSB 28
#define MDM_STARTHTDC_WIDTH ((uint32_t)0x00000004)
#define MDM_STARTDC_MASK ((uint32_t)0x0F000000)
#define MDM_STARTDC_LSB 24
#define MDM_STARTDC_WIDTH ((uint32_t)0x00000004)
#define MDM_DELAYSYNCTD20_MASK ((uint32_t)0x007F0000)
#define MDM_DELAYSYNCTD20_LSB 16
#define MDM_DELAYSYNCTD20_WIDTH ((uint32_t)0x00000007)
#define MDM_DELAYSYNC_MASK ((uint32_t)0x00007F00)
#define MDM_DELAYSYNC_LSB 8
#define MDM_DELAYSYNC_WIDTH ((uint32_t)0x00000007)
#define MDM_WAITHTSTF_MASK ((uint32_t)0x0000007F)
#define MDM_WAITHTSTF_LSB 0
#define MDM_WAITHTSTF_WIDTH ((uint32_t)0x00000007)
#define MDM_STARTHTDC_RST 0x0
#define MDM_STARTDC_RST 0x0
#define MDM_DELAYSYNCTD20_RST 0x0
#define MDM_DELAYSYNC_RST 0xF
#define MDM_WAITHTSTF_RST 0xF
__INLINE void mdm_dcestimctrl_pack(uint8_t starthtdc, uint8_t startdc, uint8_t delaysynctd20, uint8_t delaysync, uint8_t waithtstf)
{
    ASSERT_ERR((((uint32_t)starthtdc << 28) & ~((uint32_t)0xF0000000)) == 0);
    ASSERT_ERR((((uint32_t)startdc << 24) & ~((uint32_t)0x0F000000)) == 0);
    ASSERT_ERR((((uint32_t)delaysynctd20 << 16) & ~((uint32_t)0x007F0000)) == 0);
    ASSERT_ERR((((uint32_t)delaysync << 8) & ~((uint32_t)0x00007F00)) == 0);
    ASSERT_ERR((((uint32_t)waithtstf << 0) & ~((uint32_t)0x0000007F)) == 0);
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, ((uint32_t)starthtdc << 28) | ((uint32_t)startdc << 24) | ((uint32_t)delaysynctd20 << 16) | ((uint32_t)delaysync << 8) | ((uint32_t)waithtstf << 0));
}
__INLINE void mdm_dcestimctrl_unpack(uint8_t* starthtdc, uint8_t* startdc, uint8_t* delaysynctd20, uint8_t* delaysync, uint8_t* waithtstf)
{
    uint32_t localVal = REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
    *starthtdc = (localVal & ((uint32_t)0xF0000000)) >> 28;
    *startdc = (localVal & ((uint32_t)0x0F000000)) >> 24;
    *delaysynctd20 = (localVal & ((uint32_t)0x007F0000)) >> 16;
    *delaysync = (localVal & ((uint32_t)0x00007F00)) >> 8;
    *waithtstf = (localVal & ((uint32_t)0x0000007F)) >> 0;
}
__INLINE uint8_t mdm_starthtdc_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
    return ((localVal & ((uint32_t)0xF0000000)) >> 28);
}
__INLINE void mdm_starthtdc_setf(uint8_t starthtdc)
{
    ASSERT_ERR((((uint32_t)starthtdc << 28) & ~((uint32_t)0xF0000000)) == 0);
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, (REG_PL_RD(MDM_DCESTIMCTRL_ADDR) & ~((uint32_t)0xF0000000)) | ((uint32_t)starthtdc << 28));
}
__INLINE uint8_t mdm_startdc_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
    return ((localVal & ((uint32_t)0x0F000000)) >> 24);
}
__INLINE void mdm_startdc_setf(uint8_t startdc)
{
    ASSERT_ERR((((uint32_t)startdc << 24) & ~((uint32_t)0x0F000000)) == 0);
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, (REG_PL_RD(MDM_DCESTIMCTRL_ADDR) & ~((uint32_t)0x0F000000)) | ((uint32_t)startdc << 24));
}
__INLINE uint8_t mdm_delaysynctd20_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
    return ((localVal & ((uint32_t)0x007F0000)) >> 16);
}
__INLINE void mdm_delaysynctd20_setf(uint8_t delaysynctd20)
{
    ASSERT_ERR((((uint32_t)delaysynctd20 << 16) & ~((uint32_t)0x007F0000)) == 0);
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, (REG_PL_RD(MDM_DCESTIMCTRL_ADDR) & ~((uint32_t)0x007F0000)) | ((uint32_t)delaysynctd20 << 16));
}
__INLINE uint8_t mdm_delaysync_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
    return ((localVal & ((uint32_t)0x00007F00)) >> 8);
}
__INLINE void mdm_delaysync_setf(uint8_t delaysync)
{
    ASSERT_ERR((((uint32_t)delaysync << 8) & ~((uint32_t)0x00007F00)) == 0);
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, (REG_PL_RD(MDM_DCESTIMCTRL_ADDR) & ~((uint32_t)0x00007F00)) | ((uint32_t)delaysync << 8));
}
__INLINE uint8_t mdm_waithtstf_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_DCESTIMCTRL_ADDR);
    return ((localVal & ((uint32_t)0x0000007F)) >> 0);
}
__INLINE void mdm_waithtstf_setf(uint8_t waithtstf)
{
    ASSERT_ERR((((uint32_t)waithtstf << 0) & ~((uint32_t)0x0000007F)) == 0);
    REG_PL_WR(MDM_DCESTIMCTRL_ADDR, (REG_PL_RD(MDM_DCESTIMCTRL_ADDR) & ~((uint32_t)0x0000007F)) | ((uint32_t)waithtstf << 0));
}
#define MDM_RXMODE_ADDR (REG_MDM_CFG_BASE_ADDR+0x0820)
#define MDM_RXMODE_OFFSET 0x00000020
#define MDM_RXMODE_INDEX 0x00000008
#define MDM_RXMODE_RESET 0x0010331F
__INLINE uint32_t mdm_rxmode_get(void)
{
    return REG_PL_RD(MDM_RXMODE_ADDR);
}
__INLINE void mdm_rxmode_set(uint32_t value)
{
    REG_PL_WR(MDM_RXMODE_ADDR, value);
}
#define MDM_RXMUMIMOAPEPLENEN_BIT ((uint32_t)0x00100000)
#define MDM_RXMUMIMOAPEPLENEN_POS 20
#define MDM_RXMUMIMOEN_BIT ((uint32_t)0x00010000)
#define MDM_RXMUMIMOEN_POS 16
#define MDM_RXNDPNSTSMAX_MASK ((uint32_t)0x00007000)
#define MDM_RXNDPNSTSMAX_LSB 12
#define MDM_RXNDPNSTSMAX_WIDTH ((uint32_t)0x00000003)
#define MDM_RXESSEN_BIT ((uint32_t)0x00000800)
#define MDM_RXESSEN_POS 11
#define MDM_RXUNEQMODEN_BIT ((uint32_t)0x00000400)
#define MDM_RXUNEQMODEN_POS 10
#define MDM_RXSTBCEN_BIT ((uint32_t)0x00000200)
#define MDM_RXSTBCEN_POS 9
#define MDM_RXLDPCEN_BIT ((uint32_t)0x00000100)
#define MDM_RXLDPCEN_POS 8
#define MDM_RXNSSMAX_MASK ((uint32_t)0x00000070)
#define MDM_RXNSSMAX_LSB 4
#define MDM_RXNSSMAX_WIDTH ((uint32_t)0x00000003)
#define MDM_RXGFEN_BIT ((uint32_t)0x00000008)
#define MDM_RXGFEN_POS 3
#define MDM_RXMMEN_BIT ((uint32_t)0x00000004)
#define MDM_RXMMEN_POS 2
#define MDM_RXVHTEN_BIT ((uint32_t)0x00000002)
#define MDM_RXVHTEN_POS 1
#define MDM_RXDSSSEN_BIT ((uint32_t)0x00000001)
#define MDM_RXDSSSEN_POS 0
#define MDM_RXMUMIMOAPEPLENEN_RST 0x1
#define MDM_RXMUMIMOEN_RST 0x0
#define MDM_RXNDPNSTSMAX_RST 0x3
#define MDM_RXESSEN_RST 0x0
#define MDM_RXUNEQMODEN_RST 0x0
#define MDM_RXSTBCEN_RST 0x1
#define MDM_RXLDPCEN_RST 0x1
#define MDM_RXNSSMAX_RST 0x1
#define MDM_RXGFEN_RST 0x1
#define MDM_RXMMEN_RST 0x1
#define MDM_RXVHTEN_RST 0x1
#define MDM_RXDSSSEN_RST 0x1
__INLINE void mdm_rxmode_pack(uint8_t rxmumimoapeplenen, uint8_t rxmumimoen, uint8_t rxndpnstsmax, uint8_t rxessen, uint8_t rxuneqmoden, uint8_t rxstbcen, uint8_t rxldpcen, uint8_t rxnssmax, uint8_t rxgfen, uint8_t rxmmen, uint8_t rxvhten, uint8_t rxdsssen)
{
    ASSERT_ERR((((uint32_t)rxmumimoapeplenen << 20) & ~((uint32_t)0x00100000)) == 0);
    ASSERT_ERR((((uint32_t)rxmumimoen << 16) & ~((uint32_t)0x00010000)) == 0);
    ASSERT_ERR((((uint32_t)rxndpnstsmax << 12) & ~((uint32_t)0x00007000)) == 0);
    ASSERT_ERR((((uint32_t)rxessen << 11) & ~((uint32_t)0x00000800)) == 0);
    ASSERT_ERR((((uint32_t)rxuneqmoden << 10) & ~((uint32_t)0x00000400)) == 0);
    ASSERT_ERR((((uint32_t)rxstbcen << 9) & ~((uint32_t)0x00000200)) == 0);
    ASSERT_ERR((((uint32_t)rxldpcen << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)rxnssmax << 4) & ~((uint32_t)0x00000070)) == 0);
    ASSERT_ERR((((uint32_t)rxgfen << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)rxmmen << 2) & ~((uint32_t)0x00000004)) == 0);
    ASSERT_ERR((((uint32_t)rxvhten << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)rxdsssen << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, ((uint32_t)rxmumimoapeplenen << 20) | ((uint32_t)rxmumimoen << 16) | ((uint32_t)rxndpnstsmax << 12) | ((uint32_t)rxessen << 11) | ((uint32_t)rxuneqmoden << 10) | ((uint32_t)rxstbcen << 9) | ((uint32_t)rxldpcen << 8) | ((uint32_t)rxnssmax << 4) | ((uint32_t)rxgfen << 3) | ((uint32_t)rxmmen << 2) | ((uint32_t)rxvhten << 1) | ((uint32_t)rxdsssen << 0));
}
__INLINE void mdm_rxmode_unpack(uint8_t* rxmumimoapeplenen, uint8_t* rxmumimoen, uint8_t* rxndpnstsmax, uint8_t* rxessen, uint8_t* rxuneqmoden, uint8_t* rxstbcen, uint8_t* rxldpcen, uint8_t* rxnssmax, uint8_t* rxgfen, uint8_t* rxmmen, uint8_t* rxvhten, uint8_t* rxdsssen)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    *rxmumimoapeplenen = (localVal & ((uint32_t)0x00100000)) >> 20;
    *rxmumimoen = (localVal & ((uint32_t)0x00010000)) >> 16;
    *rxndpnstsmax = (localVal & ((uint32_t)0x00007000)) >> 12;
    *rxessen = (localVal & ((uint32_t)0x00000800)) >> 11;
    *rxuneqmoden = (localVal & ((uint32_t)0x00000400)) >> 10;
    *rxstbcen = (localVal & ((uint32_t)0x00000200)) >> 9;
    *rxldpcen = (localVal & ((uint32_t)0x00000100)) >> 8;
    *rxnssmax = (localVal & ((uint32_t)0x00000070)) >> 4;
    *rxgfen = (localVal & ((uint32_t)0x00000008)) >> 3;
    *rxmmen = (localVal & ((uint32_t)0x00000004)) >> 2;
    *rxvhten = (localVal & ((uint32_t)0x00000002)) >> 1;
    *rxdsssen = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_rxmumimoapeplenen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00100000)) >> 20);
}
__INLINE void mdm_rxmumimoapeplenen_setf(uint8_t rxmumimoapeplenen)
{
    ASSERT_ERR((((uint32_t)rxmumimoapeplenen << 20) & ~((uint32_t)0x00100000)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00100000)) | ((uint32_t)rxmumimoapeplenen << 20));
}
__INLINE uint8_t mdm_rxmumimoen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00010000)) >> 16);
}
__INLINE void mdm_rxmumimoen_setf(uint8_t rxmumimoen)
{
    ASSERT_ERR((((uint32_t)rxmumimoen << 16) & ~((uint32_t)0x00010000)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00010000)) | ((uint32_t)rxmumimoen << 16));
}
__INLINE uint8_t mdm_rxndpnstsmax_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00007000)) >> 12);
}
__INLINE void mdm_rxndpnstsmax_setf(uint8_t rxndpnstsmax)
{
    ASSERT_ERR((((uint32_t)rxndpnstsmax << 12) & ~((uint32_t)0x00007000)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00007000)) | ((uint32_t)rxndpnstsmax << 12));
}
__INLINE uint8_t mdm_rxessen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000800)) >> 11);
}
__INLINE void mdm_rxessen_setf(uint8_t rxessen)
{
    ASSERT_ERR((((uint32_t)rxessen << 11) & ~((uint32_t)0x00000800)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000800)) | ((uint32_t)rxessen << 11));
}
__INLINE uint8_t mdm_rxuneqmoden_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000400)) >> 10);
}
__INLINE void mdm_rxuneqmoden_setf(uint8_t rxuneqmoden)
{
    ASSERT_ERR((((uint32_t)rxuneqmoden << 10) & ~((uint32_t)0x00000400)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000400)) | ((uint32_t)rxuneqmoden << 10));
}
__INLINE uint8_t mdm_rxstbcen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000200)) >> 9);
}
__INLINE void mdm_rxstbcen_setf(uint8_t rxstbcen)
{
    ASSERT_ERR((((uint32_t)rxstbcen << 9) & ~((uint32_t)0x00000200)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000200)) | ((uint32_t)rxstbcen << 9));
}
__INLINE uint8_t mdm_rxldpcen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}
__INLINE void mdm_rxldpcen_setf(uint8_t rxldpcen)
{
    ASSERT_ERR((((uint32_t)rxldpcen << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000100)) | ((uint32_t)rxldpcen << 8));
}
__INLINE uint8_t mdm_rxnssmax_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000070)) >> 4);
}
__INLINE void mdm_rxnssmax_setf(uint8_t rxnssmax)
{
    ASSERT_ERR((((uint32_t)rxnssmax << 4) & ~((uint32_t)0x00000070)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000070)) | ((uint32_t)rxnssmax << 4));
}
__INLINE uint8_t mdm_rxgfen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void mdm_rxgfen_setf(uint8_t rxgfen)
{
    ASSERT_ERR((((uint32_t)rxgfen << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000008)) | ((uint32_t)rxgfen << 3));
}
__INLINE uint8_t mdm_rxmmen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}
__INLINE void mdm_rxmmen_setf(uint8_t rxmmen)
{
    ASSERT_ERR((((uint32_t)rxmmen << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000004)) | ((uint32_t)rxmmen << 2));
}
__INLINE uint8_t mdm_rxvhten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE void mdm_rxvhten_setf(uint8_t rxvhten)
{
    ASSERT_ERR((((uint32_t)rxvhten << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000002)) | ((uint32_t)rxvhten << 1));
}
__INLINE uint8_t mdm_rxdsssen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_rxdsssen_setf(uint8_t rxdsssen)
{
    ASSERT_ERR((((uint32_t)rxdsssen << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_RXMODE_ADDR, (REG_PL_RD(MDM_RXMODE_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)rxdsssen << 0));
}
#define MDM_TXMODE_ADDR (REG_MDM_CFG_BASE_ADDR + 0x0824)
#define MDM_TXMODE_OFFSET 0x00000024
#define MDM_TXMODE_INDEX 0x00000009
#define MDM_TXMODE_RESET 0x0210031F
__INLINE uint32_t mdm_txmode_get(void)
{
    return REG_PL_RD(MDM_TXMODE_ADDR);
}
__INLINE void mdm_txmode_set(uint32_t value)
{
    REG_PL_WR(MDM_TXMODE_ADDR, value);
}
#define MDM_TXCBWMAX_MASK ((uint32_t)0x03000000)
#define MDM_TXCBWMAX_LSB 24
#define MDM_TXCBWMAX_WIDTH ((uint32_t)0x00000002)
#define MDM_NTXMAX_MASK ((uint32_t)0x00700000)
#define MDM_NTXMAX_LSB 20
#define MDM_NTXMAX_WIDTH ((uint32_t)0x00000003)
#define MDM_TXMUMIMOEN_BIT ((uint32_t)0x00010000)
#define MDM_TXMUMIMOEN_POS 16
#define MDM_TXESSEN_BIT ((uint32_t)0x00000800)
#define MDM_TXESSEN_POS 11
#define MDM_TXUNEQMODEN_BIT ((uint32_t)0x00000400)
#define MDM_TXUNEQMODEN_POS 10
#define MDM_TXSTBCEN_BIT ((uint32_t)0x00000200)
#define MDM_TXSTBCEN_POS 9
#define MDM_TXLDPCEN_BIT ((uint32_t)0x00000100)
#define MDM_TXLDPCEN_POS 8
#define MDM_TXNSSMAX_MASK ((uint32_t)0x00000070)
#define MDM_TXNSSMAX_LSB 4
#define MDM_TXNSSMAX_WIDTH ((uint32_t)0x00000003)
#define MDM_TXGFEN_BIT ((uint32_t)0x00000008)
#define MDM_TXGFEN_POS 3
#define MDM_TXMMEN_BIT ((uint32_t)0x00000004)
#define MDM_TXMMEN_POS 2
#define MDM_TXVHTEN_BIT ((uint32_t)0x00000002)
#define MDM_TXVHTEN_POS 1
#define MDM_TXDSSSEN_BIT ((uint32_t)0x00000001)
#define MDM_TXDSSSEN_POS 0
#define MDM_TXCBWMAX_RST 0x2
#define MDM_NTXMAX_RST 0x1
#define MDM_TXMUMIMOEN_RST 0x0
#define MDM_TXESSEN_RST 0x0
#define MDM_TXUNEQMODEN_RST 0x0
#define MDM_TXSTBCEN_RST 0x1
#define MDM_TXLDPCEN_RST 0x1
#define MDM_TXNSSMAX_RST 0x1
#define MDM_TXGFEN_RST 0x1
#define MDM_TXMMEN_RST 0x1
#define MDM_TXVHTEN_RST 0x1
#define MDM_TXDSSSEN_RST 0x1
__INLINE void mdm_txmode_pack(uint8_t txcbwmax, uint8_t ntxmax, uint8_t txmumimoen, uint8_t txessen, uint8_t txuneqmoden, uint8_t txstbcen, uint8_t txldpcen, uint8_t txnssmax, uint8_t txgfen, uint8_t txmmen, uint8_t txvhten, uint8_t txdsssen)
{
    ASSERT_ERR((((uint32_t)txcbwmax << 24) & ~((uint32_t)0x03000000)) == 0);
    ASSERT_ERR((((uint32_t)ntxmax << 20) & ~((uint32_t)0x00700000)) == 0);
    ASSERT_ERR((((uint32_t)txmumimoen << 16) & ~((uint32_t)0x00010000)) == 0);
    ASSERT_ERR((((uint32_t)txessen << 11) & ~((uint32_t)0x00000800)) == 0);
    ASSERT_ERR((((uint32_t)txuneqmoden << 10) & ~((uint32_t)0x00000400)) == 0);
    ASSERT_ERR((((uint32_t)txstbcen << 9) & ~((uint32_t)0x00000200)) == 0);
    ASSERT_ERR((((uint32_t)txldpcen << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)txnssmax << 4) & ~((uint32_t)0x00000070)) == 0);
    ASSERT_ERR((((uint32_t)txgfen << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)txmmen << 2) & ~((uint32_t)0x00000004)) == 0);
    ASSERT_ERR((((uint32_t)txvhten << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)txdsssen << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, ((uint32_t)txcbwmax << 24) | ((uint32_t)ntxmax << 20) | ((uint32_t)txmumimoen << 16) | ((uint32_t)txessen << 11) | ((uint32_t)txuneqmoden << 10) | ((uint32_t)txstbcen << 9) | ((uint32_t)txldpcen << 8) | ((uint32_t)txnssmax << 4) | ((uint32_t)txgfen << 3) | ((uint32_t)txmmen << 2) | ((uint32_t)txvhten << 1) | ((uint32_t)txdsssen << 0));
}
__INLINE void mdm_txmode_unpack(uint8_t* txcbwmax, uint8_t* ntxmax, uint8_t* txmumimoen, uint8_t* txessen, uint8_t* txuneqmoden, uint8_t* txstbcen, uint8_t* txldpcen, uint8_t* txnssmax, uint8_t* txgfen, uint8_t* txmmen, uint8_t* txvhten, uint8_t* txdsssen)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    *txcbwmax = (localVal & ((uint32_t)0x03000000)) >> 24;
    *ntxmax = (localVal & ((uint32_t)0x00700000)) >> 20;
    *txmumimoen = (localVal & ((uint32_t)0x00010000)) >> 16;
    *txessen = (localVal & ((uint32_t)0x00000800)) >> 11;
    *txuneqmoden = (localVal & ((uint32_t)0x00000400)) >> 10;
    *txstbcen = (localVal & ((uint32_t)0x00000200)) >> 9;
    *txldpcen = (localVal & ((uint32_t)0x00000100)) >> 8;
    *txnssmax = (localVal & ((uint32_t)0x00000070)) >> 4;
    *txgfen = (localVal & ((uint32_t)0x00000008)) >> 3;
    *txmmen = (localVal & ((uint32_t)0x00000004)) >> 2;
    *txvhten = (localVal & ((uint32_t)0x00000002)) >> 1;
    *txdsssen = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_txcbwmax_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x03000000)) >> 24);
}
__INLINE void mdm_txcbwmax_setf(uint8_t txcbwmax)
{
    ASSERT_ERR((((uint32_t)txcbwmax << 24) & ~((uint32_t)0x03000000)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x03000000)) | ((uint32_t)txcbwmax << 24));
}
__INLINE uint8_t mdm_ntxmax_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00700000)) >> 20);
}
__INLINE void mdm_ntxmax_setf(uint8_t ntxmax)
{
    ASSERT_ERR((((uint32_t)ntxmax << 20) & ~((uint32_t)0x00700000)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00700000)) | ((uint32_t)ntxmax << 20));
}
__INLINE uint8_t mdm_txmumimoen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00010000)) >> 16);
}
__INLINE void mdm_txmumimoen_setf(uint8_t txmumimoen)
{
    ASSERT_ERR((((uint32_t)txmumimoen << 16) & ~((uint32_t)0x00010000)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00010000)) | ((uint32_t)txmumimoen << 16));
}
__INLINE uint8_t mdm_txessen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000800)) >> 11);
}
__INLINE void mdm_txessen_setf(uint8_t txessen)
{
    ASSERT_ERR((((uint32_t)txessen << 11) & ~((uint32_t)0x00000800)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000800)) | ((uint32_t)txessen << 11));
}
__INLINE uint8_t mdm_txuneqmoden_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000400)) >> 10);
}
__INLINE void mdm_txuneqmoden_setf(uint8_t txuneqmoden)
{
    ASSERT_ERR((((uint32_t)txuneqmoden << 10) & ~((uint32_t)0x00000400)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000400)) | ((uint32_t)txuneqmoden << 10));
}
__INLINE uint8_t mdm_txstbcen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000200)) >> 9);
}
__INLINE void mdm_txstbcen_setf(uint8_t txstbcen)
{
    ASSERT_ERR((((uint32_t)txstbcen << 9) & ~((uint32_t)0x00000200)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000200)) | ((uint32_t)txstbcen << 9));
}
__INLINE uint8_t mdm_txldpcen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}
__INLINE void mdm_txldpcen_setf(uint8_t txldpcen)
{
    ASSERT_ERR((((uint32_t)txldpcen << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000100)) | ((uint32_t)txldpcen << 8));
}
__INLINE uint8_t mdm_txnssmax_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000070)) >> 4);
}
__INLINE void mdm_txnssmax_setf(uint8_t txnssmax)
{
    ASSERT_ERR((((uint32_t)txnssmax << 4) & ~((uint32_t)0x00000070)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000070)) | ((uint32_t)txnssmax << 4));
}
__INLINE uint8_t mdm_txgfen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void mdm_txgfen_setf(uint8_t txgfen)
{
    ASSERT_ERR((((uint32_t)txgfen << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000008)) | ((uint32_t)txgfen << 3));
}
__INLINE uint8_t mdm_txmmen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}
__INLINE void mdm_txmmen_setf(uint8_t txmmen)
{
    ASSERT_ERR((((uint32_t)txmmen << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000004)) | ((uint32_t)txmmen << 2));
}
__INLINE uint8_t mdm_txvhten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE void mdm_txvhten_setf(uint8_t txvhten)
{
    ASSERT_ERR((((uint32_t)txvhten << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000002)) | ((uint32_t)txvhten << 1));
}
__INLINE uint8_t mdm_txdsssen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXMODE_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_txdsssen_setf(uint8_t txdsssen)
{
    ASSERT_ERR((((uint32_t)txdsssen << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_TXMODE_ADDR, (REG_PL_RD(MDM_TXMODE_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)txdsssen << 0));
}
#define MDM_EQUALCTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x0828)
#define MDM_EQUALCTRL1_OFFSET 0x00000028
#define MDM_EQUALCTRL1_INDEX 0x0000000A
#define MDM_EQUALCTRL1_RESET 0x11101028
__INLINE uint32_t mdm_equalctrl1_get(void)
{
    return REG_PL_RD(MDM_EQUALCTRL1_ADDR);
}
__INLINE void mdm_equalctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, value);
}
#define MDM_SATSB_LDPCNSTS1_MASK ((uint32_t)0x70000000)
#define MDM_SATSB_LDPCNSTS1_LSB 28
#define MDM_SATSB_LDPCNSTS1_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_BCCNSTS1_MASK ((uint32_t)0x07000000)
#define MDM_SATSB_BCCNSTS1_LSB 24
#define MDM_SATSB_BCCNSTS1_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_LDPCNSTS0_MASK ((uint32_t)0x00700000)
#define MDM_SATSB_LDPCNSTS0_LSB 20
#define MDM_SATSB_LDPCNSTS0_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_BCCNSTS0_MASK ((uint32_t)0x00070000)
#define MDM_SATSB_BCCNSTS0_LSB 16
#define MDM_SATSB_BCCNSTS0_WIDTH ((uint32_t)0x00000003)
#define MDM_MHSHIFTVAL_MASK ((uint32_t)0x00007000)
#define MDM_MHSHIFTVAL_LSB 12
#define MDM_MHSHIFTVAL_WIDTH ((uint32_t)0x00000003)
#define MDM_MHSHIFTTHR_MASK ((uint32_t)0x000000FF)
#define MDM_MHSHIFTTHR_LSB 0
#define MDM_MHSHIFTTHR_WIDTH ((uint32_t)0x00000008)
#define MDM_SATSB_LDPCNSTS1_RST 0x1
#define MDM_SATSB_BCCNSTS1_RST 0x1
#define MDM_SATSB_LDPCNSTS0_RST 0x1
#define MDM_SATSB_BCCNSTS0_RST 0x0
#define MDM_MHSHIFTVAL_RST 0x1
#define MDM_MHSHIFTTHR_RST 0x28
__INLINE void mdm_equalctrl1_pack(uint8_t satsbldpcnsts1, uint8_t satsbbccnsts1, uint8_t satsbldpcnsts0, uint8_t satsbbccnsts0, uint8_t mhshiftval, uint8_t mhshiftthr)
{
    ASSERT_ERR((((uint32_t)satsbldpcnsts1 << 28) & ~((uint32_t)0x70000000)) == 0);
    ASSERT_ERR((((uint32_t)satsbbccnsts1 << 24) & ~((uint32_t)0x07000000)) == 0);
    ASSERT_ERR((((uint32_t)satsbldpcnsts0 << 20) & ~((uint32_t)0x00700000)) == 0);
    ASSERT_ERR((((uint32_t)satsbbccnsts0 << 16) & ~((uint32_t)0x00070000)) == 0);
    ASSERT_ERR((((uint32_t)mhshiftval << 12) & ~((uint32_t)0x00007000)) == 0);
    ASSERT_ERR((((uint32_t)mhshiftthr << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, ((uint32_t)satsbldpcnsts1 << 28) | ((uint32_t)satsbbccnsts1 << 24) | ((uint32_t)satsbldpcnsts0 << 20) | ((uint32_t)satsbbccnsts0 << 16) | ((uint32_t)mhshiftval << 12) | ((uint32_t)mhshiftthr << 0));
}
__INLINE void mdm_equalctrl1_unpack(uint8_t* satsbldpcnsts1, uint8_t* satsbbccnsts1, uint8_t* satsbldpcnsts0, uint8_t* satsbbccnsts0, uint8_t* mhshiftval, uint8_t* mhshiftthr)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    *satsbldpcnsts1 = (localVal & ((uint32_t)0x70000000)) >> 28;
    *satsbbccnsts1 = (localVal & ((uint32_t)0x07000000)) >> 24;
    *satsbldpcnsts0 = (localVal & ((uint32_t)0x00700000)) >> 20;
    *satsbbccnsts0 = (localVal & ((uint32_t)0x00070000)) >> 16;
    *mhshiftval = (localVal & ((uint32_t)0x00007000)) >> 12;
    *mhshiftthr = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_satsb_ldpcnsts1_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x70000000)) >> 28);
}
__INLINE void mdm_satsb_ldpcnsts1_setf(uint8_t satsbldpcnsts1)
{
    ASSERT_ERR((((uint32_t)satsbldpcnsts1 << 28) & ~((uint32_t)0x70000000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, (REG_PL_RD(MDM_EQUALCTRL1_ADDR) & ~((uint32_t)0x70000000)) | ((uint32_t)satsbldpcnsts1 << 28));
}
__INLINE uint8_t mdm_satsb_bccnsts1_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x07000000)) >> 24);
}
__INLINE void mdm_satsb_bccnsts1_setf(uint8_t satsbbccnsts1)
{
    ASSERT_ERR((((uint32_t)satsbbccnsts1 << 24) & ~((uint32_t)0x07000000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, (REG_PL_RD(MDM_EQUALCTRL1_ADDR) & ~((uint32_t)0x07000000)) | ((uint32_t)satsbbccnsts1 << 24));
}
__INLINE uint8_t mdm_satsb_ldpcnsts0_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00700000)) >> 20);
}
__INLINE void mdm_satsb_ldpcnsts0_setf(uint8_t satsbldpcnsts0)
{
    ASSERT_ERR((((uint32_t)satsbldpcnsts0 << 20) & ~((uint32_t)0x00700000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, (REG_PL_RD(MDM_EQUALCTRL1_ADDR) & ~((uint32_t)0x00700000)) | ((uint32_t)satsbldpcnsts0 << 20));
}
__INLINE uint8_t mdm_satsb_bccnsts0_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00070000)) >> 16);
}
__INLINE void mdm_satsb_bccnsts0_setf(uint8_t satsbbccnsts0)
{
    ASSERT_ERR((((uint32_t)satsbbccnsts0 << 16) & ~((uint32_t)0x00070000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, (REG_PL_RD(MDM_EQUALCTRL1_ADDR) & ~((uint32_t)0x00070000)) | ((uint32_t)satsbbccnsts0 << 16));
}
__INLINE uint8_t mdm_mhshiftval_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00007000)) >> 12);
}
__INLINE void mdm_mhshiftval_setf(uint8_t mhshiftval)
{
    ASSERT_ERR((((uint32_t)mhshiftval << 12) & ~((uint32_t)0x00007000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, (REG_PL_RD(MDM_EQUALCTRL1_ADDR) & ~((uint32_t)0x00007000)) | ((uint32_t)mhshiftval << 12));
}
__INLINE uint8_t mdm_mhshiftthr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_mhshiftthr_setf(uint8_t mhshiftthr)
{
    ASSERT_ERR((((uint32_t)mhshiftthr << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_EQUALCTRL1_ADDR, (REG_PL_RD(MDM_EQUALCTRL1_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)mhshiftthr << 0));
}
#if RW_MUMIMO_RX_EN
#define MDM_EQUALCTRL2_ADDR (REG_MDM_CFG_BASE_ADDR+0x082C)
#define MDM_EQUALCTRL2_OFFSET 0x0000002C
#define MDM_EQUALCTRL2_INDEX 0x0000000B
#define MDM_EQUALCTRL2_RESET 0x21010000
__INLINE uint32_t mdm_equalctrl2_get(void)
{
    return REG_PL_RD(MDM_EQUALCTRL2_ADDR);
}
__INLINE void mdm_equalctrl2_set(uint32_t value)
{
    REG_PL_WR(MDM_EQUALCTRL2_ADDR, value);
}
#define MDM_SATSB_MU_LDPCNSTS1_MASK ((uint32_t)0x70000000)
#define MDM_SATSB_MU_LDPCNSTS1_LSB 28
#define MDM_SATSB_MU_LDPCNSTS1_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_MU_BCCNSTS1_MASK ((uint32_t)0x07000000)
#define MDM_SATSB_MU_BCCNSTS1_LSB 24
#define MDM_SATSB_MU_BCCNSTS1_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_MU_LDPCNSTS0_MASK ((uint32_t)0x00700000)
#define MDM_SATSB_MU_LDPCNSTS0_LSB 20
#define MDM_SATSB_MU_LDPCNSTS0_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_MU_BCCNSTS0_MASK ((uint32_t)0x00070000)
#define MDM_SATSB_MU_BCCNSTS0_LSB 16
#define MDM_SATSB_MU_BCCNSTS0_WIDTH ((uint32_t)0x00000003)
#define MDM_SATSB_MU_LDPCNSTS1_RST 0x2
#define MDM_SATSB_MU_BCCNSTS1_RST 0x1
#define MDM_SATSB_MU_LDPCNSTS0_RST 0x0
#define MDM_SATSB_MU_BCCNSTS0_RST 0x1
__INLINE void mdm_equalctrl2_pack(uint8_t satsbmuldpcnsts1, uint8_t satsbmubccnsts1, uint8_t satsbmuldpcnsts0, uint8_t satsbmubccnsts0)
{
    ASSERT_ERR((((uint32_t)satsbmuldpcnsts1 << 28) & ~((uint32_t)0x70000000)) == 0);
    ASSERT_ERR((((uint32_t)satsbmubccnsts1 << 24) & ~((uint32_t)0x07000000)) == 0);
    ASSERT_ERR((((uint32_t)satsbmuldpcnsts0 << 20) & ~((uint32_t)0x00700000)) == 0);
    ASSERT_ERR((((uint32_t)satsbmubccnsts0 << 16) & ~((uint32_t)0x00070000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL2_ADDR, ((uint32_t)satsbmuldpcnsts1 << 28) | ((uint32_t)satsbmubccnsts1 << 24) | ((uint32_t)satsbmuldpcnsts0 << 20) | ((uint32_t)satsbmubccnsts0 << 16));
}
__INLINE void mdm_equalctrl2_unpack(uint8_t* satsbmuldpcnsts1, uint8_t* satsbmubccnsts1, uint8_t* satsbmuldpcnsts0, uint8_t* satsbmubccnsts0)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL2_ADDR);
    *satsbmuldpcnsts1 = (localVal & ((uint32_t)0x70000000)) >> 28;
    *satsbmubccnsts1 = (localVal & ((uint32_t)0x07000000)) >> 24;
    *satsbmuldpcnsts0 = (localVal & ((uint32_t)0x00700000)) >> 20;
    *satsbmubccnsts0 = (localVal & ((uint32_t)0x00070000)) >> 16;
}
__INLINE uint8_t mdm_satsb_mu_ldpcnsts1_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x70000000)) >> 28);
}
__INLINE void mdm_satsb_mu_ldpcnsts1_setf(uint8_t satsbmuldpcnsts1)
{
    ASSERT_ERR((((uint32_t)satsbmuldpcnsts1 << 28) & ~((uint32_t)0x70000000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL2_ADDR, (REG_PL_RD(MDM_EQUALCTRL2_ADDR) & ~((uint32_t)0x70000000)) | ((uint32_t)satsbmuldpcnsts1 << 28));
}
__INLINE uint8_t mdm_satsb_mu_bccnsts1_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x07000000)) >> 24);
}
__INLINE void mdm_satsb_mu_bccnsts1_setf(uint8_t satsbmubccnsts1)
{
    ASSERT_ERR((((uint32_t)satsbmubccnsts1 << 24) & ~((uint32_t)0x07000000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL2_ADDR, (REG_PL_RD(MDM_EQUALCTRL2_ADDR) & ~((uint32_t)0x07000000)) | ((uint32_t)satsbmubccnsts1 << 24));
}
__INLINE uint8_t mdm_satsb_mu_ldpcnsts0_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x00700000)) >> 20);
}
__INLINE void mdm_satsb_mu_ldpcnsts0_setf(uint8_t satsbmuldpcnsts0)
{
    ASSERT_ERR((((uint32_t)satsbmuldpcnsts0 << 20) & ~((uint32_t)0x00700000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL2_ADDR, (REG_PL_RD(MDM_EQUALCTRL2_ADDR) & ~((uint32_t)0x00700000)) | ((uint32_t)satsbmuldpcnsts0 << 20));
}
__INLINE uint8_t mdm_satsb_mu_bccnsts0_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_EQUALCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x00070000)) >> 16);
}
__INLINE void mdm_satsb_mu_bccnsts0_setf(uint8_t satsbmubccnsts0)
{
    ASSERT_ERR((((uint32_t)satsbmubccnsts0 << 16) & ~((uint32_t)0x00070000)) == 0);
    REG_PL_WR(MDM_EQUALCTRL2_ADDR, (REG_PL_RD(MDM_EQUALCTRL2_ADDR) & ~((uint32_t)0x00070000)) | ((uint32_t)satsbmubccnsts0 << 16));
}
#endif
#define MDM_SMOOTHSNRTHR_ADDR (REG_MDM_CFG_BASE_ADDR+0x0830)
#define MDM_SMOOTHSNRTHR_OFFSET 0x00000030
#define MDM_SMOOTHSNRTHR_INDEX 0x0000000C
#define MDM_SMOOTHSNRTHR_RESET 0x00001B0F
__INLINE uint32_t mdm_smoothsnrthr_get(void)
{
    return REG_PL_RD(MDM_SMOOTHSNRTHR_ADDR);
}
__INLINE void mdm_smoothsnrthr_set(uint32_t value)
{
    REG_PL_WR(MDM_SMOOTHSNRTHR_ADDR, value);
}
#define MDM_SMOOTHSNRTHRHIGH_MASK ((uint32_t)0x0000FF00)
#define MDM_SMOOTHSNRTHRHIGH_LSB 8
#define MDM_SMOOTHSNRTHRHIGH_WIDTH ((uint32_t)0x00000008)
#define MDM_SMOOTHSNRTHRMID_MASK ((uint32_t)0x000000FF)
#define MDM_SMOOTHSNRTHRMID_LSB 0
#define MDM_SMOOTHSNRTHRMID_WIDTH ((uint32_t)0x00000008)
#define MDM_SMOOTHSNRTHRHIGH_RST 0x1B
#define MDM_SMOOTHSNRTHRMID_RST 0xF
__INLINE void mdm_smoothsnrthr_pack(uint8_t smoothsnrthrhigh, uint8_t smoothsnrthrmid)
{
    ASSERT_ERR((((uint32_t)smoothsnrthrhigh << 8) & ~((uint32_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint32_t)smoothsnrthrmid << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_SMOOTHSNRTHR_ADDR, ((uint32_t)smoothsnrthrhigh << 8) | ((uint32_t)smoothsnrthrmid << 0));
}
__INLINE void mdm_smoothsnrthr_unpack(uint8_t* smoothsnrthrhigh, uint8_t* smoothsnrthrmid)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHSNRTHR_ADDR);
    *smoothsnrthrhigh = (localVal & ((uint32_t)0x0000FF00)) >> 8;
    *smoothsnrthrmid = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_smoothsnrthrhigh_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHSNRTHR_ADDR);
    return ((localVal & ((uint32_t)0x0000FF00)) >> 8);
}
__INLINE void mdm_smoothsnrthrhigh_setf(uint8_t smoothsnrthrhigh)
{
    ASSERT_ERR((((uint32_t)smoothsnrthrhigh << 8) & ~((uint32_t)0x0000FF00)) == 0);
    REG_PL_WR(MDM_SMOOTHSNRTHR_ADDR, (REG_PL_RD(MDM_SMOOTHSNRTHR_ADDR) & ~((uint32_t)0x0000FF00)) | ((uint32_t)smoothsnrthrhigh << 8));
}
__INLINE uint8_t mdm_smoothsnrthrmid_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHSNRTHR_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_smoothsnrthrmid_setf(uint8_t smoothsnrthrmid)
{
    ASSERT_ERR((((uint32_t)smoothsnrthrmid << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_SMOOTHSNRTHR_ADDR, (REG_PL_RD(MDM_SMOOTHSNRTHR_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)smoothsnrthrmid << 0));
}
#define MDM_RXTDCTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x0834)
#define MDM_RXTDCTRL1_OFFSET 0x00000034
#define MDM_RXTDCTRL1_INDEX 0x0000000D
#define MDM_RXTDCTRL1_RESET 0x06160100
__INLINE uint32_t mdm_rxtdctrl1_get(void)
{
    return REG_PL_RD(MDM_RXTDCTRL1_ADDR);
}
__INLINE void mdm_rxtdctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, value);
}
#define MDM_TDDCHTSTFMARGIN_MASK ((uint32_t)0xFF000000)
#define MDM_TDDCHTSTFMARGIN_LSB 24
#define MDM_TDDCHTSTFMARGIN_WIDTH ((uint32_t)0x00000008)
#define MDM_TDBWDETWEIGHT_MASK ((uint32_t)0x001F0000)
#define MDM_TDBWDETWEIGHT_LSB 16
#define MDM_TDBWDETWEIGHT_WIDTH ((uint32_t)0x00000005)
#define MDM_RXTDDCEN_BIT ((uint32_t)0x00000100)
#define MDM_RXTDDCEN_POS 8
#define MDM_TDBWFORCE40_BIT ((uint32_t)0x00000020)
#define MDM_TDBWFORCE40_POS 5
#define MDM_TDBWFORCE20_BIT ((uint32_t)0x00000010)
#define MDM_TDBWFORCE20_POS 4
#define MDM_AGCRSTMODE_BIT ((uint32_t)0x00000001)
#define MDM_AGCRSTMODE_POS 0
#define MDM_TDDCHTSTFMARGIN_RST 0x6
#define MDM_TDBWDETWEIGHT_RST 0x16
#define MDM_RXTDDCEN_RST 0x1
#define MDM_TDBWFORCE40_RST 0x0
#define MDM_TDBWFORCE20_RST 0x0
#define MDM_AGCRSTMODE_RST 0x0
__INLINE void mdm_rxtdctrl1_pack(uint8_t tddchtstfmargin, uint8_t tdbwdetweight, uint8_t rxtddcen, uint8_t tdbwforce40, uint8_t tdbwforce20, uint8_t agcrstmode)
{
    ASSERT_ERR((((uint32_t)tddchtstfmargin << 24) & ~((uint32_t)0xFF000000)) == 0);
    ASSERT_ERR((((uint32_t)tdbwdetweight << 16) & ~((uint32_t)0x001F0000)) == 0);
    ASSERT_ERR((((uint32_t)rxtddcen << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)tdbwforce40 << 5) & ~((uint32_t)0x00000020)) == 0);
    ASSERT_ERR((((uint32_t)tdbwforce20 << 4) & ~((uint32_t)0x00000010)) == 0);
    ASSERT_ERR((((uint32_t)agcrstmode << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, ((uint32_t)tddchtstfmargin << 24) | ((uint32_t)tdbwdetweight << 16) | ((uint32_t)rxtddcen << 8) | ((uint32_t)tdbwforce40 << 5) | ((uint32_t)tdbwforce20 << 4) | ((uint32_t)agcrstmode << 0));
}
__INLINE void mdm_rxtdctrl1_unpack(uint8_t* tddchtstfmargin, uint8_t* tdbwdetweight, uint8_t* rxtddcen, uint8_t* tdbwforce40, uint8_t* tdbwforce20, uint8_t* agcrstmode)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    *tddchtstfmargin = (localVal & ((uint32_t)0xFF000000)) >> 24;
    *tdbwdetweight = (localVal & ((uint32_t)0x001F0000)) >> 16;
    *rxtddcen = (localVal & ((uint32_t)0x00000100)) >> 8;
    *tdbwforce40 = (localVal & ((uint32_t)0x00000020)) >> 5;
    *tdbwforce20 = (localVal & ((uint32_t)0x00000010)) >> 4;
    *agcrstmode = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_tddchtstfmargin_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    return ((localVal & ((uint32_t)0xFF000000)) >> 24);
}
__INLINE void mdm_tddchtstfmargin_setf(uint8_t tddchtstfmargin)
{
    ASSERT_ERR((((uint32_t)tddchtstfmargin << 24) & ~((uint32_t)0xFF000000)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, (REG_PL_RD(MDM_RXTDCTRL1_ADDR) & ~((uint32_t)0xFF000000)) | ((uint32_t)tddchtstfmargin << 24));
}
__INLINE uint8_t mdm_tdbwdetweight_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x001F0000)) >> 16);
}
__INLINE void mdm_tdbwdetweight_setf(uint8_t tdbwdetweight)
{
    ASSERT_ERR((((uint32_t)tdbwdetweight << 16) & ~((uint32_t)0x001F0000)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, (REG_PL_RD(MDM_RXTDCTRL1_ADDR) & ~((uint32_t)0x001F0000)) | ((uint32_t)tdbwdetweight << 16));
}
__INLINE uint8_t mdm_rxtddcen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}
__INLINE void mdm_rxtddcen_setf(uint8_t rxtddcen)
{
    ASSERT_ERR((((uint32_t)rxtddcen << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, (REG_PL_RD(MDM_RXTDCTRL1_ADDR) & ~((uint32_t)0x00000100)) | ((uint32_t)rxtddcen << 8));
}
__INLINE uint8_t mdm_tdbwforce40_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000020)) >> 5);
}
__INLINE void mdm_tdbwforce40_setf(uint8_t tdbwforce40)
{
    ASSERT_ERR((((uint32_t)tdbwforce40 << 5) & ~((uint32_t)0x00000020)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, (REG_PL_RD(MDM_RXTDCTRL1_ADDR) & ~((uint32_t)0x00000020)) | ((uint32_t)tdbwforce40 << 5));
}
__INLINE uint8_t mdm_tdbwforce20_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000010)) >> 4);
}
__INLINE void mdm_tdbwforce20_setf(uint8_t tdbwforce20)
{
    ASSERT_ERR((((uint32_t)tdbwforce20 << 4) & ~((uint32_t)0x00000010)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, (REG_PL_RD(MDM_RXTDCTRL1_ADDR) & ~((uint32_t)0x00000010)) | ((uint32_t)tdbwforce20 << 4));
}
__INLINE uint8_t mdm_agcrstmode_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXTDCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_agcrstmode_setf(uint8_t agcrstmode)
{
    ASSERT_ERR((((uint32_t)agcrstmode << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_RXTDCTRL1_ADDR, (REG_PL_RD(MDM_RXTDCTRL1_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)agcrstmode << 0));
}
#define MDM_TXCTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0838)
#define MDM_TXCTRL0_OFFSET 0x00000038
#define MDM_TXCTRL0_INDEX 0x0000000E
#define MDM_TXCTRL0_RESET 0x000000B4
__INLINE uint32_t mdm_txctrl0_get(void)
{
    return REG_PL_RD(MDM_TXCTRL0_ADDR);
}
__INLINE void mdm_txctrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_TXCTRL0_ADDR, value);
}
#define MDM_TXSTARTDELAY_MASK ((uint32_t)0x000003FF)
#define MDM_TXSTARTDELAY_LSB 0
#define MDM_TXSTARTDELAY_WIDTH ((uint32_t)0x0000000A)
#define MDM_TXSTARTDELAY_RST 0xB4
__INLINE uint16_t mdm_txstartdelay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL0_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x000003FF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_txstartdelay_setf(uint16_t txstartdelay)
{
    ASSERT_ERR((((uint32_t)txstartdelay << 0) & ~((uint32_t)0x000003FF)) == 0);
    REG_PL_WR(MDM_TXCTRL0_ADDR, (uint32_t)txstartdelay << 0);
}
#endif
#define MDM_RXCTRL1_ADDR(band) (REG_MDM_CFG_BASE_ADDR(band)+0x083C)
#define MDM_RXCTRL1_OFFSET 0x0000003C
#define MDM_RXCTRL1_INDEX 0x0000000F
#define MDM_RXCTRL1_RESET 0x0C300C30
#if 0
__INLINE uint32_t mdm_rxctrl1_get(void)
{
    return REG_PL_RD(MDM_RXCTRL1_ADDR);
}
__INLINE void mdm_rxctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_RXCTRL1_ADDR, value);
}
#define MDM_RXBCCNDBPSMAXSGI_MASK ((uint32_t)0xFFFF0000)
#define MDM_RXBCCNDBPSMAXSGI_LSB 16
#define MDM_RXBCCNDBPSMAXSGI_WIDTH ((uint32_t)0x00000010)
#define MDM_RXBCCNDBPSMAXLGI_MASK ((uint32_t)0x0000FFFF)
#define MDM_RXBCCNDBPSMAXLGI_LSB 0
#define MDM_RXBCCNDBPSMAXLGI_WIDTH ((uint32_t)0x00000010)
#define MDM_RXBCCNDBPSMAXSGI_RST 0xC30
#define MDM_RXBCCNDBPSMAXLGI_RST 0xC30
__INLINE void mdm_rxctrl1_pack(uint16_t rxbccndbpsmaxsgi, uint16_t rxbccndbpsmaxlgi)
{
    ASSERT_ERR((((uint32_t)rxbccndbpsmaxsgi << 16) & ~((uint32_t)0xFFFF0000)) == 0);
    ASSERT_ERR((((uint32_t)rxbccndbpsmaxlgi << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(MDM_RXCTRL1_ADDR, ((uint32_t)rxbccndbpsmaxsgi << 16) | ((uint32_t)rxbccndbpsmaxlgi << 0));
}
__INLINE void mdm_rxctrl1_unpack(uint16_t* rxbccndbpsmaxsgi, uint16_t* rxbccndbpsmaxlgi)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL1_ADDR);
    *rxbccndbpsmaxsgi = (localVal & ((uint32_t)0xFFFF0000)) >> 16;
    *rxbccndbpsmaxlgi = (localVal & ((uint32_t)0x0000FFFF)) >> 0;
}
__INLINE uint16_t mdm_rxbccndbpsmaxsgi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL1_ADDR);
    return ((localVal & ((uint32_t)0xFFFF0000)) >> 16);
}
__INLINE void mdm_rxbccndbpsmaxsgi_setf(uint16_t rxbccndbpsmaxsgi)
{
    ASSERT_ERR((((uint32_t)rxbccndbpsmaxsgi << 16) & ~((uint32_t)0xFFFF0000)) == 0);
    REG_PL_WR(MDM_RXCTRL1_ADDR, (REG_PL_RD(MDM_RXCTRL1_ADDR) & ~((uint32_t)0xFFFF0000)) | ((uint32_t)rxbccndbpsmaxsgi << 16));
}
__INLINE uint16_t mdm_rxbccndbpsmaxlgi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x0000FFFF)) >> 0);
}
__INLINE void mdm_rxbccndbpsmaxlgi_setf(uint16_t rxbccndbpsmaxlgi)
{
    ASSERT_ERR((((uint32_t)rxbccndbpsmaxlgi << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(MDM_RXCTRL1_ADDR, (REG_PL_RD(MDM_RXCTRL1_ADDR) & ~((uint32_t)0x0000FFFF)) | ((uint32_t)rxbccndbpsmaxlgi << 0));
}
#endif
#if 0
#define MDM_SMOOTHFORCECTRL_ADDR (REG_MDM_CFG_BASE_ADDR + 0x0840)
#define MDM_SMOOTHFORCECTRL_OFFSET 0x00000040
#define MDM_SMOOTHFORCECTRL_INDEX 0x00000010
#define MDM_SMOOTHFORCECTRL_RESET 0x03310000
__INLINE uint32_t mdm_smoothforcectrl_get(void)
{
    return REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
}
__INLINE void mdm_smoothforcectrl_set(uint32_t value)
{
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, value);
}
#define MDM_MUSMOOTHFORCE_BIT ((uint32_t)0x20000000)
#define MDM_MUSMOOTHFORCE_POS 29
#define MDM_BFSMOOTHFORCE_BIT ((uint32_t)0x10000000)
#define MDM_BFSMOOTHFORCE_POS 28
#define MDM_CFGVHTSTS2SMOOTHFORCE_BIT ((uint32_t)0x02000000)
#define MDM_CFGVHTSTS2SMOOTHFORCE_POS 25
#define MDM_CFGVHTSTS1SMOOTHFORCE_BIT ((uint32_t)0x01000000)
#define MDM_CFGVHTSTS1SMOOTHFORCE_POS 24
#define MDM_CFGHTSTS2SMOOTHFORCE_BIT ((uint32_t)0x00200000)
#define MDM_CFGHTSTS2SMOOTHFORCE_POS 21
#define MDM_CFGHTSTS1SMOOTHFORCE_BIT ((uint32_t)0x00100000)
#define MDM_CFGHTSTS1SMOOTHFORCE_POS 20
#define MDM_CFGNONHTSMOOTHFORCE_BIT ((uint32_t)0x00010000)
#define MDM_CFGNONHTSMOOTHFORCE_POS 16
#define MDM_CFGVHTSTS2SMOOTH_MASK ((uint32_t)0x00000C00)
#define MDM_CFGVHTSTS2SMOOTH_LSB 10
#define MDM_CFGVHTSTS2SMOOTH_WIDTH ((uint32_t)0x00000002)
#define MDM_CFGVHTSTS1SMOOTH_MASK ((uint32_t)0x00000300)
#define MDM_CFGVHTSTS1SMOOTH_LSB 8
#define MDM_CFGVHTSTS1SMOOTH_WIDTH ((uint32_t)0x00000002)
#define MDM_CFGHTSTS2SMOOTH_MASK ((uint32_t)0x000000C0)
#define MDM_CFGHTSTS2SMOOTH_LSB 6
#define MDM_CFGHTSTS2SMOOTH_WIDTH ((uint32_t)0x00000002)
#define MDM_CFGHTSTS1SMOOTH_MASK ((uint32_t)0x00000030)
#define MDM_CFGHTSTS1SMOOTH_LSB 4
#define MDM_CFGHTSTS1SMOOTH_WIDTH ((uint32_t)0x00000002)
#define MDM_CFGNONHTSMOOTH_MASK ((uint32_t)0x00000003)
#define MDM_CFGNONHTSMOOTH_LSB 0
#define MDM_CFGNONHTSMOOTH_WIDTH ((uint32_t)0x00000002)
#define MDM_MUSMOOTHFORCE_RST 0x0
#define MDM_BFSMOOTHFORCE_RST 0x0
#define MDM_CFGVHTSTS2SMOOTHFORCE_RST 0x1
#define MDM_CFGVHTSTS1SMOOTHFORCE_RST 0x1
#define MDM_CFGHTSTS2SMOOTHFORCE_RST 0x1
#define MDM_CFGHTSTS1SMOOTHFORCE_RST 0x1
#define MDM_CFGNONHTSMOOTHFORCE_RST 0x1
#define MDM_CFGVHTSTS2SMOOTH_RST 0x0
#define MDM_CFGVHTSTS1SMOOTH_RST 0x0
#define MDM_CFGHTSTS2SMOOTH_RST 0x0
#define MDM_CFGHTSTS1SMOOTH_RST 0x0
#define MDM_CFGNONHTSMOOTH_RST 0x0
__INLINE void mdm_smoothforcectrl_pack(uint8_t musmoothforce, uint8_t bfsmoothforce, uint8_t cfgvhtsts2smoothforce, uint8_t cfgvhtsts1smoothforce, uint8_t cfghtsts2smoothforce, uint8_t cfghtsts1smoothforce, uint8_t cfgnonhtsmoothforce, uint8_t cfgvhtsts2smooth, uint8_t cfgvhtsts1smooth, uint8_t cfghtsts2smooth, uint8_t cfghtsts1smooth, uint8_t cfgnonhtsmooth)
{
    ASSERT_ERR((((uint32_t)musmoothforce << 29) & ~((uint32_t)0x20000000)) == 0);
    ASSERT_ERR((((uint32_t)bfsmoothforce << 28) & ~((uint32_t)0x10000000)) == 0);
    ASSERT_ERR((((uint32_t)cfgvhtsts2smoothforce << 25) & ~((uint32_t)0x02000000)) == 0);
    ASSERT_ERR((((uint32_t)cfgvhtsts1smoothforce << 24) & ~((uint32_t)0x01000000)) == 0);
    ASSERT_ERR((((uint32_t)cfghtsts2smoothforce << 21) & ~((uint32_t)0x00200000)) == 0);
    ASSERT_ERR((((uint32_t)cfghtsts1smoothforce << 20) & ~((uint32_t)0x00100000)) == 0);
    ASSERT_ERR((((uint32_t)cfgnonhtsmoothforce << 16) & ~((uint32_t)0x00010000)) == 0);
    ASSERT_ERR((((uint32_t)cfgvhtsts2smooth << 10) & ~((uint32_t)0x00000C00)) == 0);
    ASSERT_ERR((((uint32_t)cfgvhtsts1smooth << 8) & ~((uint32_t)0x00000300)) == 0);
    ASSERT_ERR((((uint32_t)cfghtsts2smooth << 6) & ~((uint32_t)0x000000C0)) == 0);
    ASSERT_ERR((((uint32_t)cfghtsts1smooth << 4) & ~((uint32_t)0x00000030)) == 0);
    ASSERT_ERR((((uint32_t)cfgnonhtsmooth << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, ((uint32_t)musmoothforce << 29) | ((uint32_t)bfsmoothforce << 28) | ((uint32_t)cfgvhtsts2smoothforce << 25) | ((uint32_t)cfgvhtsts1smoothforce << 24) | ((uint32_t)cfghtsts2smoothforce << 21) | ((uint32_t)cfghtsts1smoothforce << 20) | ((uint32_t)cfgnonhtsmoothforce << 16) | ((uint32_t)cfgvhtsts2smooth << 10) | ((uint32_t)cfgvhtsts1smooth << 8) | ((uint32_t)cfghtsts2smooth << 6) | ((uint32_t)cfghtsts1smooth << 4) | ((uint32_t)cfgnonhtsmooth << 0));
}
__INLINE void mdm_smoothforcectrl_unpack(uint8_t* musmoothforce, uint8_t* bfsmoothforce, uint8_t* cfgvhtsts2smoothforce, uint8_t* cfgvhtsts1smoothforce, uint8_t* cfghtsts2smoothforce, uint8_t* cfghtsts1smoothforce, uint8_t* cfgnonhtsmoothforce, uint8_t* cfgvhtsts2smooth, uint8_t* cfgvhtsts1smooth, uint8_t* cfghtsts2smooth, uint8_t* cfghtsts1smooth, uint8_t* cfgnonhtsmooth)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    *musmoothforce = (localVal & ((uint32_t)0x20000000)) >> 29;
    *bfsmoothforce = (localVal & ((uint32_t)0x10000000)) >> 28;
    *cfgvhtsts2smoothforce = (localVal & ((uint32_t)0x02000000)) >> 25;
    *cfgvhtsts1smoothforce = (localVal & ((uint32_t)0x01000000)) >> 24;
    *cfghtsts2smoothforce = (localVal & ((uint32_t)0x00200000)) >> 21;
    *cfghtsts1smoothforce = (localVal & ((uint32_t)0x00100000)) >> 20;
    *cfgnonhtsmoothforce = (localVal & ((uint32_t)0x00010000)) >> 16;
    *cfgvhtsts2smooth = (localVal & ((uint32_t)0x00000C00)) >> 10;
    *cfgvhtsts1smooth = (localVal & ((uint32_t)0x00000300)) >> 8;
    *cfghtsts2smooth = (localVal & ((uint32_t)0x000000C0)) >> 6;
    *cfghtsts1smooth = (localVal & ((uint32_t)0x00000030)) >> 4;
    *cfgnonhtsmooth = (localVal & ((uint32_t)0x00000003)) >> 0;
}
__INLINE uint8_t mdm_musmoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x20000000)) >> 29);
}
__INLINE void mdm_musmoothforce_setf(uint8_t musmoothforce)
{
    ASSERT_ERR((((uint32_t)musmoothforce << 29) & ~((uint32_t)0x20000000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x20000000)) | ((uint32_t)musmoothforce << 29));
}
__INLINE uint8_t mdm_bfsmoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x10000000)) >> 28);
}
__INLINE void mdm_bfsmoothforce_setf(uint8_t bfsmoothforce)
{
    ASSERT_ERR((((uint32_t)bfsmoothforce << 28) & ~((uint32_t)0x10000000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x10000000)) | ((uint32_t)bfsmoothforce << 28));
}
__INLINE uint8_t mdm_cfgvhtsts2smoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x02000000)) >> 25);
}
__INLINE void mdm_cfgvhtsts2smoothforce_setf(uint8_t cfgvhtsts2smoothforce)
{
    ASSERT_ERR((((uint32_t)cfgvhtsts2smoothforce << 25) & ~((uint32_t)0x02000000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x02000000)) | ((uint32_t)cfgvhtsts2smoothforce << 25));
}
__INLINE uint8_t mdm_cfgvhtsts1smoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x01000000)) >> 24);
}
__INLINE void mdm_cfgvhtsts1smoothforce_setf(uint8_t cfgvhtsts1smoothforce)
{
    ASSERT_ERR((((uint32_t)cfgvhtsts1smoothforce << 24) & ~((uint32_t)0x01000000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x01000000)) | ((uint32_t)cfgvhtsts1smoothforce << 24));
}
__INLINE uint8_t mdm_cfghtsts2smoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00200000)) >> 21);
}
__INLINE void mdm_cfghtsts2smoothforce_setf(uint8_t cfghtsts2smoothforce)
{
    ASSERT_ERR((((uint32_t)cfghtsts2smoothforce << 21) & ~((uint32_t)0x00200000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00200000)) | ((uint32_t)cfghtsts2smoothforce << 21));
}
__INLINE uint8_t mdm_cfghtsts1smoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00100000)) >> 20);
}
__INLINE void mdm_cfghtsts1smoothforce_setf(uint8_t cfghtsts1smoothforce)
{
    ASSERT_ERR((((uint32_t)cfghtsts1smoothforce << 20) & ~((uint32_t)0x00100000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00100000)) | ((uint32_t)cfghtsts1smoothforce << 20));
}
__INLINE uint8_t mdm_cfgnonhtsmoothforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00010000)) >> 16);
}
__INLINE void mdm_cfgnonhtsmoothforce_setf(uint8_t cfgnonhtsmoothforce)
{
    ASSERT_ERR((((uint32_t)cfgnonhtsmoothforce << 16) & ~((uint32_t)0x00010000)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00010000)) | ((uint32_t)cfgnonhtsmoothforce << 16));
}
__INLINE uint8_t mdm_cfgvhtsts2smooth_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00000C00)) >> 10);
}
__INLINE void mdm_cfgvhtsts2smooth_setf(uint8_t cfgvhtsts2smooth)
{
    ASSERT_ERR((((uint32_t)cfgvhtsts2smooth << 10) & ~((uint32_t)0x00000C00)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00000C00)) | ((uint32_t)cfgvhtsts2smooth << 10));
}
__INLINE uint8_t mdm_cfgvhtsts1smooth_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00000300)) >> 8);
}
__INLINE void mdm_cfgvhtsts1smooth_setf(uint8_t cfgvhtsts1smooth)
{
    ASSERT_ERR((((uint32_t)cfgvhtsts1smooth << 8) & ~((uint32_t)0x00000300)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00000300)) | ((uint32_t)cfgvhtsts1smooth << 8));
}
__INLINE uint8_t mdm_cfghtsts2smooth_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x000000C0)) >> 6);
}
__INLINE void mdm_cfghtsts2smooth_setf(uint8_t cfghtsts2smooth)
{
    ASSERT_ERR((((uint32_t)cfghtsts2smooth << 6) & ~((uint32_t)0x000000C0)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x000000C0)) | ((uint32_t)cfghtsts2smooth << 6));
}
__INLINE uint8_t mdm_cfghtsts1smooth_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00000030)) >> 4);
}
__INLINE void mdm_cfghtsts1smooth_setf(uint8_t cfghtsts1smooth)
{
    ASSERT_ERR((((uint32_t)cfghtsts1smooth << 4) & ~((uint32_t)0x00000030)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00000030)) | ((uint32_t)cfghtsts1smooth << 4));
}
__INLINE uint8_t mdm_cfgnonhtsmooth_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR);
    return ((localVal & ((uint32_t)0x00000003)) >> 0);
}
__INLINE void mdm_cfgnonhtsmooth_setf(uint8_t cfgnonhtsmooth)
{
    ASSERT_ERR((((uint32_t)cfgnonhtsmooth << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(MDM_SMOOTHFORCECTRL_ADDR, (REG_PL_RD(MDM_SMOOTHFORCECTRL_ADDR) & ~((uint32_t)0x00000003)) | ((uint32_t)cfgnonhtsmooth << 0));
}
#define MDM_SCRAMBLERCTRL_ADDR (REG_MDM_CFG_BASE_ADDR+0x0854)
#define MDM_SCRAMBLERCTRL_OFFSET 0x00000054
#define MDM_SCRAMBLERCTRL_INDEX 0x00000015
#define MDM_SCRAMBLERCTRL_RESET 0x00000000
__INLINE uint32_t mdm_scramblerctrl_get(void)
{
    return REG_PL_RD(MDM_SCRAMBLERCTRL_ADDR);
}
__INLINE void mdm_scramblerctrl_set(uint32_t value)
{
    REG_PL_WR(MDM_SCRAMBLERCTRL_ADDR, value);
}
#define MDM_SCRAMSEED_MASK ((uint32_t)0x000000FF)
#define MDM_SCRAMSEED_LSB 0
#define MDM_SCRAMSEED_WIDTH ((uint32_t)0x00000008)
#define MDM_SCRAMSEED_RST 0x0
__INLINE uint8_t mdm_scramseed_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SCRAMBLERCTRL_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x000000FF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_scramseed_setf(uint8_t scramseed)
{
    ASSERT_ERR((((uint32_t)scramseed << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_SCRAMBLERCTRL_ADDR, (uint32_t)scramseed << 0);
}
#define MDM_TBECTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0858)
#define MDM_TBECTRL0_OFFSET 0x00000058
#define MDM_TBECTRL0_INDEX 0x00000016
#define MDM_TBECTRL0_RESET 0x0C130800
__INLINE uint32_t mdm_tbectrl0_get(void)
{
    return REG_PL_RD(MDM_TBECTRL0_ADDR);
}
__INLINE void mdm_tbectrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_TBECTRL0_ADDR, value);
}
#define MDM_CROSS_CORR_SNR_THRESHOLD_MASK ((uint32_t)0xFF000000)
#define MDM_CROSS_CORR_SNR_THRESHOLD_LSB 24
#define MDM_CROSS_CORR_SNR_THRESHOLD_WIDTH ((uint32_t)0x00000008)
#define MDM_TBE_COUNT_ADJUST_80_MASK ((uint32_t)0x00FF0000)
#define MDM_TBE_COUNT_ADJUST_80_LSB 16
#define MDM_TBE_COUNT_ADJUST_80_WIDTH ((uint32_t)0x00000008)
#define MDM_TBE_COUNT_ADJUST_40_MASK ((uint32_t)0x0000FF00)
#define MDM_TBE_COUNT_ADJUST_40_LSB 8
#define MDM_TBE_COUNT_ADJUST_40_WIDTH ((uint32_t)0x00000008)
#define MDM_TBE_COUNT_ADJUST_20_MASK ((uint32_t)0x000000FF)
#define MDM_TBE_COUNT_ADJUST_20_LSB 0
#define MDM_TBE_COUNT_ADJUST_20_WIDTH ((uint32_t)0x00000008)
#define MDM_CROSS_CORR_SNR_THRESHOLD_RST 0xC
#define MDM_TBE_COUNT_ADJUST_80_RST 0x13
#define MDM_TBE_COUNT_ADJUST_40_RST 0x8
#define MDM_TBE_COUNT_ADJUST_20_RST 0x0
__INLINE void mdm_tbectrl0_pack(uint8_t crosscorrsnrthreshold, uint8_t tbecountadjust80, uint8_t tbecountadjust40, uint8_t tbecountadjust20)
{
    ASSERT_ERR((((uint32_t)crosscorrsnrthreshold << 24) & ~((uint32_t)0xFF000000)) == 0);
    ASSERT_ERR((((uint32_t)tbecountadjust80 << 16) & ~((uint32_t)0x00FF0000)) == 0);
    ASSERT_ERR((((uint32_t)tbecountadjust40 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint32_t)tbecountadjust20 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TBECTRL0_ADDR, ((uint32_t)crosscorrsnrthreshold << 24) | ((uint32_t)tbecountadjust80 << 16) | ((uint32_t)tbecountadjust40 << 8) | ((uint32_t)tbecountadjust20 << 0));
}
__INLINE void mdm_tbectrl0_unpack(uint8_t* crosscorrsnrthreshold, uint8_t* tbecountadjust80, uint8_t* tbecountadjust40, uint8_t* tbecountadjust20)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL0_ADDR);
    *crosscorrsnrthreshold = (localVal & ((uint32_t)0xFF000000)) >> 24;
    *tbecountadjust80 = (localVal & ((uint32_t)0x00FF0000)) >> 16;
    *tbecountadjust40 = (localVal & ((uint32_t)0x0000FF00)) >> 8;
    *tbecountadjust20 = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_cross_corr_snr_threshold_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL0_ADDR);
    return ((localVal & ((uint32_t)0xFF000000)) >> 24);
}
__INLINE void mdm_cross_corr_snr_threshold_setf(uint8_t crosscorrsnrthreshold)
{
    ASSERT_ERR((((uint32_t)crosscorrsnrthreshold << 24) & ~((uint32_t)0xFF000000)) == 0);
    REG_PL_WR(MDM_TBECTRL0_ADDR, (REG_PL_RD(MDM_TBECTRL0_ADDR) & ~((uint32_t)0xFF000000)) | ((uint32_t)crosscorrsnrthreshold << 24));
}
__INLINE uint8_t mdm_tbe_count_adjust_80_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00FF0000)) >> 16);
}
__INLINE void mdm_tbe_count_adjust_80_setf(uint8_t tbecountadjust80)
{
    ASSERT_ERR((((uint32_t)tbecountadjust80 << 16) & ~((uint32_t)0x00FF0000)) == 0);
    REG_PL_WR(MDM_TBECTRL0_ADDR, (REG_PL_RD(MDM_TBECTRL0_ADDR) & ~((uint32_t)0x00FF0000)) | ((uint32_t)tbecountadjust80 << 16));
}
__INLINE uint8_t mdm_tbe_count_adjust_40_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL0_ADDR);
    return ((localVal & ((uint32_t)0x0000FF00)) >> 8);
}
__INLINE void mdm_tbe_count_adjust_40_setf(uint8_t tbecountadjust40)
{
    ASSERT_ERR((((uint32_t)tbecountadjust40 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    REG_PL_WR(MDM_TBECTRL0_ADDR, (REG_PL_RD(MDM_TBECTRL0_ADDR) & ~((uint32_t)0x0000FF00)) | ((uint32_t)tbecountadjust40 << 8));
}
__INLINE uint8_t mdm_tbe_count_adjust_20_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL0_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_tbe_count_adjust_20_setf(uint8_t tbecountadjust20)
{
    ASSERT_ERR((((uint32_t)tbecountadjust20 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TBECTRL0_ADDR, (REG_PL_RD(MDM_TBECTRL0_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)tbecountadjust20 << 0));
}
#define MDM_TBECTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x085C)
#define MDM_TBECTRL1_OFFSET 0x0000005C
#define MDM_TBECTRL1_INDEX 0x00000017
#define MDM_TBECTRL1_RESET 0x3C143E14
__INLINE uint32_t mdm_tbectrl1_get(void)
{
    return REG_PL_RD(MDM_TBECTRL1_ADDR);
}
__INLINE void mdm_tbectrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_TBECTRL1_ADDR, value);
}
#define MDM_PEAK_SEARCH_STOP_HIGH_SNR_MASK ((uint32_t)0x7F000000)
#define MDM_PEAK_SEARCH_STOP_HIGH_SNR_LSB 24
#define MDM_PEAK_SEARCH_STOP_HIGH_SNR_WIDTH ((uint32_t)0x00000007)
#define MDM_PEAK_SEARCH_START_HIGH_SNR_MASK ((uint32_t)0x007F0000)
#define MDM_PEAK_SEARCH_START_HIGH_SNR_LSB 16
#define MDM_PEAK_SEARCH_START_HIGH_SNR_WIDTH ((uint32_t)0x00000007)
#define MDM_PEAK_SEARCH_STOP_LOW_SNR_MASK ((uint32_t)0x00007F00)
#define MDM_PEAK_SEARCH_STOP_LOW_SNR_LSB 8
#define MDM_PEAK_SEARCH_STOP_LOW_SNR_WIDTH ((uint32_t)0x00000007)
#define MDM_PEAK_SEARCH_START_LOW_SNR_MASK ((uint32_t)0x0000007F)
#define MDM_PEAK_SEARCH_START_LOW_SNR_LSB 0
#define MDM_PEAK_SEARCH_START_LOW_SNR_WIDTH ((uint32_t)0x00000007)
#define MDM_PEAK_SEARCH_STOP_HIGH_SNR_RST 0x3C
#define MDM_PEAK_SEARCH_START_HIGH_SNR_RST 0x14
#define MDM_PEAK_SEARCH_STOP_LOW_SNR_RST 0x3E
#define MDM_PEAK_SEARCH_START_LOW_SNR_RST 0x14
__INLINE void mdm_tbectrl1_pack(uint8_t peaksearchstophighsnr, uint8_t peaksearchstarthighsnr, uint8_t peaksearchstoplowsnr, uint8_t peaksearchstartlowsnr)
{
    ASSERT_ERR((((uint32_t)peaksearchstophighsnr << 24) & ~((uint32_t)0x7F000000)) == 0);
    ASSERT_ERR((((uint32_t)peaksearchstarthighsnr << 16) & ~((uint32_t)0x007F0000)) == 0);
    ASSERT_ERR((((uint32_t)peaksearchstoplowsnr << 8) & ~((uint32_t)0x00007F00)) == 0);
    ASSERT_ERR((((uint32_t)peaksearchstartlowsnr << 0) & ~((uint32_t)0x0000007F)) == 0);
    REG_PL_WR(MDM_TBECTRL1_ADDR, ((uint32_t)peaksearchstophighsnr << 24) | ((uint32_t)peaksearchstarthighsnr << 16) | ((uint32_t)peaksearchstoplowsnr << 8) | ((uint32_t)peaksearchstartlowsnr << 0));
}
__INLINE void mdm_tbectrl1_unpack(uint8_t* peaksearchstophighsnr, uint8_t* peaksearchstarthighsnr, uint8_t* peaksearchstoplowsnr, uint8_t* peaksearchstartlowsnr)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL1_ADDR);
    *peaksearchstophighsnr = (localVal & ((uint32_t)0x7F000000)) >> 24;
    *peaksearchstarthighsnr = (localVal & ((uint32_t)0x007F0000)) >> 16;
    *peaksearchstoplowsnr = (localVal & ((uint32_t)0x00007F00)) >> 8;
    *peaksearchstartlowsnr = (localVal & ((uint32_t)0x0000007F)) >> 0;
}
__INLINE uint8_t mdm_peak_search_stop_high_snr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL1_ADDR);
    return ((localVal & ((uint32_t)0x7F000000)) >> 24);
}
__INLINE void mdm_peak_search_stop_high_snr_setf(uint8_t peaksearchstophighsnr)
{
    ASSERT_ERR((((uint32_t)peaksearchstophighsnr << 24) & ~((uint32_t)0x7F000000)) == 0);
    REG_PL_WR(MDM_TBECTRL1_ADDR, (REG_PL_RD(MDM_TBECTRL1_ADDR) & ~((uint32_t)0x7F000000)) | ((uint32_t)peaksearchstophighsnr << 24));
}
__INLINE uint8_t mdm_peak_search_start_high_snr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL1_ADDR);
    return ((localVal & ((uint32_t)0x007F0000)) >> 16);
}
__INLINE void mdm_peak_search_start_high_snr_setf(uint8_t peaksearchstarthighsnr)
{
    ASSERT_ERR((((uint32_t)peaksearchstarthighsnr << 16) & ~((uint32_t)0x007F0000)) == 0);
    REG_PL_WR(MDM_TBECTRL1_ADDR, (REG_PL_RD(MDM_TBECTRL1_ADDR) & ~((uint32_t)0x007F0000)) | ((uint32_t)peaksearchstarthighsnr << 16));
}
__INLINE uint8_t mdm_peak_search_stop_low_snr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00007F00)) >> 8);
}
__INLINE void mdm_peak_search_stop_low_snr_setf(uint8_t peaksearchstoplowsnr)
{
    ASSERT_ERR((((uint32_t)peaksearchstoplowsnr << 8) & ~((uint32_t)0x00007F00)) == 0);
    REG_PL_WR(MDM_TBECTRL1_ADDR, (REG_PL_RD(MDM_TBECTRL1_ADDR) & ~((uint32_t)0x00007F00)) | ((uint32_t)peaksearchstoplowsnr << 8));
}
__INLINE uint8_t mdm_peak_search_start_low_snr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL1_ADDR);
    return ((localVal & ((uint32_t)0x0000007F)) >> 0);
}
__INLINE void mdm_peak_search_start_low_snr_setf(uint8_t peaksearchstartlowsnr)
{
    ASSERT_ERR((((uint32_t)peaksearchstartlowsnr << 0) & ~((uint32_t)0x0000007F)) == 0);
    REG_PL_WR(MDM_TBECTRL1_ADDR, (REG_PL_RD(MDM_TBECTRL1_ADDR) & ~((uint32_t)0x0000007F)) | ((uint32_t)peaksearchstartlowsnr << 0));
}
#define MDM_TBECTRL2_ADDR (REG_MDM_CFG_BASE_ADDR+0x0860)
#define MDM_TBECTRL2_OFFSET 0x00000060
#define MDM_TBECTRL2_INDEX 0x00000018
#define MDM_TBECTRL2_RESET 0x00007F05
__INLINE uint32_t mdm_tbectrl2_get(void)
{
    return REG_PL_RD(MDM_TBECTRL2_ADDR);
}
__INLINE void mdm_tbectrl2_set(uint32_t value)
{
    REG_PL_WR(MDM_TBECTRL2_ADDR, value);
}
#define MDM_TD_ADJUST_80_SHORT_GI_MASK ((uint32_t)0xF0000000)
#define MDM_TD_ADJUST_80_SHORT_GI_LSB 28
#define MDM_TD_ADJUST_80_SHORT_GI_WIDTH ((uint32_t)0x00000004)
#define MDM_TD_ADJUST_40_SHORT_GI_MASK ((uint32_t)0x0F000000)
#define MDM_TD_ADJUST_40_SHORT_GI_LSB 24
#define MDM_TD_ADJUST_40_SHORT_GI_WIDTH ((uint32_t)0x00000004)
#define MDM_TD_ADJUST_20_SHORT_GI_MASK ((uint32_t)0x00F00000)
#define MDM_TD_ADJUST_20_SHORT_GI_LSB 20
#define MDM_TD_ADJUST_20_SHORT_GI_WIDTH ((uint32_t)0x00000004)
#define MDM_PEAK_SEARCH_DELTA_MASK ((uint32_t)0x0001F000)
#define MDM_PEAK_SEARCH_DELTA_LSB 12
#define MDM_PEAK_SEARCH_DELTA_WIDTH ((uint32_t)0x00000005)
#define MDM_PEAK_SEARCH_NDL_EN_BIT ((uint32_t)0x00000800)
#define MDM_PEAK_SEARCH_NDL_EN_POS 11
#define MDM_PEAK_SEARCH_NDL_INDEX_MASK ((uint32_t)0x00000700)
#define MDM_PEAK_SEARCH_NDL_INDEX_LSB 8
#define MDM_PEAK_SEARCH_NDL_INDEX_WIDTH ((uint32_t)0x00000003)
#define MDM_TBE_BIAS_MASK ((uint32_t)0x000000FF)
#define MDM_TBE_BIAS_LSB 0
#define MDM_TBE_BIAS_WIDTH ((uint32_t)0x00000008)
#define MDM_TD_ADJUST_80_SHORT_GI_RST 0x0
#define MDM_TD_ADJUST_40_SHORT_GI_RST 0x0
#define MDM_TD_ADJUST_20_SHORT_GI_RST 0x0
#define MDM_PEAK_SEARCH_DELTA_RST 0x7
#define MDM_PEAK_SEARCH_NDL_EN_RST 0x1
#define MDM_PEAK_SEARCH_NDL_INDEX_RST 0x7
#define MDM_TBE_BIAS_RST 0x5
__INLINE void mdm_tbectrl2_pack(uint8_t tdadjust80shortgi, uint8_t tdadjust40shortgi, uint8_t tdadjust20shortgi, uint8_t peaksearchdelta, uint8_t peaksearchndlen, uint8_t peaksearchndlindex, uint8_t tbebias)
{
    ASSERT_ERR((((uint32_t)tdadjust80shortgi << 28) & ~((uint32_t)0xF0000000)) == 0);
    ASSERT_ERR((((uint32_t)tdadjust40shortgi << 24) & ~((uint32_t)0x0F000000)) == 0);
    ASSERT_ERR((((uint32_t)tdadjust20shortgi << 20) & ~((uint32_t)0x00F00000)) == 0);
    ASSERT_ERR((((uint32_t)peaksearchdelta << 12) & ~((uint32_t)0x0001F000)) == 0);
    ASSERT_ERR((((uint32_t)peaksearchndlen << 11) & ~((uint32_t)0x00000800)) == 0);
    ASSERT_ERR((((uint32_t)peaksearchndlindex << 8) & ~((uint32_t)0x00000700)) == 0);
    ASSERT_ERR((((uint32_t)tbebias << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, ((uint32_t)tdadjust80shortgi << 28) | ((uint32_t)tdadjust40shortgi << 24) | ((uint32_t)tdadjust20shortgi << 20) | ((uint32_t)peaksearchdelta << 12) | ((uint32_t)peaksearchndlen << 11) | ((uint32_t)peaksearchndlindex << 8) | ((uint32_t)tbebias << 0));
}
__INLINE void mdm_tbectrl2_unpack(uint8_t* tdadjust80shortgi, uint8_t* tdadjust40shortgi, uint8_t* tdadjust20shortgi, uint8_t* peaksearchdelta, uint8_t* peaksearchndlen, uint8_t* peaksearchndlindex, uint8_t* tbebias)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    *tdadjust80shortgi = (localVal & ((uint32_t)0xF0000000)) >> 28;
    *tdadjust40shortgi = (localVal & ((uint32_t)0x0F000000)) >> 24;
    *tdadjust20shortgi = (localVal & ((uint32_t)0x00F00000)) >> 20;
    *peaksearchdelta = (localVal & ((uint32_t)0x0001F000)) >> 12;
    *peaksearchndlen = (localVal & ((uint32_t)0x00000800)) >> 11;
    *peaksearchndlindex = (localVal & ((uint32_t)0x00000700)) >> 8;
    *tbebias = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_td_adjust_80_short_gi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0xF0000000)) >> 28);
}
__INLINE void mdm_td_adjust_80_short_gi_setf(uint8_t tdadjust80shortgi)
{
    ASSERT_ERR((((uint32_t)tdadjust80shortgi << 28) & ~((uint32_t)0xF0000000)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0xF0000000)) | ((uint32_t)tdadjust80shortgi << 28));
}
__INLINE uint8_t mdm_td_adjust_40_short_gi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0x0F000000)) >> 24);
}
__INLINE void mdm_td_adjust_40_short_gi_setf(uint8_t tdadjust40shortgi)
{
    ASSERT_ERR((((uint32_t)tdadjust40shortgi << 24) & ~((uint32_t)0x0F000000)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0x0F000000)) | ((uint32_t)tdadjust40shortgi << 24));
}
__INLINE uint8_t mdm_td_adjust_20_short_gi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0x00F00000)) >> 20);
}
__INLINE void mdm_td_adjust_20_short_gi_setf(uint8_t tdadjust20shortgi)
{
    ASSERT_ERR((((uint32_t)tdadjust20shortgi << 20) & ~((uint32_t)0x00F00000)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0x00F00000)) | ((uint32_t)tdadjust20shortgi << 20));
}
__INLINE uint8_t mdm_peak_search_delta_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0x0001F000)) >> 12);
}
__INLINE void mdm_peak_search_delta_setf(uint8_t peaksearchdelta)
{
    ASSERT_ERR((((uint32_t)peaksearchdelta << 12) & ~((uint32_t)0x0001F000)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0x0001F000)) | ((uint32_t)peaksearchdelta << 12));
}
__INLINE uint8_t mdm_peak_search_ndl_en_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0x00000800)) >> 11);
}
__INLINE void mdm_peak_search_ndl_en_setf(uint8_t peaksearchndlen)
{
    ASSERT_ERR((((uint32_t)peaksearchndlen << 11) & ~((uint32_t)0x00000800)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0x00000800)) | ((uint32_t)peaksearchndlen << 11));
}
__INLINE uint8_t mdm_peak_search_ndl_index_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0x00000700)) >> 8);
}
__INLINE void mdm_peak_search_ndl_index_setf(uint8_t peaksearchndlindex)
{
    ASSERT_ERR((((uint32_t)peaksearchndlindex << 8) & ~((uint32_t)0x00000700)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0x00000700)) | ((uint32_t)peaksearchndlindex << 8));
}
__INLINE uint8_t mdm_tbe_bias_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TBECTRL2_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_tbe_bias_setf(uint8_t tbebias)
{
    ASSERT_ERR((((uint32_t)tbebias << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TBECTRL2_ADDR, (REG_PL_RD(MDM_TBECTRL2_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)tbebias << 0));
}
#define MDM_TDFOCTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0864)
#define MDM_TDFOCTRL0_OFFSET 0x00000064
#define MDM_TDFOCTRL0_INDEX 0x00000019
#define MDM_TDFOCTRL0_RESET 0x0020019A
__INLINE uint32_t mdm_tdfoctrl0_get(void)
{
    return REG_PL_RD(MDM_TDFOCTRL0_ADDR);
}
__INLINE void mdm_tdfoctrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_TDFOCTRL0_ADDR, value);
}
#define MDM_AUTO_CORR_COMPARE_RATIO_HIGH_SNR_MASK ((uint32_t)0x003FF000)
#define MDM_AUTO_CORR_COMPARE_RATIO_HIGH_SNR_LSB 12
#define MDM_AUTO_CORR_COMPARE_RATIO_HIGH_SNR_WIDTH ((uint32_t)0x0000000A)
#define MDM_AUTO_CORR_COMPARE_RATIO_LOW_SNR_MASK ((uint32_t)0x000003FF)
#define MDM_AUTO_CORR_COMPARE_RATIO_LOW_SNR_LSB 0
#define MDM_AUTO_CORR_COMPARE_RATIO_LOW_SNR_WIDTH ((uint32_t)0x0000000A)
#define MDM_AUTO_CORR_COMPARE_RATIO_HIGH_SNR_RST 0x200
#define MDM_AUTO_CORR_COMPARE_RATIO_LOW_SNR_RST 0x19A
__INLINE void mdm_tdfoctrl0_pack(uint16_t autocorrcompareratiohighsnr, uint16_t autocorrcompareratiolowsnr)
{
    ASSERT_ERR((((uint32_t)autocorrcompareratiohighsnr << 12) & ~((uint32_t)0x003FF000)) == 0);
    ASSERT_ERR((((uint32_t)autocorrcompareratiolowsnr << 0) & ~((uint32_t)0x000003FF)) == 0);
    REG_PL_WR(MDM_TDFOCTRL0_ADDR, ((uint32_t)autocorrcompareratiohighsnr << 12) | ((uint32_t)autocorrcompareratiolowsnr << 0));
}
__INLINE void mdm_tdfoctrl0_unpack(uint16_t* autocorrcompareratiohighsnr, uint16_t* autocorrcompareratiolowsnr)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL0_ADDR);
    *autocorrcompareratiohighsnr = (localVal & ((uint32_t)0x003FF000)) >> 12;
    *autocorrcompareratiolowsnr = (localVal & ((uint32_t)0x000003FF)) >> 0;
}
__INLINE uint16_t mdm_auto_corr_compare_ratio_high_snr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x003FF000)) >> 12);
}
__INLINE void mdm_auto_corr_compare_ratio_high_snr_setf(uint16_t autocorrcompareratiohighsnr)
{
    ASSERT_ERR((((uint32_t)autocorrcompareratiohighsnr << 12) & ~((uint32_t)0x003FF000)) == 0);
    REG_PL_WR(MDM_TDFOCTRL0_ADDR, (REG_PL_RD(MDM_TDFOCTRL0_ADDR) & ~((uint32_t)0x003FF000)) | ((uint32_t)autocorrcompareratiohighsnr << 12));
}
__INLINE uint16_t mdm_auto_corr_compare_ratio_low_snr_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x000003FF)) >> 0);
}
__INLINE void mdm_auto_corr_compare_ratio_low_snr_setf(uint16_t autocorrcompareratiolowsnr)
{
    ASSERT_ERR((((uint32_t)autocorrcompareratiolowsnr << 0) & ~((uint32_t)0x000003FF)) == 0);
    REG_PL_WR(MDM_TDFOCTRL0_ADDR, (REG_PL_RD(MDM_TDFOCTRL0_ADDR) & ~((uint32_t)0x000003FF)) | ((uint32_t)autocorrcompareratiolowsnr << 0));
}
#define MDM_TDFOCTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x0868)
#define MDM_TDFOCTRL1_OFFSET 0x00000068
#define MDM_TDFOCTRL1_INDEX 0x0000001A
#define MDM_TDFOCTRL1_RESET 0x00042032
__INLINE uint32_t mdm_tdfoctrl1_get(void)
{
    return REG_PL_RD(MDM_TDFOCTRL1_ADDR);
}
__INLINE void mdm_tdfoctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_TDFOCTRL1_ADDR, value);
}
#define MDM_AUTO_CORR_PLAT_SUM_END_MASK ((uint32_t)0x001FF000)
#define MDM_AUTO_CORR_PLAT_SUM_END_LSB 12
#define MDM_AUTO_CORR_PLAT_SUM_END_WIDTH ((uint32_t)0x00000009)
#define MDM_AUTO_CORR_PLAT_SUM_START_MASK ((uint32_t)0x000001FF)
#define MDM_AUTO_CORR_PLAT_SUM_START_LSB 0
#define MDM_AUTO_CORR_PLAT_SUM_START_WIDTH ((uint32_t)0x00000009)
#define MDM_AUTO_CORR_PLAT_SUM_END_RST 0x42
#define MDM_AUTO_CORR_PLAT_SUM_START_RST 0x32
__INLINE void mdm_tdfoctrl1_pack(uint16_t autocorrplatsumend, uint16_t autocorrplatsumstart)
{
    ASSERT_ERR((((uint32_t)autocorrplatsumend << 12) & ~((uint32_t)0x001FF000)) == 0);
    ASSERT_ERR((((uint32_t)autocorrplatsumstart << 0) & ~((uint32_t)0x000001FF)) == 0);
    REG_PL_WR(MDM_TDFOCTRL1_ADDR, ((uint32_t)autocorrplatsumend << 12) | ((uint32_t)autocorrplatsumstart << 0));
}
__INLINE void mdm_tdfoctrl1_unpack(uint16_t* autocorrplatsumend, uint16_t* autocorrplatsumstart)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL1_ADDR);
    *autocorrplatsumend = (localVal & ((uint32_t)0x001FF000)) >> 12;
    *autocorrplatsumstart = (localVal & ((uint32_t)0x000001FF)) >> 0;
}
__INLINE uint16_t mdm_auto_corr_plat_sum_end_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x001FF000)) >> 12);
}
__INLINE void mdm_auto_corr_plat_sum_end_setf(uint16_t autocorrplatsumend)
{
    ASSERT_ERR((((uint32_t)autocorrplatsumend << 12) & ~((uint32_t)0x001FF000)) == 0);
    REG_PL_WR(MDM_TDFOCTRL1_ADDR, (REG_PL_RD(MDM_TDFOCTRL1_ADDR) & ~((uint32_t)0x001FF000)) | ((uint32_t)autocorrplatsumend << 12));
}
__INLINE uint16_t mdm_auto_corr_plat_sum_start_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x000001FF)) >> 0);
}
__INLINE void mdm_auto_corr_plat_sum_start_setf(uint16_t autocorrplatsumstart)
{
    ASSERT_ERR((((uint32_t)autocorrplatsumstart << 0) & ~((uint32_t)0x000001FF)) == 0);
    REG_PL_WR(MDM_TDFOCTRL1_ADDR, (REG_PL_RD(MDM_TDFOCTRL1_ADDR) & ~((uint32_t)0x000001FF)) | ((uint32_t)autocorrplatsumstart << 0));
}
#define MDM_TDFOCTRL2_ADDR (REG_MDM_CFG_BASE_ADDR+0x086C)
#define MDM_TDFOCTRL2_OFFSET 0x0000006C
#define MDM_TDFOCTRL2_INDEX 0x0000001B
#define MDM_TDFOCTRL2_RESET 0x04C07044
__INLINE uint32_t mdm_tdfoctrl2_get(void)
{
    return REG_PL_RD(MDM_TDFOCTRL2_ADDR);
}
__INLINE void mdm_tdfoctrl2_set(uint32_t value)
{
    REG_PL_WR(MDM_TDFOCTRL2_ADDR, value);
}
#define MDM_AUTO_CORR_PH_SUM_END_MASK ((uint32_t)0x1FF00000)
#define MDM_AUTO_CORR_PH_SUM_END_LSB 20
#define MDM_AUTO_CORR_PH_SUM_END_WIDTH ((uint32_t)0x00000009)
#define MDM_AUTO_CORR_PH_SUM_START_MASK ((uint32_t)0x0007FC00)
#define MDM_AUTO_CORR_PH_SUM_START_LSB 10
#define MDM_AUTO_CORR_PH_SUM_START_WIDTH ((uint32_t)0x00000009)
#define MDM_AUTO_CORR_PLAT_FALL_SEARCH_START_MASK ((uint32_t)0x000001FF)
#define MDM_AUTO_CORR_PLAT_FALL_SEARCH_START_LSB 0
#define MDM_AUTO_CORR_PLAT_FALL_SEARCH_START_WIDTH ((uint32_t)0x00000009)
#define MDM_AUTO_CORR_PH_SUM_END_RST 0x4C
#define MDM_AUTO_CORR_PH_SUM_START_RST 0x1C
#define MDM_AUTO_CORR_PLAT_FALL_SEARCH_START_RST 0x44
__INLINE void mdm_tdfoctrl2_pack(uint16_t autocorrphsumend, uint16_t autocorrphsumstart, uint16_t autocorrplatfallsearchstart)
{
    ASSERT_ERR((((uint32_t)autocorrphsumend << 20) & ~((uint32_t)0x1FF00000)) == 0);
    ASSERT_ERR((((uint32_t)autocorrphsumstart << 10) & ~((uint32_t)0x0007FC00)) == 0);
    ASSERT_ERR((((uint32_t)autocorrplatfallsearchstart << 0) & ~((uint32_t)0x000001FF)) == 0);
    REG_PL_WR(MDM_TDFOCTRL2_ADDR, ((uint32_t)autocorrphsumend << 20) | ((uint32_t)autocorrphsumstart << 10) | ((uint32_t)autocorrplatfallsearchstart << 0));
}
__INLINE void mdm_tdfoctrl2_unpack(uint16_t* autocorrphsumend, uint16_t* autocorrphsumstart, uint16_t* autocorrplatfallsearchstart)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL2_ADDR);
    *autocorrphsumend = (localVal & ((uint32_t)0x1FF00000)) >> 20;
    *autocorrphsumstart = (localVal & ((uint32_t)0x0007FC00)) >> 10;
    *autocorrplatfallsearchstart = (localVal & ((uint32_t)0x000001FF)) >> 0;
}
__INLINE uint16_t mdm_auto_corr_ph_sum_end_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x1FF00000)) >> 20);
}
__INLINE void mdm_auto_corr_ph_sum_end_setf(uint16_t autocorrphsumend)
{
    ASSERT_ERR((((uint32_t)autocorrphsumend << 20) & ~((uint32_t)0x1FF00000)) == 0);
    REG_PL_WR(MDM_TDFOCTRL2_ADDR, (REG_PL_RD(MDM_TDFOCTRL2_ADDR) & ~((uint32_t)0x1FF00000)) | ((uint32_t)autocorrphsumend << 20));
}
__INLINE uint16_t mdm_auto_corr_ph_sum_start_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x0007FC00)) >> 10);
}
__INLINE void mdm_auto_corr_ph_sum_start_setf(uint16_t autocorrphsumstart)
{
    ASSERT_ERR((((uint32_t)autocorrphsumstart << 10) & ~((uint32_t)0x0007FC00)) == 0);
    REG_PL_WR(MDM_TDFOCTRL2_ADDR, (REG_PL_RD(MDM_TDFOCTRL2_ADDR) & ~((uint32_t)0x0007FC00)) | ((uint32_t)autocorrphsumstart << 10));
}
__INLINE uint16_t mdm_auto_corr_plat_fall_search_start_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x000001FF)) >> 0);
}
__INLINE void mdm_auto_corr_plat_fall_search_start_setf(uint16_t autocorrplatfallsearchstart)
{
    ASSERT_ERR((((uint32_t)autocorrplatfallsearchstart << 0) & ~((uint32_t)0x000001FF)) == 0);
    REG_PL_WR(MDM_TDFOCTRL2_ADDR, (REG_PL_RD(MDM_TDFOCTRL2_ADDR) & ~((uint32_t)0x000001FF)) | ((uint32_t)autocorrplatfallsearchstart << 0));
}
#define MDM_TDFOCTRL3_ADDR (REG_MDM_CFG_BASE_ADDR+0x0870)
#define MDM_TDFOCTRL3_OFFSET 0x00000070
#define MDM_TDFOCTRL3_INDEX 0x0000001C
#define MDM_TDFOCTRL3_RESET 0x881F40C1
__INLINE uint32_t mdm_tdfoctrl3_get(void)
{
    return REG_PL_RD(MDM_TDFOCTRL3_ADDR);
}
__INLINE void mdm_tdfoctrl3_set(uint32_t value)
{
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, value);
}
#define MDM_TDFOCOMPEN_BIT ((uint32_t)0x80000000)
#define MDM_TDFOCOMPEN_POS 31
#define MDM_TDFOFINEESTEN_BIT ((uint32_t)0x08000000)
#define MDM_TDFOFINEESTEN_POS 27
#define MDM_TDFO_PLAT_FALL_TIME_OUT_MASK ((uint32_t)0x00FFF000)
#define MDM_TDFO_PLAT_FALL_TIME_OUT_LSB 12
#define MDM_TDFO_PLAT_FALL_TIME_OUT_WIDTH ((uint32_t)0x0000000C)
#define MDM_TDFOSNR_THRESHOLD_MASK ((uint32_t)0x00000FF0)
#define MDM_TDFOSNR_THRESHOLD_LSB 4
#define MDM_TDFOSNR_THRESHOLD_WIDTH ((uint32_t)0x00000008)
#define MDM_AUTO_CORR_FALL_COUNT_MASK ((uint32_t)0x0000000F)
#define MDM_AUTO_CORR_FALL_COUNT_LSB 0
#define MDM_AUTO_CORR_FALL_COUNT_WIDTH ((uint32_t)0x00000004)
#define MDM_TDFOCOMPEN_RST 0x1
#define MDM_TDFOFINEESTEN_RST 0x1
#define MDM_TDFO_PLAT_FALL_TIME_OUT_RST 0x1F4
#define MDM_TDFOSNR_THRESHOLD_RST 0xC
#define MDM_AUTO_CORR_FALL_COUNT_RST 0x1
__INLINE void mdm_tdfoctrl3_pack(uint8_t tdfocompen, uint8_t tdfofineesten, uint16_t tdfoplatfalltimeout, uint8_t tdfosnrthreshold, uint8_t autocorrfallcount)
{
    ASSERT_ERR((((uint32_t)tdfocompen << 31) & ~((uint32_t)0x80000000)) == 0);
    ASSERT_ERR((((uint32_t)tdfofineesten << 27) & ~((uint32_t)0x08000000)) == 0);
    ASSERT_ERR((((uint32_t)tdfoplatfalltimeout << 12) & ~((uint32_t)0x00FFF000)) == 0);
    ASSERT_ERR((((uint32_t)tdfosnrthreshold << 4) & ~((uint32_t)0x00000FF0)) == 0);
    ASSERT_ERR((((uint32_t)autocorrfallcount << 0) & ~((uint32_t)0x0000000F)) == 0);
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, ((uint32_t)tdfocompen << 31) | ((uint32_t)tdfofineesten << 27) | ((uint32_t)tdfoplatfalltimeout << 12) | ((uint32_t)tdfosnrthreshold << 4) | ((uint32_t)autocorrfallcount << 0));
}
__INLINE void mdm_tdfoctrl3_unpack(uint8_t* tdfocompen, uint8_t* tdfofineesten, uint16_t* tdfoplatfalltimeout, uint8_t* tdfosnrthreshold, uint8_t* autocorrfallcount)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL3_ADDR);
    *tdfocompen = (localVal & ((uint32_t)0x80000000)) >> 31;
    *tdfofineesten = (localVal & ((uint32_t)0x08000000)) >> 27;
    *tdfoplatfalltimeout = (localVal & ((uint32_t)0x00FFF000)) >> 12;
    *tdfosnrthreshold = (localVal & ((uint32_t)0x00000FF0)) >> 4;
    *autocorrfallcount = (localVal & ((uint32_t)0x0000000F)) >> 0;
}
__INLINE uint8_t mdm_tdfocompen_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x80000000)) >> 31);
}
__INLINE void mdm_tdfocompen_setf(uint8_t tdfocompen)
{
    ASSERT_ERR((((uint32_t)tdfocompen << 31) & ~((uint32_t)0x80000000)) == 0);
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, (REG_PL_RD(MDM_TDFOCTRL3_ADDR) & ~((uint32_t)0x80000000)) | ((uint32_t)tdfocompen << 31));
}
__INLINE uint8_t mdm_tdfofineesten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x08000000)) >> 27);
}
__INLINE void mdm_tdfofineesten_setf(uint8_t tdfofineesten)
{
    ASSERT_ERR((((uint32_t)tdfofineesten << 27) & ~((uint32_t)0x08000000)) == 0);
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, (REG_PL_RD(MDM_TDFOCTRL3_ADDR) & ~((uint32_t)0x08000000)) | ((uint32_t)tdfofineesten << 27));
}
__INLINE uint16_t mdm_tdfo_plat_fall_time_out_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x00FFF000)) >> 12);
}
__INLINE void mdm_tdfo_plat_fall_time_out_setf(uint16_t tdfoplatfalltimeout)
{
    ASSERT_ERR((((uint32_t)tdfoplatfalltimeout << 12) & ~((uint32_t)0x00FFF000)) == 0);
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, (REG_PL_RD(MDM_TDFOCTRL3_ADDR) & ~((uint32_t)0x00FFF000)) | ((uint32_t)tdfoplatfalltimeout << 12));
}
__INLINE uint8_t mdm_tdfosnr_threshold_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x00000FF0)) >> 4);
}
__INLINE void mdm_tdfosnr_threshold_setf(uint8_t tdfosnrthreshold)
{
    ASSERT_ERR((((uint32_t)tdfosnrthreshold << 4) & ~((uint32_t)0x00000FF0)) == 0);
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, (REG_PL_RD(MDM_TDFOCTRL3_ADDR) & ~((uint32_t)0x00000FF0)) | ((uint32_t)tdfosnrthreshold << 4));
}
__INLINE uint8_t mdm_auto_corr_fall_count_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TDFOCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x0000000F)) >> 0);
}
__INLINE void mdm_auto_corr_fall_count_setf(uint8_t autocorrfallcount)
{
    ASSERT_ERR((((uint32_t)autocorrfallcount << 0) & ~((uint32_t)0x0000000F)) == 0);
    REG_PL_WR(MDM_TDFOCTRL3_ADDR, (REG_PL_RD(MDM_TDFOCTRL3_ADDR) & ~((uint32_t)0x0000000F)) | ((uint32_t)autocorrfallcount << 0));
}
#define MDM_CLKGATEFCTRL0_ADDR (REG_MDM_CFG_BASE_ADDR+0x0874)
#define MDM_CLKGATEFCTRL0_OFFSET 0x00000074
#define MDM_CLKGATEFCTRL0_INDEX 0x0000001D
#define MDM_CLKGATEFCTRL0_RESET 0x00000000
__INLINE uint32_t mdm_clkgatefctrl0_get(void)
{
    return REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
}
__INLINE void mdm_clkgatefctrl0_set(uint32_t value)
{
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, value);
}
#define MDM_PHYTXCLKFORCE_BIT ((uint32_t)0x80000000)
#define MDM_PHYTXCLKFORCE_POS 31
#define MDM_BDTXCLKFORCE_BIT ((uint32_t)0x40000000)
#define MDM_BDTXCLKFORCE_POS 30
#define MDM_AGCMEMCLKFORCE_BIT ((uint32_t)0x20000000)
#define MDM_AGCMEMCLKFORCE_POS 29
#define MDM_AGCCLKFORCE_BIT ((uint32_t)0x10000000)
#define MDM_AGCCLKFORCE_POS 28
#define MDM_RCCLKFORCE_BIT ((uint32_t)0x08000000)
#define MDM_RCCLKFORCE_POS 27
#define MDM_FECLKFORCE_BIT ((uint32_t)0x01000000)
#define MDM_FECLKFORCE_POS 24
#define MDM_FDOCLKFORCE_BIT ((uint32_t)0x00800000)
#define MDM_FDOCLKFORCE_POS 23
#define MDM_EQUCLKFORCE_BIT ((uint32_t)0x00400000)
#define MDM_EQUCLKFORCE_POS 22
#define MDM_PHYSVDCLKFORCE_BIT ((uint32_t)0x00200000)
#define MDM_PHYSVDCLKFORCE_POS 21
#define MDM_PHYTXBFCLKFORCE_BIT ((uint32_t)0x00100000)
#define MDM_PHYTXBFCLKFORCE_POS 20
#define MDM_TDCOMPCLKFORCE_BIT ((uint32_t)0x00040000)
#define MDM_TDCOMPCLKFORCE_POS 18
#define MDM_TDFOESTCLKFORCE_BIT ((uint32_t)0x00020000)
#define MDM_TDFOESTCLKFORCE_POS 17
#define MDM_TBECLKFORCE_BIT ((uint32_t)0x00010000)
#define MDM_TBECLKFORCE_POS 16
#define MDM_FFT1MEMCLKFORCE_BIT ((uint32_t)0x00000400)
#define MDM_FFT1MEMCLKFORCE_POS 10
#define MDM_FFT1CLKFORCE_BIT ((uint32_t)0x00000200)
#define MDM_FFT1CLKFORCE_POS 9
#define MDM_FFT0MEMCLKFORCE_BIT ((uint32_t)0x00000100)
#define MDM_FFT0MEMCLKFORCE_POS 8
#define MDM_FFT0CLKFORCE_BIT ((uint32_t)0x00000080)
#define MDM_FFT0CLKFORCE_POS 7
#define MDM_CHESTCLKFORCE_BIT ((uint32_t)0x00000040)
#define MDM_CHESTCLKFORCE_POS 6
#define MDM_HMEMCLKFORCE_BIT ((uint32_t)0x00000010)
#define MDM_HMEMCLKFORCE_POS 4
#define MDM_INTLVMEMCLKFORCE_BIT ((uint32_t)0x00000008)
#define MDM_INTLVMEMCLKFORCE_POS 3
#define MDM_VTB1CLKFORCE_BIT ((uint32_t)0x00000004)
#define MDM_VTB1CLKFORCE_POS 2
#define MDM_VTB0CLKFORCE_BIT ((uint32_t)0x00000002)
#define MDM_VTB0CLKFORCE_POS 1
#define MDM_BDRXCLKFORCE_BIT ((uint32_t)0x00000001)
#define MDM_BDRXCLKFORCE_POS 0
#define MDM_PHYTXCLKFORCE_RST 0x0
#define MDM_BDTXCLKFORCE_RST 0x0
#define MDM_AGCMEMCLKFORCE_RST 0x0
#define MDM_AGCCLKFORCE_RST 0x0
#define MDM_RCCLKFORCE_RST 0x0
#define MDM_FECLKFORCE_RST 0x0
#define MDM_FDOCLKFORCE_RST 0x0
#define MDM_EQUCLKFORCE_RST 0x0
#define MDM_PHYSVDCLKFORCE_RST 0x0
#define MDM_PHYTXBFCLKFORCE_RST 0x0
#define MDM_TDCOMPCLKFORCE_RST 0x0
#define MDM_TDFOESTCLKFORCE_RST 0x0
#define MDM_TBECLKFORCE_RST 0x0
#define MDM_FFT1MEMCLKFORCE_RST 0x0
#define MDM_FFT1CLKFORCE_RST 0x0
#define MDM_FFT0MEMCLKFORCE_RST 0x0
#define MDM_FFT0CLKFORCE_RST 0x0
#define MDM_CHESTCLKFORCE_RST 0x0
#define MDM_HMEMCLKFORCE_RST 0x0
#define MDM_INTLVMEMCLKFORCE_RST 0x0
#define MDM_VTB1CLKFORCE_RST 0x0
#define MDM_VTB0CLKFORCE_RST 0x0
#define MDM_BDRXCLKFORCE_RST 0x0
__INLINE void mdm_clkgatefctrl0_pack(uint8_t phytxclkforce, uint8_t bdtxclkforce, uint8_t agcmemclkforce, uint8_t agcclkforce, uint8_t rcclkforce, uint8_t feclkforce, uint8_t fdoclkforce, uint8_t equclkforce, uint8_t physvdclkforce, uint8_t phytxbfclkforce, uint8_t tdcompclkforce, uint8_t tdfoestclkforce, uint8_t tbeclkforce, uint8_t fft1memclkforce, uint8_t fft1clkforce, uint8_t fft0memclkforce, uint8_t fft0clkforce, uint8_t chestclkforce, uint8_t hmemclkforce, uint8_t intlvmemclkforce, uint8_t vtb1clkforce, uint8_t vtb0clkforce, uint8_t bdrxclkforce)
{
    ASSERT_ERR((((uint32_t)phytxclkforce << 31) & ~((uint32_t)0x80000000)) == 0);
    ASSERT_ERR((((uint32_t)bdtxclkforce << 30) & ~((uint32_t)0x40000000)) == 0);
    ASSERT_ERR((((uint32_t)agcmemclkforce << 29) & ~((uint32_t)0x20000000)) == 0);
    ASSERT_ERR((((uint32_t)agcclkforce << 28) & ~((uint32_t)0x10000000)) == 0);
    ASSERT_ERR((((uint32_t)rcclkforce << 27) & ~((uint32_t)0x08000000)) == 0);
    ASSERT_ERR((((uint32_t)feclkforce << 24) & ~((uint32_t)0x01000000)) == 0);
    ASSERT_ERR((((uint32_t)fdoclkforce << 23) & ~((uint32_t)0x00800000)) == 0);
    ASSERT_ERR((((uint32_t)equclkforce << 22) & ~((uint32_t)0x00400000)) == 0);
    ASSERT_ERR((((uint32_t)physvdclkforce << 21) & ~((uint32_t)0x00200000)) == 0);
    ASSERT_ERR((((uint32_t)phytxbfclkforce << 20) & ~((uint32_t)0x00100000)) == 0);
    ASSERT_ERR((((uint32_t)tdcompclkforce << 18) & ~((uint32_t)0x00040000)) == 0);
    ASSERT_ERR((((uint32_t)tdfoestclkforce << 17) & ~((uint32_t)0x00020000)) == 0);
    ASSERT_ERR((((uint32_t)tbeclkforce << 16) & ~((uint32_t)0x00010000)) == 0);
    ASSERT_ERR((((uint32_t)fft1memclkforce << 10) & ~((uint32_t)0x00000400)) == 0);
    ASSERT_ERR((((uint32_t)fft1clkforce << 9) & ~((uint32_t)0x00000200)) == 0);
    ASSERT_ERR((((uint32_t)fft0memclkforce << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)fft0clkforce << 7) & ~((uint32_t)0x00000080)) == 0);
    ASSERT_ERR((((uint32_t)chestclkforce << 6) & ~((uint32_t)0x00000040)) == 0);
    ASSERT_ERR((((uint32_t)hmemclkforce << 4) & ~((uint32_t)0x00000010)) == 0);
    ASSERT_ERR((((uint32_t)intlvmemclkforce << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)vtb1clkforce << 2) & ~((uint32_t)0x00000004)) == 0);
    ASSERT_ERR((((uint32_t)vtb0clkforce << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)bdrxclkforce << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, ((uint32_t)phytxclkforce << 31) | ((uint32_t)bdtxclkforce << 30) | ((uint32_t)agcmemclkforce << 29) | ((uint32_t)agcclkforce << 28) | ((uint32_t)rcclkforce << 27) | ((uint32_t)feclkforce << 24) | ((uint32_t)fdoclkforce << 23) | ((uint32_t)equclkforce << 22) | ((uint32_t)physvdclkforce << 21) | ((uint32_t)phytxbfclkforce << 20) | ((uint32_t)tdcompclkforce << 18) | ((uint32_t)tdfoestclkforce << 17) | ((uint32_t)tbeclkforce << 16) | ((uint32_t)fft1memclkforce << 10) | ((uint32_t)fft1clkforce << 9) | ((uint32_t)fft0memclkforce << 8) | ((uint32_t)fft0clkforce << 7) | ((uint32_t)chestclkforce << 6) | ((uint32_t)hmemclkforce << 4) | ((uint32_t)intlvmemclkforce << 3) | ((uint32_t)vtb1clkforce << 2) | ((uint32_t)vtb0clkforce << 1) | ((uint32_t)bdrxclkforce << 0));
}
__INLINE void mdm_clkgatefctrl0_unpack(uint8_t* phytxclkforce, uint8_t* bdtxclkforce, uint8_t* agcmemclkforce, uint8_t* agcclkforce, uint8_t* rcclkforce, uint8_t* feclkforce, uint8_t* fdoclkforce, uint8_t* equclkforce, uint8_t* physvdclkforce, uint8_t* phytxbfclkforce, uint8_t* tdcompclkforce, uint8_t* tdfoestclkforce, uint8_t* tbeclkforce, uint8_t* fft1memclkforce, uint8_t* fft1clkforce, uint8_t* fft0memclkforce, uint8_t* fft0clkforce, uint8_t* chestclkforce, uint8_t* hmemclkforce, uint8_t* intlvmemclkforce, uint8_t* vtb1clkforce, uint8_t* vtb0clkforce, uint8_t* bdrxclkforce)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    *phytxclkforce = (localVal & ((uint32_t)0x80000000)) >> 31;
    *bdtxclkforce = (localVal & ((uint32_t)0x40000000)) >> 30;
    *agcmemclkforce = (localVal & ((uint32_t)0x20000000)) >> 29;
    *agcclkforce = (localVal & ((uint32_t)0x10000000)) >> 28;
    *rcclkforce = (localVal & ((uint32_t)0x08000000)) >> 27;
    *feclkforce = (localVal & ((uint32_t)0x01000000)) >> 24;
    *fdoclkforce = (localVal & ((uint32_t)0x00800000)) >> 23;
    *equclkforce = (localVal & ((uint32_t)0x00400000)) >> 22;
    *physvdclkforce = (localVal & ((uint32_t)0x00200000)) >> 21;
    *phytxbfclkforce = (localVal & ((uint32_t)0x00100000)) >> 20;
    *tdcompclkforce = (localVal & ((uint32_t)0x00040000)) >> 18;
    *tdfoestclkforce = (localVal & ((uint32_t)0x00020000)) >> 17;
    *tbeclkforce = (localVal & ((uint32_t)0x00010000)) >> 16;
    *fft1memclkforce = (localVal & ((uint32_t)0x00000400)) >> 10;
    *fft1clkforce = (localVal & ((uint32_t)0x00000200)) >> 9;
    *fft0memclkforce = (localVal & ((uint32_t)0x00000100)) >> 8;
    *fft0clkforce = (localVal & ((uint32_t)0x00000080)) >> 7;
    *chestclkforce = (localVal & ((uint32_t)0x00000040)) >> 6;
    *hmemclkforce = (localVal & ((uint32_t)0x00000010)) >> 4;
    *intlvmemclkforce = (localVal & ((uint32_t)0x00000008)) >> 3;
    *vtb1clkforce = (localVal & ((uint32_t)0x00000004)) >> 2;
    *vtb0clkforce = (localVal & ((uint32_t)0x00000002)) >> 1;
    *bdrxclkforce = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_phytxclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x80000000)) >> 31);
}
__INLINE void mdm_phytxclkforce_setf(uint8_t phytxclkforce)
{
    ASSERT_ERR((((uint32_t)phytxclkforce << 31) & ~((uint32_t)0x80000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x80000000)) | ((uint32_t)phytxclkforce << 31));
}
__INLINE uint8_t mdm_bdtxclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x40000000)) >> 30);
}
__INLINE void mdm_bdtxclkforce_setf(uint8_t bdtxclkforce)
{
    ASSERT_ERR((((uint32_t)bdtxclkforce << 30) & ~((uint32_t)0x40000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x40000000)) | ((uint32_t)bdtxclkforce << 30));
}
__INLINE uint8_t mdm_agcmemclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x20000000)) >> 29);
}
__INLINE void mdm_agcmemclkforce_setf(uint8_t agcmemclkforce)
{
    ASSERT_ERR((((uint32_t)agcmemclkforce << 29) & ~((uint32_t)0x20000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x20000000)) | ((uint32_t)agcmemclkforce << 29));
}
__INLINE uint8_t mdm_agcclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x10000000)) >> 28);
}
__INLINE void mdm_agcclkforce_setf(uint8_t agcclkforce)
{
    ASSERT_ERR((((uint32_t)agcclkforce << 28) & ~((uint32_t)0x10000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x10000000)) | ((uint32_t)agcclkforce << 28));
}
__INLINE uint8_t mdm_rcclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x08000000)) >> 27);
}
__INLINE void mdm_rcclkforce_setf(uint8_t rcclkforce)
{
    ASSERT_ERR((((uint32_t)rcclkforce << 27) & ~((uint32_t)0x08000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x08000000)) | ((uint32_t)rcclkforce << 27));
}
__INLINE uint8_t mdm_feclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x01000000)) >> 24);
}
__INLINE void mdm_feclkforce_setf(uint8_t feclkforce)
{
    ASSERT_ERR((((uint32_t)feclkforce << 24) & ~((uint32_t)0x01000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x01000000)) | ((uint32_t)feclkforce << 24));
}
__INLINE uint8_t mdm_fdoclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00800000)) >> 23);
}
__INLINE void mdm_fdoclkforce_setf(uint8_t fdoclkforce)
{
    ASSERT_ERR((((uint32_t)fdoclkforce << 23) & ~((uint32_t)0x00800000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00800000)) | ((uint32_t)fdoclkforce << 23));
}
__INLINE uint8_t mdm_equclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00400000)) >> 22);
}
__INLINE void mdm_equclkforce_setf(uint8_t equclkforce)
{
    ASSERT_ERR((((uint32_t)equclkforce << 22) & ~((uint32_t)0x00400000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00400000)) | ((uint32_t)equclkforce << 22));
}
__INLINE uint8_t mdm_physvdclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00200000)) >> 21);
}
__INLINE void mdm_physvdclkforce_setf(uint8_t physvdclkforce)
{
    ASSERT_ERR((((uint32_t)physvdclkforce << 21) & ~((uint32_t)0x00200000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00200000)) | ((uint32_t)physvdclkforce << 21));
}
__INLINE uint8_t mdm_phytxbfclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00100000)) >> 20);
}
__INLINE void mdm_phytxbfclkforce_setf(uint8_t phytxbfclkforce)
{
    ASSERT_ERR((((uint32_t)phytxbfclkforce << 20) & ~((uint32_t)0x00100000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00100000)) | ((uint32_t)phytxbfclkforce << 20));
}
__INLINE uint8_t mdm_tdcompclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00040000)) >> 18);
}
__INLINE void mdm_tdcompclkforce_setf(uint8_t tdcompclkforce)
{
    ASSERT_ERR((((uint32_t)tdcompclkforce << 18) & ~((uint32_t)0x00040000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00040000)) | ((uint32_t)tdcompclkforce << 18));
}
__INLINE uint8_t mdm_tdfoestclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00020000)) >> 17);
}
__INLINE void mdm_tdfoestclkforce_setf(uint8_t tdfoestclkforce)
{
    ASSERT_ERR((((uint32_t)tdfoestclkforce << 17) & ~((uint32_t)0x00020000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00020000)) | ((uint32_t)tdfoestclkforce << 17));
}
__INLINE uint8_t mdm_tbeclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00010000)) >> 16);
}
__INLINE void mdm_tbeclkforce_setf(uint8_t tbeclkforce)
{
    ASSERT_ERR((((uint32_t)tbeclkforce << 16) & ~((uint32_t)0x00010000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00010000)) | ((uint32_t)tbeclkforce << 16));
}
__INLINE uint8_t mdm_fft1memclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000400)) >> 10);
}
__INLINE void mdm_fft1memclkforce_setf(uint8_t fft1memclkforce)
{
    ASSERT_ERR((((uint32_t)fft1memclkforce << 10) & ~((uint32_t)0x00000400)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000400)) | ((uint32_t)fft1memclkforce << 10));
}
__INLINE uint8_t mdm_fft1clkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000200)) >> 9);
}
__INLINE void mdm_fft1clkforce_setf(uint8_t fft1clkforce)
{
    ASSERT_ERR((((uint32_t)fft1clkforce << 9) & ~((uint32_t)0x00000200)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000200)) | ((uint32_t)fft1clkforce << 9));
}
__INLINE uint8_t mdm_fft0memclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}
__INLINE void mdm_fft0memclkforce_setf(uint8_t fft0memclkforce)
{
    ASSERT_ERR((((uint32_t)fft0memclkforce << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000100)) | ((uint32_t)fft0memclkforce << 8));
}
__INLINE uint8_t mdm_fft0clkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000080)) >> 7);
}
__INLINE void mdm_fft0clkforce_setf(uint8_t fft0clkforce)
{
    ASSERT_ERR((((uint32_t)fft0clkforce << 7) & ~((uint32_t)0x00000080)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000080)) | ((uint32_t)fft0clkforce << 7));
}
__INLINE uint8_t mdm_chestclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000040)) >> 6);
}
__INLINE void mdm_chestclkforce_setf(uint8_t chestclkforce)
{
    ASSERT_ERR((((uint32_t)chestclkforce << 6) & ~((uint32_t)0x00000040)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000040)) | ((uint32_t)chestclkforce << 6));
}
__INLINE uint8_t mdm_hmemclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000010)) >> 4);
}
__INLINE void mdm_hmemclkforce_setf(uint8_t hmemclkforce)
{
    ASSERT_ERR((((uint32_t)hmemclkforce << 4) & ~((uint32_t)0x00000010)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000010)) | ((uint32_t)hmemclkforce << 4));
}
__INLINE uint8_t mdm_intlvmemclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void mdm_intlvmemclkforce_setf(uint8_t intlvmemclkforce)
{
    ASSERT_ERR((((uint32_t)intlvmemclkforce << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000008)) | ((uint32_t)intlvmemclkforce << 3));
}
__INLINE uint8_t mdm_vtb1clkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}
__INLINE void mdm_vtb1clkforce_setf(uint8_t vtb1clkforce)
{
    ASSERT_ERR((((uint32_t)vtb1clkforce << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000004)) | ((uint32_t)vtb1clkforce << 2));
}
__INLINE uint8_t mdm_vtb0clkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE void mdm_vtb0clkforce_setf(uint8_t vtb0clkforce)
{
    ASSERT_ERR((((uint32_t)vtb0clkforce << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000002)) | ((uint32_t)vtb0clkforce << 1));
}
__INLINE uint8_t mdm_bdrxclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_bdrxclkforce_setf(uint8_t bdrxclkforce)
{
    ASSERT_ERR((((uint32_t)bdrxclkforce << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)bdrxclkforce << 0));
}
#define MDM_CLKGATEFCTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x0878)
#define MDM_CLKGATEFCTRL1_OFFSET 0x00000078
#define MDM_CLKGATEFCTRL1_INDEX 0x0000001E
#define MDM_CLKGATEFCTRL1_RESET 0x00000000
__INLINE uint32_t mdm_clkgatefctrl1_get(void)
{
    return REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
}
__INLINE void mdm_clkgatefctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, value);
}
#define MDM_RADARTIMCLKFORCE_BIT ((uint32_t)0x00000020)
#define MDM_RADARTIMCLKFORCE_POS 5
#define MDM_MDMBTXCLKFORCE_BIT ((uint32_t)0x00000010)
#define MDM_MDMBTXCLKFORCE_POS 4
#define MDM_MDMBRXCLKFORCE_BIT ((uint32_t)0x00000008)
#define MDM_MDMBRXCLKFORCE_POS 3
#define MDM_LDPCENCCLKFORCE_BIT ((uint32_t)0x00000002)
#define MDM_LDPCENCCLKFORCE_POS 1
#define MDM_LDPCDECCLKFORCE_BIT ((uint32_t)0x00000001)
#define MDM_LDPCDECCLKFORCE_POS 0
#define MDM_RADARTIMCLKFORCE_RST 0x0
#define MDM_MDMBTXCLKFORCE_RST 0x0
#define MDM_MDMBRXCLKFORCE_RST 0x0
#define MDM_LDPCENCCLKFORCE_RST 0x0
#define MDM_LDPCDECCLKFORCE_RST 0x0
__INLINE void mdm_clkgatefctrl1_pack(uint8_t radartimclkforce, uint8_t mdmbtxclkforce, uint8_t mdmbrxclkforce, uint8_t ldpcencclkforce, uint8_t ldpcdecclkforce)
{
    ASSERT_ERR((((uint32_t)radartimclkforce << 5) & ~((uint32_t)0x00000020)) == 0);
    ASSERT_ERR((((uint32_t)mdmbtxclkforce << 4) & ~((uint32_t)0x00000010)) == 0);
    ASSERT_ERR((((uint32_t)mdmbrxclkforce << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)ldpcencclkforce << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)ldpcdecclkforce << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, ((uint32_t)radartimclkforce << 5) | ((uint32_t)mdmbtxclkforce << 4) | ((uint32_t)mdmbrxclkforce << 3) | ((uint32_t)ldpcencclkforce << 1) | ((uint32_t)ldpcdecclkforce << 0));
}
__INLINE void mdm_clkgatefctrl1_unpack(uint8_t* radartimclkforce, uint8_t* mdmbtxclkforce, uint8_t* mdmbrxclkforce, uint8_t* ldpcencclkforce, uint8_t* ldpcdecclkforce)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
    *radartimclkforce = (localVal & ((uint32_t)0x00000020)) >> 5;
    *mdmbtxclkforce = (localVal & ((uint32_t)0x00000010)) >> 4;
    *mdmbrxclkforce = (localVal & ((uint32_t)0x00000008)) >> 3;
    *ldpcencclkforce = (localVal & ((uint32_t)0x00000002)) >> 1;
    *ldpcdecclkforce = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_radartimclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000020)) >> 5);
}
__INLINE void mdm_radartimclkforce_setf(uint8_t radartimclkforce)
{
    ASSERT_ERR((((uint32_t)radartimclkforce << 5) & ~((uint32_t)0x00000020)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR) & ~((uint32_t)0x00000020)) | ((uint32_t)radartimclkforce << 5));
}
__INLINE uint8_t mdm_mdmbtxclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000010)) >> 4);
}
__INLINE void mdm_mdmbtxclkforce_setf(uint8_t mdmbtxclkforce)
{
    ASSERT_ERR((((uint32_t)mdmbtxclkforce << 4) & ~((uint32_t)0x00000010)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR) & ~((uint32_t)0x00000010)) | ((uint32_t)mdmbtxclkforce << 4));
}
__INLINE uint8_t mdm_mdmbrxclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void mdm_mdmbrxclkforce_setf(uint8_t mdmbrxclkforce)
{
    ASSERT_ERR((((uint32_t)mdmbrxclkforce << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR) & ~((uint32_t)0x00000008)) | ((uint32_t)mdmbrxclkforce << 3));
}
__INLINE uint8_t mdm_ldpcencclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE void mdm_ldpcencclkforce_setf(uint8_t ldpcencclkforce)
{
    ASSERT_ERR((((uint32_t)ldpcencclkforce << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR) & ~((uint32_t)0x00000002)) | ((uint32_t)ldpcencclkforce << 1));
}
__INLINE uint8_t mdm_ldpcdecclkforce_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_ldpcdecclkforce_setf(uint8_t ldpcdecclkforce)
{
    ASSERT_ERR((((uint32_t)ldpcdecclkforce << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL1_ADDR, (REG_PL_RD(MDM_CLKGATEFCTRL1_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)ldpcdecclkforce << 0));
}
#define MDM_SWRESET_ADDR (REG_MDM_CFG_BASE_ADDR+0x0888)
#define MDM_SWRESET_OFFSET 0x00000088
#define MDM_SWRESET_INDEX 0x00000022
#define MDM_SWRESET_RESET 0x00000000
__INLINE uint32_t mdm_swreset_get(void)
{
    return REG_PL_RD(MDM_SWRESET_ADDR);
}
__INLINE void mdm_swreset_set(uint32_t value)
{
    REG_PL_WR(MDM_SWRESET_ADDR, value);
}
#define MDM_AGCSWRESET_BIT ((uint32_t)0x00001000)
#define MDM_AGCSWRESET_POS 12
#define MDM_DSPSWRESET_BIT ((uint32_t)0x00000100)
#define MDM_DSPSWRESET_POS 8
#define MDM_MDMSWRESET_BIT ((uint32_t)0x00000001)
#define MDM_MDMSWRESET_POS 0
#define MDM_AGCSWRESET_RST 0x0
#define MDM_DSPSWRESET_RST 0x0
#define MDM_MDMSWRESET_RST 0x0
__INLINE void mdm_swreset_pack(uint8_t agcswreset, uint8_t dspswreset, uint8_t mdmswreset)
{
    ASSERT_ERR((((uint32_t)agcswreset << 12) & ~((uint32_t)0x00001000)) == 0);
    ASSERT_ERR((((uint32_t)dspswreset << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)mdmswreset << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_SWRESET_ADDR, ((uint32_t)agcswreset << 12) | ((uint32_t)dspswreset << 8) | ((uint32_t)mdmswreset << 0));
}
__INLINE void mdm_swreset_unpack(uint8_t* agcswreset, uint8_t* dspswreset, uint8_t* mdmswreset)
{
    uint32_t localVal = REG_PL_RD(MDM_SWRESET_ADDR);
    *agcswreset = (localVal & ((uint32_t)0x00001000)) >> 12;
    *dspswreset = (localVal & ((uint32_t)0x00000100)) >> 8;
    *mdmswreset = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t mdm_agcswreset_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SWRESET_ADDR);
    return ((localVal & ((uint32_t)0x00001000)) >> 12);
}
__INLINE void mdm_agcswreset_setf(uint8_t agcswreset)
{
    ASSERT_ERR((((uint32_t)agcswreset << 12) & ~((uint32_t)0x00001000)) == 0);
    REG_PL_WR(MDM_SWRESET_ADDR, (REG_PL_RD(MDM_SWRESET_ADDR) & ~((uint32_t)0x00001000)) | ((uint32_t)agcswreset << 12));
}
__INLINE uint8_t mdm_dspswreset_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SWRESET_ADDR);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}
__INLINE void mdm_dspswreset_setf(uint8_t dspswreset)
{
    ASSERT_ERR((((uint32_t)dspswreset << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(MDM_SWRESET_ADDR, (REG_PL_RD(MDM_SWRESET_ADDR) & ~((uint32_t)0x00000100)) | ((uint32_t)dspswreset << 8));
}
__INLINE uint8_t mdm_mdmswreset_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SWRESET_ADDR);
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void mdm_mdmswreset_setf(uint8_t mdmswreset)
{
    ASSERT_ERR((((uint32_t)mdmswreset << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(MDM_SWRESET_ADDR, (REG_PL_RD(MDM_SWRESET_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)mdmswreset << 0));
}
#define MDM_TXCTRL1_ADDR (REG_MDM_CFG_BASE_ADDR+0x088C)
#define MDM_TXCTRL1_OFFSET 0x0000008C
#define MDM_TXCTRL1_INDEX 0x00000023
#define MDM_TXCTRL1_RESET 0x00001C30
__INLINE uint32_t mdm_txctrl1_get(void)
{
    return REG_PL_RD(MDM_TXCTRL1_ADDR);
}
__INLINE void mdm_txctrl1_set(uint32_t value)
{
    REG_PL_WR(MDM_TXCTRL1_ADDR, value);
}
#define MDM_TXFEOFDM80DELAY_MASK ((uint32_t)0xFF000000)
#define MDM_TXFEOFDM80DELAY_LSB 24
#define MDM_TXFEOFDM80DELAY_WIDTH ((uint32_t)0x00000008)
#define MDM_TXFEOFDM40DELAY_MASK ((uint32_t)0x00FF0000)
#define MDM_TXFEOFDM40DELAY_LSB 16
#define MDM_TXFEOFDM40DELAY_WIDTH ((uint32_t)0x00000008)
#define MDM_TXFEOFDM20DELAY_MASK ((uint32_t)0x0000FF00)
#define MDM_TXFEOFDM20DELAY_LSB 8
#define MDM_TXFEOFDM20DELAY_WIDTH ((uint32_t)0x00000008)
#define MDM_TXFEDSSSDELAY_MASK ((uint32_t)0x000000FF)
#define MDM_TXFEDSSSDELAY_LSB 0
#define MDM_TXFEDSSSDELAY_WIDTH ((uint32_t)0x00000008)
#define MDM_TXFEOFDM80DELAY_RST 0x0
#define MDM_TXFEOFDM40DELAY_RST 0x0
#define MDM_TXFEOFDM20DELAY_RST 0x1C
#define MDM_TXFEDSSSDELAY_RST 0x30
__INLINE void mdm_txctrl1_pack(uint8_t txfeofdm80delay, uint8_t txfeofdm40delay, uint8_t txfeofdm20delay, uint8_t txfedsssdelay)
{
    ASSERT_ERR((((uint32_t)txfeofdm80delay << 24) & ~((uint32_t)0xFF000000)) == 0);
    ASSERT_ERR((((uint32_t)txfeofdm40delay << 16) & ~((uint32_t)0x00FF0000)) == 0);
    ASSERT_ERR((((uint32_t)txfeofdm20delay << 8) & ~((uint32_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint32_t)txfedsssdelay << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TXCTRL1_ADDR, ((uint32_t)txfeofdm80delay << 24) | ((uint32_t)txfeofdm40delay << 16) | ((uint32_t)txfeofdm20delay << 8) | ((uint32_t)txfedsssdelay << 0));
}
__INLINE void mdm_txctrl1_unpack(uint8_t* txfeofdm80delay, uint8_t* txfeofdm40delay, uint8_t* txfeofdm20delay, uint8_t* txfedsssdelay)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL1_ADDR);
    *txfeofdm80delay = (localVal & ((uint32_t)0xFF000000)) >> 24;
    *txfeofdm40delay = (localVal & ((uint32_t)0x00FF0000)) >> 16;
    *txfeofdm20delay = (localVal & ((uint32_t)0x0000FF00)) >> 8;
    *txfedsssdelay = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_txfeofdm80delay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL1_ADDR);
    return ((localVal & ((uint32_t)0xFF000000)) >> 24);
}
__INLINE void mdm_txfeofdm80delay_setf(uint8_t txfeofdm80delay)
{
    ASSERT_ERR((((uint32_t)txfeofdm80delay << 24) & ~((uint32_t)0xFF000000)) == 0);
    REG_PL_WR(MDM_TXCTRL1_ADDR, (REG_PL_RD(MDM_TXCTRL1_ADDR) & ~((uint32_t)0xFF000000)) | ((uint32_t)txfeofdm80delay << 24));
}
__INLINE uint8_t mdm_txfeofdm40delay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x00FF0000)) >> 16);
}
__INLINE void mdm_txfeofdm40delay_setf(uint8_t txfeofdm40delay)
{
    ASSERT_ERR((((uint32_t)txfeofdm40delay << 16) & ~((uint32_t)0x00FF0000)) == 0);
    REG_PL_WR(MDM_TXCTRL1_ADDR, (REG_PL_RD(MDM_TXCTRL1_ADDR) & ~((uint32_t)0x00FF0000)) | ((uint32_t)txfeofdm40delay << 16));
}
__INLINE uint8_t mdm_txfeofdm20delay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x0000FF00)) >> 8);
}
__INLINE void mdm_txfeofdm20delay_setf(uint8_t txfeofdm20delay)
{
    ASSERT_ERR((((uint32_t)txfeofdm20delay << 8) & ~((uint32_t)0x0000FF00)) == 0);
    REG_PL_WR(MDM_TXCTRL1_ADDR, (REG_PL_RD(MDM_TXCTRL1_ADDR) & ~((uint32_t)0x0000FF00)) | ((uint32_t)txfeofdm20delay << 8));
}
__INLINE uint8_t mdm_txfedsssdelay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL1_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_txfedsssdelay_setf(uint8_t txfedsssdelay)
{
    ASSERT_ERR((((uint32_t)txfedsssdelay << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TXCTRL1_ADDR, (REG_PL_RD(MDM_TXCTRL1_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)txfedsssdelay << 0));
}
#define MDM_TXCTRL2_ADDR (REG_MDM_CFG_BASE_ADDR+0x0890)
#define MDM_TXCTRL2_OFFSET 0x00000090
#define MDM_TXCTRL2_INDEX 0x00000024
#define MDM_TXCTRL2_RESET 0x00F0F8FC
__INLINE uint32_t mdm_txctrl2_get(void)
{
    return REG_PL_RD(MDM_TXCTRL2_ADDR);
}
__INLINE void mdm_txctrl2_set(uint32_t value)
{
    REG_PL_WR(MDM_TXCTRL2_ADDR, value);
}
#define MDM_TXTDCSD80_MASK ((uint32_t)0x00FF0000)
#define MDM_TXTDCSD80_LSB 16
#define MDM_TXTDCSD80_WIDTH ((uint32_t)0x00000008)
#define MDM_TXTDCSD40_MASK ((uint32_t)0x0000FF00)
#define MDM_TXTDCSD40_LSB 8
#define MDM_TXTDCSD40_WIDTH ((uint32_t)0x00000008)
#define MDM_TXTDCSD20_MASK ((uint32_t)0x000000FF)
#define MDM_TXTDCSD20_LSB 0
#define MDM_TXTDCSD20_WIDTH ((uint32_t)0x00000008)
#define MDM_TXTDCSD80_RST 0xF0
#define MDM_TXTDCSD40_RST 0xF8
#define MDM_TXTDCSD20_RST 0xFC
__INLINE void mdm_txctrl2_pack(uint8_t txtdcsd80, uint8_t txtdcsd40, uint8_t txtdcsd20)
{
    ASSERT_ERR((((uint32_t)txtdcsd80 << 16) & ~((uint32_t)0x00FF0000)) == 0);
    ASSERT_ERR((((uint32_t)txtdcsd40 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint32_t)txtdcsd20 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TXCTRL2_ADDR, ((uint32_t)txtdcsd80 << 16) | ((uint32_t)txtdcsd40 << 8) | ((uint32_t)txtdcsd20 << 0));
}
__INLINE void mdm_txctrl2_unpack(uint8_t* txtdcsd80, uint8_t* txtdcsd40, uint8_t* txtdcsd20)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL2_ADDR);
    *txtdcsd80 = (localVal & ((uint32_t)0x00FF0000)) >> 16;
    *txtdcsd40 = (localVal & ((uint32_t)0x0000FF00)) >> 8;
    *txtdcsd20 = (localVal & ((uint32_t)0x000000FF)) >> 0;
}
__INLINE uint8_t mdm_txtdcsd80_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x00FF0000)) >> 16);
}
__INLINE void mdm_txtdcsd80_setf(uint8_t txtdcsd80)
{
    ASSERT_ERR((((uint32_t)txtdcsd80 << 16) & ~((uint32_t)0x00FF0000)) == 0);
    REG_PL_WR(MDM_TXCTRL2_ADDR, (REG_PL_RD(MDM_TXCTRL2_ADDR) & ~((uint32_t)0x00FF0000)) | ((uint32_t)txtdcsd80 << 16));
}
__INLINE uint8_t mdm_txtdcsd40_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x0000FF00)) >> 8);
}
__INLINE void mdm_txtdcsd40_setf(uint8_t txtdcsd40)
{
    ASSERT_ERR((((uint32_t)txtdcsd40 << 8) & ~((uint32_t)0x0000FF00)) == 0);
    REG_PL_WR(MDM_TXCTRL2_ADDR, (REG_PL_RD(MDM_TXCTRL2_ADDR) & ~((uint32_t)0x0000FF00)) | ((uint32_t)txtdcsd40 << 8));
}
__INLINE uint8_t mdm_txtdcsd20_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL2_ADDR);
    return ((localVal & ((uint32_t)0x000000FF)) >> 0);
}
__INLINE void mdm_txtdcsd20_setf(uint8_t txtdcsd20)
{
    ASSERT_ERR((((uint32_t)txtdcsd20 << 0) & ~((uint32_t)0x000000FF)) == 0);
    REG_PL_WR(MDM_TXCTRL2_ADDR, (REG_PL_RD(MDM_TXCTRL2_ADDR) & ~((uint32_t)0x000000FF)) | ((uint32_t)txtdcsd20 << 0));
}
#if RW_NX_LDPC_DEC
#define MDM_LDPCCTRL_ADDR (REG_MDM_CFG_BASE_ADDR+0x0894)
#define MDM_LDPCCTRL_OFFSET 0x00000094
#define MDM_LDPCCTRL_INDEX 0x00000025
#define MDM_LDPCCTRL_RESET 0x03F10002
__INLINE uint32_t mdm_ldpcctrl_get(void)
{
    return REG_PL_RD(MDM_LDPCCTRL_ADDR);
}
__INLINE void mdm_ldpcctrl_set(uint32_t value)
{
    REG_PL_WR(MDM_LDPCCTRL_ADDR, value);
}
#define MDM_LDPCDECSPLITCFG_MASK ((uint32_t)0x03F00000)
#define MDM_LDPCDECSPLITCFG_LSB 20
#define MDM_LDPCDECSPLITCFG_WIDTH ((uint32_t)0x00000006)
#define MDM_LDPCDECSPLITEN_BIT ((uint32_t)0x00010000)
#define MDM_LDPCDECSPLITEN_POS 16
#define MDM_LDPCDECBEATTIMELINE_BIT ((uint32_t)0x00001000)
#define MDM_LDPCDECBEATTIMELINE_POS 12
#define MDM_LDPCDECENLOSTTIME_BIT ((uint32_t)0x00000100)
#define MDM_LDPCDECENLOSTTIME_POS 8
#define MDM_LDPCDECLLRUNITY_MASK ((uint32_t)0x0000003F)
#define MDM_LDPCDECLLRUNITY_LSB 0
#define MDM_LDPCDECLLRUNITY_WIDTH ((uint32_t)0x00000006)
#define MDM_LDPCDECSPLITCFG_RST 0x3F
#define MDM_LDPCDECSPLITEN_RST 0x1
#define MDM_LDPCDECBEATTIMELINE_RST 0x0
#define MDM_LDPCDECENLOSTTIME_RST 0x0
#define MDM_LDPCDECLLRUNITY_RST 0x2
__INLINE void mdm_ldpcctrl_pack(uint8_t ldpcdecsplitcfg, uint8_t ldpcdecspliten, uint8_t ldpcdecbeattimeline, uint8_t ldpcdecenlosttime, uint8_t ldpcdecllrunity)
{
    ASSERT_ERR((((uint32_t)ldpcdecsplitcfg << 20) & ~((uint32_t)0x03F00000)) == 0);
    ASSERT_ERR((((uint32_t)ldpcdecspliten << 16) & ~((uint32_t)0x00010000)) == 0);
    ASSERT_ERR((((uint32_t)ldpcdecbeattimeline << 12) & ~((uint32_t)0x00001000)) == 0);
    ASSERT_ERR((((uint32_t)ldpcdecenlosttime << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)ldpcdecllrunity << 0) & ~((uint32_t)0x0000003F)) == 0);
    REG_PL_WR(MDM_LDPCCTRL_ADDR, ((uint32_t)ldpcdecsplitcfg << 20) | ((uint32_t)ldpcdecspliten << 16) | ((uint32_t)ldpcdecbeattimeline << 12) | ((uint32_t)ldpcdecenlosttime << 8) | ((uint32_t)ldpcdecllrunity << 0));
}
__INLINE void mdm_ldpcctrl_unpack(uint8_t* ldpcdecsplitcfg, uint8_t* ldpcdecspliten, uint8_t* ldpcdecbeattimeline, uint8_t* ldpcdecenlosttime, uint8_t* ldpcdecllrunity)
{
    uint32_t localVal = REG_PL_RD(MDM_LDPCCTRL_ADDR);
    *ldpcdecsplitcfg = (localVal & ((uint32_t)0x03F00000)) >> 20;
    *ldpcdecspliten = (localVal & ((uint32_t)0x00010000)) >> 16;
    *ldpcdecbeattimeline = (localVal & ((uint32_t)0x00001000)) >> 12;
    *ldpcdecenlosttime = (localVal & ((uint32_t)0x00000100)) >> 8;
    *ldpcdecllrunity = (localVal & ((uint32_t)0x0000003F)) >> 0;
}
__INLINE uint8_t mdm_ldpcdecsplitcfg_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_LDPCCTRL_ADDR);
    return ((localVal & ((uint32_t)0x03F00000)) >> 20);
}
__INLINE void mdm_ldpcdecsplitcfg_setf(uint8_t ldpcdecsplitcfg)
{
    ASSERT_ERR((((uint32_t)ldpcdecsplitcfg << 20) & ~((uint32_t)0x03F00000)) == 0);
    REG_PL_WR(MDM_LDPCCTRL_ADDR, (REG_PL_RD(MDM_LDPCCTRL_ADDR) & ~((uint32_t)0x03F00000)) | ((uint32_t)ldpcdecsplitcfg << 20));
}
__INLINE uint8_t mdm_ldpcdecspliten_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_LDPCCTRL_ADDR);
    return ((localVal & ((uint32_t)0x00010000)) >> 16);
}
__INLINE void mdm_ldpcdecspliten_setf(uint8_t ldpcdecspliten)
{
    ASSERT_ERR((((uint32_t)ldpcdecspliten << 16) & ~((uint32_t)0x00010000)) == 0);
    REG_PL_WR(MDM_LDPCCTRL_ADDR, (REG_PL_RD(MDM_LDPCCTRL_ADDR) & ~((uint32_t)0x00010000)) | ((uint32_t)ldpcdecspliten << 16));
}
__INLINE uint8_t mdm_ldpcdecbeattimeline_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_LDPCCTRL_ADDR);
    return ((localVal & ((uint32_t)0x00001000)) >> 12);
}
__INLINE void mdm_ldpcdecbeattimeline_setf(uint8_t ldpcdecbeattimeline)
{
    ASSERT_ERR((((uint32_t)ldpcdecbeattimeline << 12) & ~((uint32_t)0x00001000)) == 0);
    REG_PL_WR(MDM_LDPCCTRL_ADDR, (REG_PL_RD(MDM_LDPCCTRL_ADDR) & ~((uint32_t)0x00001000)) | ((uint32_t)ldpcdecbeattimeline << 12));
}
__INLINE uint8_t mdm_ldpcdecenlosttime_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_LDPCCTRL_ADDR);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}
__INLINE void mdm_ldpcdecenlosttime_setf(uint8_t ldpcdecenlosttime)
{
    ASSERT_ERR((((uint32_t)ldpcdecenlosttime << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(MDM_LDPCCTRL_ADDR, (REG_PL_RD(MDM_LDPCCTRL_ADDR) & ~((uint32_t)0x00000100)) | ((uint32_t)ldpcdecenlosttime << 8));
}
__INLINE uint8_t mdm_ldpcdecllrunity_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_LDPCCTRL_ADDR);
    return ((localVal & ((uint32_t)0x0000003F)) >> 0);
}
__INLINE void mdm_ldpcdecllrunity_setf(uint8_t ldpcdecllrunity)
{
    ASSERT_ERR((((uint32_t)ldpcdecllrunity << 0) & ~((uint32_t)0x0000003F)) == 0);
    REG_PL_WR(MDM_LDPCCTRL_ADDR, (REG_PL_RD(MDM_LDPCCTRL_ADDR) & ~((uint32_t)0x0000003F)) | ((uint32_t)ldpcdecllrunity << 0));
}
#endif
#define MDM_TXCTRL3_ADDR (REG_MDM_CFG_BASE_ADDR+0x0898)
#define MDM_TXCTRL3_OFFSET 0x00000098
#define MDM_TXCTRL3_INDEX 0x00000026
#define MDM_TXCTRL3_RESET 0x02D00438
__INLINE uint32_t mdm_txctrl3_get(void)
{
    return REG_PL_RD(MDM_TXCTRL3_ADDR);
}
__INLINE void mdm_txctrl3_set(uint32_t value)
{
    REG_PL_WR(MDM_TXCTRL3_ADDR, value);
}
#define MDM_TXPHYRDYHTDELAY_MASK ((uint32_t)0x0FFF0000)
#define MDM_TXPHYRDYHTDELAY_LSB 16
#define MDM_TXPHYRDYHTDELAY_WIDTH ((uint32_t)0x0000000C)
#define MDM_TXPHYRDYNONHTDELAY_MASK ((uint32_t)0x00000FFF)
#define MDM_TXPHYRDYNONHTDELAY_LSB 0
#define MDM_TXPHYRDYNONHTDELAY_WIDTH ((uint32_t)0x0000000C)
#define MDM_TXPHYRDYHTDELAY_RST 0x2D0
#define MDM_TXPHYRDYNONHTDELAY_RST 0x438
__INLINE void mdm_txctrl3_pack(uint16_t txphyrdyhtdelay, uint16_t txphyrdynonhtdelay)
{
    ASSERT_ERR((((uint32_t)txphyrdyhtdelay << 16) & ~((uint32_t)0x0FFF0000)) == 0);
    ASSERT_ERR((((uint32_t)txphyrdynonhtdelay << 0) & ~((uint32_t)0x00000FFF)) == 0);
    REG_PL_WR(MDM_TXCTRL3_ADDR, ((uint32_t)txphyrdyhtdelay << 16) | ((uint32_t)txphyrdynonhtdelay << 0));
}
__INLINE void mdm_txctrl3_unpack(uint16_t* txphyrdyhtdelay, uint16_t* txphyrdynonhtdelay)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL3_ADDR);
    *txphyrdyhtdelay = (localVal & ((uint32_t)0x0FFF0000)) >> 16;
    *txphyrdynonhtdelay = (localVal & ((uint32_t)0x00000FFF)) >> 0;
}
__INLINE uint16_t mdm_txphyrdyhtdelay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x0FFF0000)) >> 16);
}
__INLINE void mdm_txphyrdyhtdelay_setf(uint16_t txphyrdyhtdelay)
{
    ASSERT_ERR((((uint32_t)txphyrdyhtdelay << 16) & ~((uint32_t)0x0FFF0000)) == 0);
    REG_PL_WR(MDM_TXCTRL3_ADDR, (REG_PL_RD(MDM_TXCTRL3_ADDR) & ~((uint32_t)0x0FFF0000)) | ((uint32_t)txphyrdyhtdelay << 16));
}
__INLINE uint16_t mdm_txphyrdynonhtdelay_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_TXCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x00000FFF)) >> 0);
}
__INLINE void mdm_txphyrdynonhtdelay_setf(uint16_t txphyrdynonhtdelay)
{
    ASSERT_ERR((((uint32_t)txphyrdynonhtdelay << 0) & ~((uint32_t)0x00000FFF)) == 0);
    REG_PL_WR(MDM_TXCTRL3_ADDR, (REG_PL_RD(MDM_TXCTRL3_ADDR) & ~((uint32_t)0x00000FFF)) | ((uint32_t)txphyrdynonhtdelay << 0));
}
#define MDM_RXCTRL2_ADDR (REG_MDM_CFG_BASE_ADDR+0x089C)
#define MDM_RXCTRL2_OFFSET 0x0000009C
#define MDM_RXCTRL2_INDEX 0x00000027
#define MDM_RXCTRL2_RESET 0xFFFFFFFF
__INLINE uint32_t mdm_rxctrl2_get(void)
{
    return REG_PL_RD(MDM_RXCTRL2_ADDR);
}
__INLINE void mdm_rxctrl2_set(uint32_t value)
{
    REG_PL_WR(MDM_RXCTRL2_ADDR, value);
}
#define MDM_RXFRAMEVIOLATIONMASK_MASK ((uint32_t)0xFFFFFFFF)
#define MDM_RXFRAMEVIOLATIONMASK_LSB 0
#define MDM_RXFRAMEVIOLATIONMASK_WIDTH ((uint32_t)0x00000020)
#define MDM_RXFRAMEVIOLATIONMASK_RST 0xFFFFFFFF
__INLINE uint32_t mdm_rxframeviolationmask_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL2_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_rxframeviolationmask_setf(uint32_t rxframeviolationmask)
{
    ASSERT_ERR((((uint32_t)rxframeviolationmask << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(MDM_RXCTRL2_ADDR, (uint32_t)rxframeviolationmask << 0);
}
#endif
#if RW_NX_LDPC_DEC
#define MDM_RXCTRL3_ADDR(band) (REG_MDM_CFG_BASE_ADDR(band)+0x08A0)
#define MDM_RXCTRL3_OFFSET 0x000000A0
#define MDM_RXCTRL3_INDEX 0x00000028
#define MDM_RXCTRL3_RESET 0x0C300C30
#if 0
__INLINE uint32_t mdm_rxctrl3_get(void)
{
    return REG_PL_RD(MDM_RXCTRL3_ADDR);
}
__INLINE void mdm_rxctrl3_set(uint32_t value)
{
    REG_PL_WR(MDM_RXCTRL3_ADDR, value);
}
#define MDM_RXLDPCNDBPSMAXSGI_MASK ((uint32_t)0xFFFF0000)
#define MDM_RXLDPCNDBPSMAXSGI_LSB 16
#define MDM_RXLDPCNDBPSMAXSGI_WIDTH ((uint32_t)0x00000010)
#define MDM_RXLDPCNDBPSMAXLGI_MASK ((uint32_t)0x0000FFFF)
#define MDM_RXLDPCNDBPSMAXLGI_LSB 0
#define MDM_RXLDPCNDBPSMAXLGI_WIDTH ((uint32_t)0x00000010)
#define MDM_RXLDPCNDBPSMAXSGI_RST 0xC30
#define MDM_RXLDPCNDBPSMAXLGI_RST 0xC30
__INLINE void mdm_rxctrl3_pack(uint16_t rxldpcndbpsmaxsgi, uint16_t rxldpcndbpsmaxlgi)
{
    ASSERT_ERR((((uint32_t)rxldpcndbpsmaxsgi << 16) & ~((uint32_t)0xFFFF0000)) == 0);
    ASSERT_ERR((((uint32_t)rxldpcndbpsmaxlgi << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(MDM_RXCTRL3_ADDR, ((uint32_t)rxldpcndbpsmaxsgi << 16) | ((uint32_t)rxldpcndbpsmaxlgi << 0));
}
__INLINE void mdm_rxctrl3_unpack(uint16_t* rxldpcndbpsmaxsgi, uint16_t* rxldpcndbpsmaxlgi)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL3_ADDR);
    *rxldpcndbpsmaxsgi = (localVal & ((uint32_t)0xFFFF0000)) >> 16;
    *rxldpcndbpsmaxlgi = (localVal & ((uint32_t)0x0000FFFF)) >> 0;
}
__INLINE uint16_t mdm_rxldpcndbpsmaxsgi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL3_ADDR);
    return ((localVal & ((uint32_t)0xFFFF0000)) >> 16);
}
__INLINE void mdm_rxldpcndbpsmaxsgi_setf(uint16_t rxldpcndbpsmaxsgi)
{
    ASSERT_ERR((((uint32_t)rxldpcndbpsmaxsgi << 16) & ~((uint32_t)0xFFFF0000)) == 0);
    REG_PL_WR(MDM_RXCTRL3_ADDR, (REG_PL_RD(MDM_RXCTRL3_ADDR) & ~((uint32_t)0xFFFF0000)) | ((uint32_t)rxldpcndbpsmaxsgi << 16));
}
__INLINE uint16_t mdm_rxldpcndbpsmaxlgi_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_RXCTRL3_ADDR);
    return ((localVal & ((uint32_t)0x0000FFFF)) >> 0);
}
__INLINE void mdm_rxldpcndbpsmaxlgi_setf(uint16_t rxldpcndbpsmaxlgi)
{
    ASSERT_ERR((((uint32_t)rxldpcndbpsmaxlgi << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(MDM_RXCTRL3_ADDR, (REG_PL_RD(MDM_RXCTRL3_ADDR) & ~((uint32_t)0x0000FFFF)) | ((uint32_t)rxldpcndbpsmaxlgi << 0));
}
#endif
#endif
#if 0
#if RW_BFMEE_EN
#define MDM_SVDCTRL_ADDR (REG_MDM_CFG_BASE_ADDR+0x08A4)
#define MDM_SVDCTRL_OFFSET 0x000000A4
#define MDM_SVDCTRL_INDEX 0x00000029
#define MDM_SVDCTRL_RESET 0x00300056
__INLINE uint32_t mdm_svdctrl_get(void)
{
    return REG_PL_RD(MDM_SVDCTRL_ADDR);
}
__INLINE void mdm_svdctrl_set(uint32_t value)
{
    REG_PL_WR(MDM_SVDCTRL_ADDR, value);
}
#define MDM_RXANTSEL_MASK ((uint32_t)0x00F00000)
#define MDM_RXANTSEL_LSB 20
#define MDM_RXANTSEL_WIDTH ((uint32_t)0x00000004)
#define MDM_SNRSCALING_MASK ((uint32_t)0x000003FF)
#define MDM_SNRSCALING_LSB 0
#define MDM_SNRSCALING_WIDTH ((uint32_t)0x0000000A)
#define MDM_RXANTSEL_RST 0x3
#define MDM_SNRSCALING_RST 0x56
__INLINE void mdm_svdctrl_pack(uint8_t rxantsel, uint16_t snrscaling)
{
    ASSERT_ERR((((uint32_t)rxantsel << 20) & ~((uint32_t)0x00F00000)) == 0);
    ASSERT_ERR((((uint32_t)snrscaling << 0) & ~((uint32_t)0x000003FF)) == 0);
    REG_PL_WR(MDM_SVDCTRL_ADDR, ((uint32_t)rxantsel << 20) | ((uint32_t)snrscaling << 0));
}
__INLINE void mdm_svdctrl_unpack(uint8_t* rxantsel, uint16_t* snrscaling)
{
    uint32_t localVal = REG_PL_RD(MDM_SVDCTRL_ADDR);
    *rxantsel = (localVal & ((uint32_t)0x00F00000)) >> 20;
    *snrscaling = (localVal & ((uint32_t)0x000003FF)) >> 0;
}
__INLINE uint8_t mdm_rxantsel_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SVDCTRL_ADDR);
    return ((localVal & ((uint32_t)0x00F00000)) >> 20);
}
__INLINE void mdm_rxantsel_setf(uint8_t rxantsel)
{
    ASSERT_ERR((((uint32_t)rxantsel << 20) & ~((uint32_t)0x00F00000)) == 0);
    REG_PL_WR(MDM_SVDCTRL_ADDR, (REG_PL_RD(MDM_SVDCTRL_ADDR) & ~((uint32_t)0x00F00000)) | ((uint32_t)rxantsel << 20));
}
__INLINE uint16_t mdm_snrscaling_getf(void)
{
    uint32_t localVal = REG_PL_RD(MDM_SVDCTRL_ADDR);
    return ((localVal & ((uint32_t)0x000003FF)) >> 0);
}
__INLINE void mdm_snrscaling_setf(uint16_t snrscaling)
{
    ASSERT_ERR((((uint32_t)snrscaling << 0) & ~((uint32_t)0x000003FF)) == 0);
    REG_PL_WR(MDM_SVDCTRL_ADDR, (REG_PL_RD(MDM_SVDCTRL_ADDR) & ~((uint32_t)0x000003FF)) | ((uint32_t)snrscaling << 0));
}
#endif
#if RW_MUMIMO_RX_EN
#define MDM_MUMIMO_GROUPID_TAB_ADDR (REG_MDM_CFG_BASE_ADDR+0x08A8)
#define MDM_MUMIMO_GROUPID_TAB_OFFSET 0x000000A8
#define MDM_MUMIMO_GROUPID_TAB_INDEX 0x0000002A
#define MDM_MUMIMO_GROUPID_TAB_RESET 0x00000000
#define MDM_MUMIMO_GROUPID_TAB_COUNT 2
__INLINE uint32_t mdm_mumimo_groupid_tab_get(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    return REG_PL_RD(MDM_MUMIMO_GROUPID_TAB_ADDR + reg_idx * 4);
}
__INLINE void mdm_mumimo_groupid_tab_set(int reg_idx, uint32_t value)
{
    ASSERT_ERR(reg_idx <= 1);
    REG_PL_WR(MDM_MUMIMO_GROUPID_TAB_ADDR + reg_idx * 4, value);
}
#define MDM_MEMBERSHIPSTAT_MASK ((uint32_t)0xFFFFFFFF)
#define MDM_MEMBERSHIPSTAT_LSB 0
#define MDM_MEMBERSHIPSTAT_WIDTH ((uint32_t)0x00000020)
#define MDM_MEMBERSHIPSTAT_RST 0x0
__INLINE uint32_t mdm_membershipstat_getf(int reg_idx)
{
    uint32_t localVal = REG_PL_RD(MDM_MUMIMO_GROUPID_TAB_ADDR + reg_idx * 4);
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_membershipstat_setf(int reg_idx, uint32_t membershipstat)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)membershipstat << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(MDM_MUMIMO_GROUPID_TAB_ADDR + reg_idx * 4, (uint32_t)membershipstat << 0);
}
#endif
#if RW_MUMIMO_RX_EN
#define MDM_MUMIMO_USERPOSITION_TAB_ADDR (REG_MDM_CFG_BASE_ADDR+0x08B0)
#define MDM_MUMIMO_USERPOSITION_TAB_OFFSET 0x000000B0
#define MDM_MUMIMO_USERPOSITION_TAB_INDEX 0x0000002C
#define MDM_MUMIMO_USERPOSITION_TAB_RESET 0x00000000
#define MDM_MUMIMO_USERPOSITION_TAB_COUNT 4
__INLINE uint32_t mdm_mumimo_userposition_tab_get(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 3);
    return REG_PL_RD(MDM_MUMIMO_USERPOSITION_TAB_ADDR + reg_idx * 4);
}
__INLINE void mdm_mumimo_userposition_tab_set(int reg_idx, uint32_t value)
{
    ASSERT_ERR(reg_idx <= 3);
    REG_PL_WR(MDM_MUMIMO_USERPOSITION_TAB_ADDR + reg_idx * 4, value);
}
#define MDM_USERPOSID_MASK ((uint32_t)0xFFFFFFFF)
#define MDM_USERPOSID_LSB 0
#define MDM_USERPOSID_WIDTH ((uint32_t)0x00000020)
#define MDM_USERPOSID_RST 0x0
__INLINE uint32_t mdm_userposid_getf(int reg_idx)
{
    uint32_t localVal = REG_PL_RD(MDM_MUMIMO_USERPOSITION_TAB_ADDR + reg_idx * 4);
    ASSERT_ERR(reg_idx <= 3);
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void mdm_userposid_setf(int reg_idx, uint32_t userposid)
{
    ASSERT_ERR(reg_idx <= 3);
    ASSERT_ERR((((uint32_t)userposid << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(MDM_MUMIMO_USERPOSITION_TAB_ADDR + reg_idx * 4, (uint32_t)userposid << 0);
}
#endif
#endif
#endif
