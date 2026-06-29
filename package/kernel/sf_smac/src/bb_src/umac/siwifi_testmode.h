#ifndef SIWIFI_TESTMODE_H_
#define SIWIFI_TESTMODE_H_ 
#include <net/mac80211.h>
#include <net/netlink.h>
enum siwifi_tm_cmd_t {
    SIWIFI_TM_CMD_APP2DEV_REG_READ = 1,
    SIWIFI_TM_CMD_APP2DEV_REG_WRITE,
    SIWIFI_TM_CMD_APP2DEV_SET_DBGMODFILTER,
    SIWIFI_TM_CMD_APP2DEV_SET_DBGSEVFILTER,
    SIWIFI_TM_CMD_APP2DEV_REG_READ_DBG,
    SIWIFI_TM_CMD_APP2DEV_REG_WRITE_DBG,
    SIWIFI_TM_CMD_MAX,
};
enum siwifi_tm_attr_t {
    SIWIFI_TM_ATTR_NOT_APPLICABLE = 0,
    SIWIFI_TM_ATTR_COMMAND,
    SIWIFI_TM_ATTR_REG_OFFSET,
    SIWIFI_TM_ATTR_REG_VALUE32,
    SIWIFI_TM_ATTR_REG_FILTER,
    SIWIFI_TM_ATTR_MAX,
};
int siwifi_testmode_reg(struct ieee80211_hw *hw, struct nlattr **tb);
int siwifi_testmode_dbg_filter(struct ieee80211_hw *hw, struct nlattr **tb);
int siwifi_testmode_reg_dbg(struct ieee80211_hw *hw, struct nlattr **tb);
#endif
