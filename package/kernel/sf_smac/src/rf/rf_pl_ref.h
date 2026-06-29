#ifndef _RF_PL_REF_H_
#define _RF_PL_REF_H_ 
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#define REG_SYSM_CATRF_RESET (volatile void *)(0xB9E00000 + 0x2C98)
#define REG_SYSM_CATRF_SOFT_RESET (volatile void *)(0xB9E00000 + 0xD400)
#define REG_SYSM_CATRF_SOFT_CLKEN (volatile void *)(0xB9E00000 + 0xD404)
#define REG_SYSM_CATRF_SOFT_BOE (volatile void *)(0xB9E00000 + 0xD40C)
#define REG_SYSM_LPCLK_GATING (volatile void *)(0xB9E30000 + 0xFFCC)
#define DEFAULT_LPCLK_GPIONUM 24
enum {
    RF_EVENT_NEED_RESTART = (1 << 0),
    RF_EVENT_STOP_BUSINESS = (1 << 1),
    RF_EVENT_START_BUSINESS = (1 << 2),
    RF_EVENT_TRX_RECALI = (1 << 3),
    RF_EVENT_DUMP_CMD = (1 << 4),
    RF_EVENT_GET_TEMP = (1 << 5),
    RF_EVENT_COOLING_TEMP = (1 << 6),
    RF_EVENT_TEMP_CTL = (1 << 7),
};
enum {
    RF_FLAGS_NONBLOCK = (1 << 0),
};
enum{
    RF_RET_FAILED = -1,
    RF_RET_OK = 0,
    RF_RET_AGAIN,
};
typedef int (*baseband_callback)(void *data, uint32_t event, uint32_t flags, void *parameters);
#define CLIENT_LB 1
#define CLIENT_HB 2
#define SUPPORT_MAX_CLIENT_NUM 2
struct rf_ex_pa_config {
    uint8_t lb1_idle_cfg;
    uint8_t lb1_rx_cfg;
    uint8_t lb1_tx_cfg;
    uint8_t lb1_pa_cfg;
    uint8_t lb2_idle_cfg;
    uint8_t lb2_rx_cfg;
    uint8_t lb2_tx_cfg;
    uint8_t lb2_pa_cfg;
    uint8_t hb1_idle_cfg;
    uint8_t hb1_rx_cfg;
    uint8_t hb1_tx_cfg;
    uint8_t hb1_pa_cfg;
    uint8_t hb2_idle_cfg;
    uint8_t hb2_rx_cfg;
    uint8_t hb2_tx_cfg;
    uint8_t hb2_pa_cfg;
    uint8_t hb_rx_step2_value;
    uint8_t hb_rx_step2_delay;
    uint8_t hb_tx_step2_delay;
};
enum sfax8_rf_cooling_state {
    RF_COOLING_STATE_NORMAL,
    RF_COOLING_STATE_LV1,
    RF_COOLING_STATE_LV2,
    RF_COOLING_STATE_LV3,
 RF_COOLING_STATE_LV4,
 RF_COOLING_STATE_LV5,
 RF_COOLING_STATE_LV6,
 RF_COOLING_STATE_LV7,
 RF_COOLING_STATE_LV8,
 RF_COOLING_STATE_LV9,
 RF_COOLING_STATE_LV10,
 RF_COOLING_STATE_LV11,
 RF_COOLING_STATE_LV12,
};
struct rf_cooling_temp_set {
    uint8_t max_index[SUPPORT_MAX_CLIENT_NUM];
 bool flag;
    int thermal_on;
    int trig_temp;
    unsigned long cool_state;
    uint32_t hb_low_power_state;
    int change_power_trend[3];
    int change_power[3];
    int cur_temp;
    int dual_cali_enable;
};
struct sfax8_rf_thermal_state {
    unsigned long state;
    int temp;
    int temp_last;
    int relative_diff;
    unsigned int count;
};
struct rf_get_temp{
    uint16_t value;
};
struct rf_cmd_dump_set {
    uint32_t address;
    uint32_t size;
    uint32_t count;
    uint8_t flag;
};
struct rf_cmd_dump_val
{
    uint16_t cmdid;
    uint16_t argnum;
    uint16_t arg0;
    uint16_t arg1;
    uint16_t arg2;
    uint16_t arg3;
};
enum sf_rf_feature {
    RF_FEATURE_SUPPORT_ENHANCE_LOWRATE_POWER = 1 << 0,
    RF_FEATURE_SUPPORT_EXTERNAL_PA_HB = 1 << 1,
    RF_FEATURE_SUPPORT_EXTERNAL_PA_LB = 1 << 2,
 RF_FEATURE_DUAL_ANTENNA_CALIBRATE = 1 << 3,
};
struct rf_client_info {
    int8_t band_type;
    baseband_callback cb;
    void *cb_data;
};
struct rf_pl_context{
    int8_t fw_loaded;
    int8_t booted;
    int8_t lb_working;
    int8_t hb_working;
    int8_t calibrating;
    int rf_hw_version;
    uint32_t irq_num;
    struct mutex bb_mtx;
    spinlock_t cali_lock;
    struct delayed_work cali_work;
    struct rf_client_info clients[SUPPORT_MAX_CLIENT_NUM];
    struct rf_cmd_dump_set rf_cmd_sets[SUPPORT_MAX_CLIENT_NUM];
    struct dentry *debugfs;
    uint16_t dbg_reg_addr;
    uint32_t dbg_recalibrate;
    void __iomem *base;
    int irq;
    unsigned long irq_source;
    uint32_t hk_irq_source_num[16];
    uint32_t cmd_err_irq_source_num[16];
    long long cali_work_num;
    uint8_t enable_recali;
    uint16_t xo_config;
    uint8_t trx_path_cfg;
    bool lb_ex_pa_exist;
    bool hb_ex_pa_exist;
    struct rf_ex_pa_config ex_pa_config;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
 unsigned int dual_antenna_calibrate;
#endif
    struct sfax8_rf_thermal_state rf_thermal_state;
    struct rf_cooling_temp_set cooling_temp_set;
    struct timer_list lp_clk_timer;
    bool lp_clk_timer_enable;
    uint8_t lp_clk_edge;
    uint8_t hk_time_source;
    int32_t hk_temp_diff;
    bool update_tx_gain_table;
    struct device *dev;
    int num_clks;
    struct clk_bulk_data *clks;
    struct reset_control *rstc;
    uint32_t rf_sw_version;
    uint32_t rf_sw_version1;
    uint32_t cali_table_version;
};
#endif
