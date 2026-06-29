#ifndef _REG_MDM_RIU_H_
#define _REG_MDM_RIU_H_ 
#include "reg_access.h"
#define MDM_CLKGATEFCTRL0_ADDR(band) (REG_MDM_CFG_BASE_ADDR(band)+0x0874)
__INLINE uint8_t mdm_agcclkforce_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x10000000)) >> 28);
}
__INLINE void mdm_agcclkforce_setf(struct siwifi_hw *siwifi_hw, uint8_t agcclkforce)
{
    ASSERT_ERR((((uint32_t)agcclkforce << 28) & ~((uint32_t)0x10000000)) == 0);
    REG_PL_WR(MDM_CLKGATEFCTRL0_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(MDM_CLKGATEFCTRL0_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x10000000)) | ((uint32_t)agcclkforce << 28));
}
#define RIU_SIWIFIAGCCNTL_ADDR(band) (REG_RIU_BASE_ADDR(band)+0xB390)
#define REG_RIU_SIWIFIDYNAMICCONFIG_OFFSET 0xB008
#define BIT_RIU_SIWIFIDYNAMICCONFIG_AGCMEMBISTSTART 28
#define REG_RIU_AGCMEMBISTSTAT_OFFSET 0xB238
__INLINE uint8_t riu_agcfsmreset_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(RIU_SIWIFIAGCCNTL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00001000)) >> 12);
}
__INLINE void riu_agcfsmreset_setf(struct siwifi_hw *siwifi_hw, uint8_t agcfsmreset)
{
    ASSERT_ERR((((uint32_t)agcfsmreset << 12) & ~((uint32_t)0x00001000)) == 0);
    REG_PL_WR(RIU_SIWIFIAGCCNTL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(RIU_SIWIFIAGCCNTL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00001000)) | ((uint32_t)agcfsmreset << 12));
}
#endif
