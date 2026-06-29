#include "rf_regbank_defines.h"
#ifndef _RF_ACCESS_H_
#define _RF_ACCESS_H_ 
#define LOW_BAND 1
#define HIGH_BAND 2
#define NO_BAND_RUN -2
#define TEMP_ERROR_FROM_LMAC -1
#define TEMP_ABNORMAL -50
#define TEMP_DEFAULT 25
#define HK_EXPIRE_INTERVAL 1000
#define HK_TEMP_DIFF 55
void rf_set_access_base(void *base);
int rf_bootup(void);
int rf_app_calibrate(bool reactive);
void ml_fw_load(uint16_t *data, uint32_t size, uint8_t run);
void ml_apb_write(uint16_t addr, uint16_t data);
uint16_t ml_apb_read(uint16_t addr);
uint16_t ml_apb_rsbus_read(uint16_t addr);
int8_t init_aetnensis_by_script(uint8_t band, uint8_t trx);
void aet_app_check_status(void);
int rf_get_temperature(bool from_lmac);
int32_t rf_trx_status_change(int txpower_idx,uint32_t txrxid,bool txrx);
int8_t rf_switch_fem_ctrl(uint8_t fem_ctl);
int rf_trx_stop(int trxid);
int rf_trx_set_rfappconfig(
               uint8_t conf_in_idel_hb1, uint8_t conf_in_idel_hb2,uint8_t conf_in_rx_hb1, uint8_t conf_in_rx_hb2,
               uint8_t conf_in_tx_hb1, uint8_t conf_in_tx_hb2, uint8_t conf_in_pa_hb1, uint8_t conf_in_pa_hb2,
               uint8_t conf_in_idel_lb1, uint8_t conf_in_idel_lb2,uint8_t conf_in_rx_lb1, uint8_t conf_in_rx_lb2,
               uint8_t conf_in_tx_lb1, uint8_t conf_in_tx_lb2, uint8_t conf_in_pa_lb1, uint8_t conf_in_pa_lb2
               );
#ifdef UPDATE_GAIN_TABLE
#define SF19A28_RF_GAIN_NUM 32
#define SF19A28_RF_DBB_GAIN_NUM 16
extern uint16_t rf_lb_dbb_gain_table[SF19A28_RF_DBB_GAIN_NUM];
extern uint16_t rf_lb_dbb_gain_table_def[SF19A28_RF_DBB_GAIN_NUM];
extern uint16_t rf_lb_gain_table[SF19A28_RF_GAIN_NUM];
extern uint16_t rf_lb_gain_table_def[SF19A28_RF_GAIN_NUM];
void rf_update_gain_table(uint16_t table_base_addr, uint16_t *table_addr,int gain_num);
void rf_update_hb_tx_gain_table(void);
void rf_reset_hb_tx_gain_table(void);
#endif
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
#ifdef CFG_DUAL_ANTENNA_CALIBRATE_CSD
#define RF_HB_BASE_GAIN 3
#else
#define RF_HB_BASE_GAIN 10
#endif
#define RF_LB_BASE_GAIN 16
#define RF_HB_BASE_GAIN_DIFF 3
#endif
int32_t rf_get_trx_status(bool is_hb, uint16_t *trx_state, uint16_t *freq, uint16_t *chbw);
int16_t rf_set_property(uint8_t property_id, uint16_t value);
uint16_t rf_get_property(uint8_t property_id);
int8_t ml_apb_send_0_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd);
int8_t ml_apb_send_1_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args);
int8_t ml_apb_send_2_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args0, uint16_t args1);
int8_t ml_apb_send_3_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args0, uint16_t args1, uint16_t args2);
int8_t ml_apb_send_4_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args0, uint16_t args1, uint16_t args2, uint16_t args3);
#endif
