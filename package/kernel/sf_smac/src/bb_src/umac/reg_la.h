#ifndef _REG_LA_H_
#define _REG_LA_H_ 
#ifndef __KERNEL__
#include <stdint.h>
#include "arch.h"
#else
#include "ipc_compat.h"
#endif
#include "reg_access.h"
#define REG_LA_COUNT 16
#define REG_LA_BASE_ADDR(band) (WIFI_BASE_ADDR(band) + 0x00200000)
#define REG_LA_DECODING_MASK 0x0000003F
#define LA_ID_LOW_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0000)
#define LA_ID_LOW_OFFSET 0x00000000
#define LA_ID_LOW_INDEX 0x00000000
#define LA_ID_LOW_RESET 0x656E6562
__INLINE uint32_t la_id_low_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_ID_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}
#define LA_ID_LOW_MASK ((uint32_t)0xFFFFFFFF)
#define LA_ID_LOW_LSB 0
#define LA_ID_LOW_WIDTH ((uint32_t)0x00000020)
#define LA_ID_LOW_RST 0x656E6562
__INLINE uint32_t la_id_low_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_ID_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
#define LA_ID_HIGH_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0004)
#define LA_ID_HIGH_OFFSET 0x00000004
#define LA_ID_HIGH_INDEX 0x00000001
#define LA_ID_HIGH_RESET 0x00000000
#define LA_ID_HIGH_MASK 0xFFFFFFFF
__INLINE uint32_t la_id_high_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_ID_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}
#define LA_LA_DEPTH_MASK ((uint32_t)0x0000FFFF)
#define LA_LA_DEPTH_LSB 0
#define LA_LA_DEPTH_WIDTH ((uint32_t)0x00000010)
#define LA_LA_DEPTH_RST 0x0
__INLINE uint16_t la_la_depth_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_ID_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}
#define LA_VERSION_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0008)
#define LA_VERSION_OFFSET 0x00000008
#define LA_VERSION_INDEX 0x00000002
#define LA_VERSION_RESET 0x00040000
__INLINE uint32_t la_version_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
}
#define LA_SAMPLING_FREQ_MASK ((uint32_t)0xFF000000)
#define LA_SAMPLING_FREQ_LSB 24
#define LA_SAMPLING_FREQ_WIDTH ((uint32_t)0x00000008)
#define LA_LA_VERSION_MASK ((uint32_t)0x00FFFFFF)
#define LA_LA_VERSION_LSB 0
#define LA_LA_VERSION_WIDTH ((uint32_t)0x00000018)
#define LA_SAMPLING_FREQ_RST 0x0
#define LA_LA_VERSION_RST 0x40000
__INLINE void la_version_unpack(struct siwifi_hw * siwifi_hw, uint8_t* samplingfreq, uint32_t* laversion)
{
    uint32_t localVal = REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
    *samplingfreq = (localVal & ((uint32_t)0xFF000000)) >> 24;
    *laversion = (localVal & ((uint32_t)0x00FFFFFF)) >> 0;
}
__INLINE uint8_t la_sampling_freq_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0xFF000000)) >> 24);
}
__INLINE uint32_t la_la_version_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00FFFFFF)) >> 0);
}
#define LA_CNTRL_ADDR(band) (REG_LA_BASE_ADDR(band)+0x000C)
#define LA_CNTRL_OFFSET 0x0000000C
#define LA_CNTRL_INDEX 0x00000003
#define LA_CNTRL_RESET 0x000000F0
__INLINE uint32_t la_cntrl_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_cntrl_set(struct siwifi_hw * siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_CLK_SEL_BIT ((uint32_t)0x00010000)
#define LA_CLK_SEL_POS 16
#define LA_CONT_MODE_BIT ((uint32_t)0x00008000)
#define LA_CONT_MODE_POS 15
#define LA_TRIGGER_INT_EN_BIT ((uint32_t)0x00004000)
#define LA_TRIGGER_INT_EN_POS 14
#define LA_TRIGGER_INT_CLEAR_BIT ((uint32_t)0x00002000)
#define LA_TRIGGER_INT_CLEAR_POS 13
#define LA_TRIGGER_INT_SET_BIT ((uint32_t)0x00001000)
#define LA_TRIGGER_INT_SET_POS 12
#define LA_EXT_TRIGGER_EN_MASK ((uint32_t)0x000000F0)
#define LA_EXT_TRIGGER_EN_LSB 4
#define LA_EXT_TRIGGER_EN_WIDTH ((uint32_t)0x00000004)
#define LA_SW_TRIGGER_BIT ((uint32_t)0x00000008)
#define LA_SW_TRIGGER_POS 3
#define LA_RESET_BIT ((uint32_t)0x00000004)
#define LA_RESET_POS 2
#define LA_STOP_BIT ((uint32_t)0x00000002)
#define LA_STOP_POS 1
#define LA_START_BIT ((uint32_t)0x00000001)
#define LA_START_POS 0
#define LA_TRIGGER_INT_EN_RST 0x0
#define LA_TRIGGER_INT_CLEAR_RST 0x0
#define LA_TRIGGER_INT_SET_RST 0x0
#define LA_EXT_TRIGGER_EN_RST 0xF
#define LA_SW_TRIGGER_RST 0x0
#define LA_RESET_RST 0x0
#define LA_STOP_RST 0x0
#define LA_START_RST 0x0
__INLINE void la_cntrl_pack(struct siwifi_hw *siwifi_hw, uint8_t triggerinten, uint8_t triggerintclear, uint8_t triggerintset, uint8_t exttriggeren, uint8_t swtrigger, uint8_t reset, uint8_t stop, uint8_t start)
{
    ASSERT_ERR((((uint32_t)triggerinten << 14) & ~((uint32_t)0x00004000)) == 0);
    ASSERT_ERR((((uint32_t)triggerintclear << 13) & ~((uint32_t)0x00002000)) == 0);
    ASSERT_ERR((((uint32_t)triggerintset << 12) & ~((uint32_t)0x00001000)) == 0);
    ASSERT_ERR((((uint32_t)exttriggeren << 4) & ~((uint32_t)0x000000F0)) == 0);
    ASSERT_ERR((((uint32_t)swtrigger << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)reset << 2) & ~((uint32_t)0x00000004)) == 0);
    ASSERT_ERR((((uint32_t)stop << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)start << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), ((uint32_t)triggerinten << 14) | ((uint32_t)triggerintclear << 13) | ((uint32_t)triggerintset << 12) | ((uint32_t)exttriggeren << 4) | ((uint32_t)swtrigger << 3) | ((uint32_t)reset << 2) | ((uint32_t)stop << 1) | ((uint32_t)start << 0));
}
__INLINE void la_cntrl_unpack(struct siwifi_hw *siwifi_hw, uint8_t* triggerinten, uint8_t* triggerintclear, uint8_t* triggerintset, uint8_t* exttriggeren, uint8_t* swtrigger, uint8_t* reset, uint8_t* stop, uint8_t* start)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    *triggerinten = (localVal & ((uint32_t)0x00004000)) >> 14;
    *triggerintclear = (localVal & ((uint32_t)0x00002000)) >> 13;
    *triggerintset = (localVal & ((uint32_t)0x00001000)) >> 12;
    *exttriggeren = (localVal & ((uint32_t)0x000000F0)) >> 4;
    *swtrigger = (localVal & ((uint32_t)0x00000008)) >> 3;
    *reset = (localVal & ((uint32_t)0x00000004)) >> 2;
    *stop = (localVal & ((uint32_t)0x00000002)) >> 1;
    *start = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint8_t la_trigger_int_en_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00004000)) >> 14);
}
__INLINE void la_trigger_int_en_setf(struct siwifi_hw *siwifi_hw, uint8_t triggerinten)
{
    ASSERT_ERR((((uint32_t)triggerinten << 14) & ~((uint32_t)0x00004000)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00004000)) | ((uint32_t)triggerinten << 14));
}
__INLINE uint8_t la_trigger_int_clear_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00002000)) >> 13);
}
__INLINE void la_trigger_int_clear_setf(struct siwifi_hw *siwifi_hw, uint8_t triggerintclear)
{
    ASSERT_ERR((((uint32_t)triggerintclear << 13) & ~((uint32_t)0x00002000)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00002000)) | ((uint32_t)triggerintclear << 13));
}
__INLINE uint8_t la_trigger_int_set_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00001000)) >> 12);
}
__INLINE void la_trigger_int_set_setf(struct siwifi_hw *siwifi_hw, uint8_t triggerintset)
{
    ASSERT_ERR((((uint32_t)triggerintset << 12) & ~((uint32_t)0x00001000)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00001000)) | ((uint32_t)triggerintset << 12));
}
__INLINE uint8_t la_ext_trigger_en_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x000000F0)) >> 4);
}
__INLINE void la_ext_trigger_en_setf(struct siwifi_hw *siwifi_hw, uint8_t exttriggeren)
{
    ASSERT_ERR((((uint32_t)exttriggeren << 4) & ~((uint32_t)0x000000F0)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x000000F0)) | ((uint32_t)exttriggeren << 4));
}
__INLINE uint8_t la_sw_trigger_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE void la_sw_trigger_setf(struct siwifi_hw *siwifi_hw, uint8_t swtrigger)
{
    ASSERT_ERR((((uint32_t)swtrigger << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000008)) | ((uint32_t)swtrigger << 3));
}
__INLINE uint8_t la_reset_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}
__INLINE void la_reset_setf(struct siwifi_hw *siwifi_hw, uint8_t reset)
{
    ASSERT_ERR((((uint32_t)reset << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000004)) | ((uint32_t)reset << 2));
}
__INLINE uint8_t la_stop_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE void la_stop_setf(struct siwifi_hw *siwifi_hw, uint8_t stop)
{
    ASSERT_ERR((((uint32_t)stop << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000002)) | ((uint32_t)stop << 1));
}
__INLINE uint8_t la_start_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
__INLINE void la_start_setf(struct siwifi_hw *siwifi_hw, uint8_t start)
{
    ASSERT_ERR((((uint32_t)start << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000001)) | ((uint32_t)start << 0));
}
#define LA_STATUS_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0010)
#define LA_STATUS_OFFSET 0x00000010
#define LA_STATUS_INDEX 0x00000004
#define LA_STATUS_RESET 0x00000000
__INLINE uint32_t la_status_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
}
#define LA_WRITEADDR_MASK ((uint32_t)0x00FFFF00)
#define LA_WRITEADDR_LSB 8
#define LA_WRITEADDR_WIDTH ((uint32_t)0x00000010)
#define LA_EXT_TRIGGER_STATUS_MASK ((uint32_t)0x000000F0)
#define LA_EXT_TRIGGER_STATUS_LSB 4
#define LA_EXT_TRIGGER_STATUS_WIDTH ((uint32_t)0x00000004)
#define LA_SW_TRIGGER_STATUS_BIT ((uint32_t)0x00000008)
#define LA_SW_TRIGGER_STATUS_POS 3
#define LA_INTERNAL_TRIGGER_STATUS_BIT ((uint32_t)0x00000004)
#define LA_INTERNAL_TRIGGER_STATUS_POS 2
#define LA_TRIGGERED_BIT ((uint32_t)0x00000002)
#define LA_TRIGGERED_POS 1
#define LA_STARTED_BIT ((uint32_t)0x00000001)
#define LA_STARTED_POS 0
#define LA_WRITEADDR_RST 0x0
#define LA_EXT_TRIGGER_STATUS_RST 0x0
#define LA_SW_TRIGGER_STATUS_RST 0x0
#define LA_INTERNAL_TRIGGER_STATUS_RST 0x0
#define LA_TRIGGERED_RST 0x0
#define LA_STARTED_RST 0x0
__INLINE void la_status_unpack(struct siwifi_hw *siwifi_hw, uint16_t* writeaddr, uint8_t* exttriggerstatus, uint8_t* swtriggerstatus, uint8_t* internaltriggerstatus, uint8_t* triggered, uint8_t* started)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    *writeaddr = (localVal & ((uint32_t)0x00FFFF00)) >> 8;
    *exttriggerstatus = (localVal & ((uint32_t)0x000000F0)) >> 4;
    *swtriggerstatus = (localVal & ((uint32_t)0x00000008)) >> 3;
    *internaltriggerstatus = (localVal & ((uint32_t)0x00000004)) >> 2;
    *triggered = (localVal & ((uint32_t)0x00000002)) >> 1;
    *started = (localVal & ((uint32_t)0x00000001)) >> 0;
}
__INLINE uint16_t la_writeaddr_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00FFFF00)) >> 8);
}
__INLINE uint8_t la_ext_trigger_status_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x000000F0)) >> 4);
}
__INLINE uint8_t la_sw_trigger_status_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}
__INLINE uint8_t la_internal_trigger_status_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}
__INLINE uint8_t la_triggered_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}
__INLINE uint8_t la_started_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}
#define LA_SAMPLING_MASK_LOW_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0014)
#define LA_SAMPLING_MASK_LOW_OFFSET 0x00000014
#define LA_SAMPLING_MASK_LOW_INDEX 0x00000005
#define LA_SAMPLING_MASK_LOW_RESET 0x00000000
__INLINE uint32_t la_sampling_mask_low_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_sampling_mask_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_SAMPLING_MASK_LOW_MASK ((uint32_t)0xFFFFFFFF)
#define LA_SAMPLING_MASK_LOW_LSB 0
#define LA_SAMPLING_MASK_LOW_WIDTH ((uint32_t)0x00000020)
#define LA_SAMPLING_MASK_LOW_RST 0x0
__INLINE uint32_t la_sampling_mask_low_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_sampling_mask_low_setf(struct siwifi_hw *siwifi_hw, uint32_t samplingmasklow)
{
    ASSERT_ERR((((uint32_t)samplingmasklow << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)samplingmasklow << 0);
}
#define LA_SAMPLING_MASK_MED_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0018)
#define LA_SAMPLING_MASK_MED_OFFSET 0x00000018
#define LA_SAMPLING_MASK_MED_INDEX 0x00000006
#define LA_SAMPLING_MASK_MED_RESET 0x00000000
__INLINE uint32_t la_sampling_mask_med_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_sampling_mask_med_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_SAMPLING_MASK_MED_MASK ((uint32_t)0xFFFFFFFF)
#define LA_SAMPLING_MASK_MED_LSB 0
#define LA_SAMPLING_MASK_MED_WIDTH ((uint32_t)0x00000020)
#define LA_SAMPLING_MASK_MED_RST 0x0
__INLINE uint32_t la_sampling_mask_med_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_sampling_mask_med_setf(struct siwifi_hw *siwifi_hw, uint32_t samplingmaskmed)
{
    ASSERT_ERR((((uint32_t)samplingmaskmed << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)samplingmaskmed << 0);
}
#define LA_SAMPLING_MASK_HIGH_ADDR(band) (REG_LA_BASE_ADDR(band)+0x001C)
#define LA_SAMPLING_MASK_HIGH_OFFSET 0x0000001C
#define LA_SAMPLING_MASK_HIGH_INDEX 0x00000007
#define LA_SAMPLING_MASK_HIGH_RESET 0x00000000
__INLINE uint32_t la_sampling_mask_high_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_sampling_mask_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_SAMPLING_MASK_HIGH_MASK ((uint32_t)0xFFFFFFFF)
#define LA_SAMPLING_MASK_HIGH_LSB 0
#define LA_SAMPLING_MASK_HIGH_WIDTH ((uint32_t)0x00000020)
#define LA_SAMPLING_MASK_HIGH_RST 0x0
__INLINE uint32_t la_sampling_mask_high_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_sampling_mask_high_setf(struct siwifi_hw *siwifi_hw, uint32_t samplingmaskhigh)
{
    ASSERT_ERR((((uint32_t)samplingmaskhigh << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)samplingmaskhigh << 0);
}
#define LA_TRIGGER_MASK_LOW_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0020)
#define LA_TRIGGER_MASK_LOW_OFFSET 0x00000020
#define LA_TRIGGER_MASK_LOW_INDEX 0x00000008
#define LA_TRIGGER_MASK_LOW_RESET 0x00000000
__INLINE uint32_t la_trigger_mask_low_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_mask_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_MASK_LOW_MASK ((uint32_t)0xFFFFFFFF)
#define LA_TRIGGER_MASK_LOW_LSB 0
#define LA_TRIGGER_MASK_LOW_WIDTH ((uint32_t)0x00000020)
#define LA_TRIGGER_MASK_LOW_RST 0x0
__INLINE uint32_t la_trigger_mask_low_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_mask_low_setf(struct siwifi_hw *siwifi_hw, uint32_t triggermasklow)
{
    ASSERT_ERR((((uint32_t)triggermasklow << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggermasklow << 0);
}
#define LA_TRIGGER_MASK_MED_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0024)
#define LA_TRIGGER_MASK_MED_OFFSET 0x00000024
#define LA_TRIGGER_MASK_MED_INDEX 0x00000009
#define LA_TRIGGER_MASK_MED_RESET 0x00000000
__INLINE uint32_t la_trigger_mask_med_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_mask_med_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_MASK_MED_MASK ((uint32_t)0xFFFFFFFF)
#define LA_TRIGGER_MASK_MED_LSB 0
#define LA_TRIGGER_MASK_MED_WIDTH ((uint32_t)0x00000020)
#define LA_TRIGGER_MASK_MED_RST 0x0
__INLINE uint32_t la_trigger_mask_med_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_mask_med_setf(struct siwifi_hw *siwifi_hw, uint32_t triggermaskmed)
{
    ASSERT_ERR((((uint32_t)triggermaskmed << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggermaskmed << 0);
}
#define LA_TRIGGER_MASK_HIGH_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0028)
#define LA_TRIGGER_MASK_HIGH_OFFSET 0x00000028
#define LA_TRIGGER_MASK_HIGH_INDEX 0x0000000A
#define LA_TRIGGER_MASK_HIGH_RESET 0x00000000
__INLINE uint32_t la_trigger_mask_high_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_mask_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_MASK_HIGH_MASK ((uint32_t)0xFFFFFFFF)
#define LA_TRIGGER_MASK_HIGH_LSB 0
#define LA_TRIGGER_MASK_HIGH_WIDTH ((uint32_t)0x00000020)
#define LA_TRIGGER_MASK_HIGH_RST 0x0
__INLINE uint32_t la_trigger_mask_high_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_mask_high_setf(struct siwifi_hw *siwifi_hw, uint32_t triggermaskhigh)
{
    ASSERT_ERR((((uint32_t)triggermaskhigh << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggermaskhigh << 0);
}
#define LA_TRIGGER_VALUE_LOW_ADDR(band) (REG_LA_BASE_ADDR(band)+0x002C)
#define LA_TRIGGER_VALUE_LOW_OFFSET 0x0000002C
#define LA_TRIGGER_VALUE_LOW_INDEX 0x0000000B
#define LA_TRIGGER_VALUE_LOW_RESET 0x00000000
__INLINE uint32_t la_trigger_value_low_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_value_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_VALUE_LOW_MASK ((uint32_t)0xFFFFFFFF)
#define LA_TRIGGER_VALUE_LOW_LSB 0
#define LA_TRIGGER_VALUE_LOW_WIDTH ((uint32_t)0x00000020)
#define LA_TRIGGER_VALUE_LOW_RST 0x0
__INLINE uint32_t la_trigger_value_low_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_value_low_setf(struct siwifi_hw *siwifi_hw, uint32_t triggervaluelow)
{
    ASSERT_ERR((((uint32_t)triggervaluelow << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggervaluelow << 0);
}
#define LA_TRIGGER_VALUE_MED_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0030)
#define LA_TRIGGER_VALUE_MED_OFFSET 0x00000030
#define LA_TRIGGER_VALUE_MED_INDEX 0x0000000C
#define LA_TRIGGER_VALUE_MED_RESET 0x00000000
__INLINE uint32_t la_trigger_value_med_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_value_med_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_VALUE_MED_MASK ((uint32_t)0xFFFFFFFF)
#define LA_TRIGGER_VALUE_MED_LSB 0
#define LA_TRIGGER_VALUE_MED_WIDTH ((uint32_t)0x00000020)
#define LA_TRIGGER_VALUE_MED_RST 0x0
__INLINE uint32_t la_trigger_value_med_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_value_med_setf(struct siwifi_hw *siwifi_hw, uint32_t triggervaluemed)
{
    ASSERT_ERR((((uint32_t)triggervaluemed << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggervaluemed << 0);
}
#define LA_TRIGGER_VALUE_HIGH_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0034)
#define LA_TRIGGER_VALUE_HIGH_OFFSET 0x00000034
#define LA_TRIGGER_VALUE_HIGH_INDEX 0x0000000D
#define LA_TRIGGER_VALUE_HIGH_RESET 0x00000000
__INLINE uint32_t la_trigger_value_high_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_value_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_VALUE_HIGH_MASK ((uint32_t)0xFFFFFFFF)
#define LA_TRIGGER_VALUE_HIGH_LSB 0
#define LA_TRIGGER_VALUE_HIGH_WIDTH ((uint32_t)0x00000020)
#define LA_TRIGGER_VALUE_HIGH_RST 0x0
__INLINE uint32_t la_trigger_value_high_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_value_high_setf(struct siwifi_hw *siwifi_hw, uint32_t triggervaluehigh)
{
    ASSERT_ERR((((uint32_t)triggervaluehigh << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggervaluehigh << 0);
}
#define LA_TRIGGER_POINT_ADDR(band) (REG_LA_BASE_ADDR(band)+0x0038)
#define LA_TRIGGER_POINT_OFFSET 0x00000038
#define LA_TRIGGER_POINT_INDEX 0x0000000E
#define LA_TRIGGER_POINT_RESET 0x00000000
__INLINE uint32_t la_trigger_point_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb));
}
__INLINE void la_trigger_point_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#define LA_TRIGGER_POINT_MASK ((uint32_t)0x0000FFFF)
#define LA_TRIGGER_POINT_LSB 0
#define LA_TRIGGER_POINT_WIDTH ((uint32_t)0x00000010)
#define LA_TRIGGER_POINT_RST 0x0
__INLINE uint16_t la_trigger_point_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}
__INLINE void la_trigger_point_setf(struct siwifi_hw *siwifi_hw, uint16_t triggerpoint)
{
    ASSERT_ERR((((uint32_t)triggerpoint << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggerpoint << 0);
}
#define LA_FIRSTSAMPLE_ADDR(band) (REG_LA_BASE_ADDR(band)+0x003C)
#define LA_FIRSTSAMPLE_OFFSET 0x0000003C
#define LA_FIRSTSAMPLE_INDEX 0x0000000F
#define LA_FIRSTSAMPLE_RESET 0x00000000
__INLINE uint32_t la_firstsample_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_FIRSTSAMPLE_ADDR(siwifi_hw->mod_params->is_hb));
}
#define LA_FIRSTSAMPLE_MASK ((uint32_t)0x0000FFFF)
#define LA_FIRSTSAMPLE_LSB 0
#define LA_FIRSTSAMPLE_WIDTH ((uint32_t)0x00000010)
#define LA_FIRSTSAMPLE_RST 0x0
__INLINE uint16_t la_firstsample_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_FIRSTSAMPLE_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}
#endif
