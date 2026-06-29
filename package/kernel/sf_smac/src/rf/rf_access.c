#include <linux/types.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "rf_regbank_defines.h"
#include "rf_access.h"
#include "rf_pl_ref.h"
#define MAX_RETRY_TIME 100
#define SS_BLANK 0
#define SS_STANDBY 1
#define SS_OPERATING 2
#define SS_SLEEP 3
#define TS_UNINITIALIZED 0
#define TS_PARKED 1
#define TS_PREPARED 2
#define TS_TRANSMITTING 3
#define TS_RECEIVING 4
#define CHECK_VERSION(hb,priv) \
    { \
        int8_t resp_number; \
        uint16_t value; \
        uint16_t cmd_base = ML_CMD_IF_p1_BASE; \
        resp_number = ml_apb_send_0_params_cmd(1, cmd_base, _GEN_GetVersion); \
        WARN_ON(4 != resp_number); \
        value = ml_apb_read(cmd_base + 0x000C); \
        printk("rf hw version : 0x%x\n", value); \
        priv->rf_hw_version = value; \
        value = ml_apb_read(cmd_base + 0x000D); \
        priv->rf_sw_version = value; \
        printk("rf sw version : 0x%x\n", value); \
        value = ml_apb_read(cmd_base + 0x000E); \
        priv->rf_sw_version1 = value; \
        printk("value : 0x%x\n", value); \
    }
static void __iomem *g_rf_access_base;
extern struct rf_pl_context *g_rf_pl_ctx;
#define MAX_TRX_CALIBRATE_CONF 0x3FF
#define MAX_APP_CALIBRATE_CONF 2
#define REFERENCE_TEMP 50
#define ABNORMAL_BOOT_TEMP 65
#define SF_BOOT_TEMP_FILE_NAME "/etc/config/rf_temp"
enum {
    _GEN_GetVersion = 0x8010,
    _GEN_SetProperty = 0x8042,
    _GEN_GetProperty = 0x8052,
    _APP_GetStatus = 0x8070,
    _APP_Sleep = 0x8080,
    _APP_Activate = 0x8090,
    _APP_Deactivate = 0x80A0,
    _APP_GetTempreture = 0x80C0,
    _APP_Calibrate = 0x80D0,
    _APP_CalResistor = 0x80E0,
    _APP_SetRegistor = 0x80F1,
    _APP_ConfigTrx = 0x8101,
    _APP_ConfigFemCtrl = 0x8368,
    _TRX_Receive = 0x8161,
    _TRX_Transmit = 0x8171,
    _TRX_Stop = 0x81b1,
    _TRX_Park = 0x81c1,
    _TRX_Prepare = 0x81d3,
    _TRX_GetStatus = 0x81e1,
    _TRX_SetTestDc = 0x82c3,
    _CLK_SetXo = 0x8111,
};
enum {
    DTEMP_CAL_HB_RXDBBDC = 5,
    DTEMP_CAL_HB_TXDBBDC,
    DTEMP_CAL_HB_RXABBDC,
    DTEMP_CAL_HB_RXIQC,
    DTEMP_CAL_HB_TXIQC,
    DTEMP_CAL_LB_RXDBBDC,
    DTEMP_CAL_LB_TXDBBDC,
    DTEMP_CAL_LB_RXABBDC,
    DTEMP_CAL_LB_RXIQC,
    DTEMP_CAL_LB_TXIQC,
    HK_INTERVAL = 27,
    HK_DISABLE = 42,
    LB_BASE_GAIN = 45,
    HB_BASE_GAIN = 44,
    HB1_PA_ENABLE=46,
    HB2_PA_ENABLE=47,
    NEW_TXCMD_API_VER=48,
    HB_RX_STEP2_DELAY=49,
    HB_TX_STEP2_DELAY=50,
    HB_RX_STEP2_VALUE=51,
    LB_ADC_CLK=52,
};
enum {
    _TRXID_HB = 0,
    _TRXID_LB = 1
};
struct aet_hk_data{
    uint8_t hk_mode_enable;
    uint8_t hk_clk_src;
    uint16_t hk_task_config;
    uint16_t hk_update_time;
    uint16_t hk_tempdiff_thres;
};
void rf_set_access_base(void *base)
{
    g_rf_access_base = (void __iomem *)base;
    printk("rf access base address : %p\n", g_rf_access_base);
}
void ml_apb_write(uint16_t addr, uint16_t data)
{
    if (addr == 0xFFFF) {
        msleep(data);
        return;
    }
    writew(data, (g_rf_access_base + (addr << 1)));
    if (addr < 0x2300 || addr > 0x23BE)
        return;
    udelay(50);
}
uint16_t ml_apb_read(uint16_t addr)
{
    return readw((g_rf_access_base + (addr << 1)));
}
uint16_t ml_apb_rsbus_read(uint16_t addr)
{
    uint16_t value,i;
    i = 0;
    readw((g_rf_access_base + (addr << 1)));
    while (i++ < 1000) {
        udelay(10);
        value = readw((g_rf_access_base + (RSBUS_STATUS << 1)));
        if (value & 0x4)
            break;
    }
    if (i >= 1000)
        return 0xdead;
    return readw((g_rf_access_base + (RSBUS_READ << 1)));
}
#define APB_RESPONSE_TIMEOUT 100000
static int8_t ml_apb_wait_cmd_resp(uint16_t rrq, uint16_t cmd_base, uint16_t cmd)
{
    uint16_t value;
    int i = 0;
    if (!rrq)
        return 0;
    while(i++ < APB_RESPONSE_TIMEOUT) {
        value = ml_apb_read(cmd_base + 0x000A);
        if (((value >> 14) & 0x1) && ((value >> 4) & 0x3FF))
            break;
        udelay(100);
    }
    if (i >= APB_RESPONSE_TIMEOUT)
        printk("command 0x%x timeout, args0 : 0x%x\n", cmd, value);
    if (value & 0x8000) {
        printk("command 0x%x meet error(0x%x), value=0x%x args1 : 0x%x\n", cmd, (value >> 4) & 0x3FF, value, ml_apb_read(cmd_base + 0x000B));
        printk("DW1=0x%x DW2=0x%x DW3=0x%x DW4=0x%x DW5=0x%x DW6=0x%x DW7=0x%x DW8=0x%x DW9=0x%x\n",
                ml_apb_read(cmd_base + 0x000B), ml_apb_read(cmd_base + 0x000C), ml_apb_read(cmd_base + 0x000D), ml_apb_read(cmd_base + 0x000E), ml_apb_read(cmd_base + 0x000F),
                ml_apb_read(cmd_base + 0x0010), ml_apb_read(cmd_base + 0x0011), ml_apb_read(cmd_base + 0x0012), ml_apb_read(cmd_base + 0x0013)
                );
        return -1;
    }
    if (((value >> 4) & 0x3FF) != ((cmd >> 4) & 0x3FF)) {
        printk("command 0x%x meet invalid response id 0x%x\n", cmd, (value >> 4) & 0x3FF);
        return -2;
    }
    if(cmd != 0x80c0)
        printk("command 0x%x get a repsonse with args : %d\n", cmd, (value & 0xF));
    return (value & 0xF);
}
int8_t ml_apb_send_0_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd)
{
    if(cmd != 0x80c0)
        printk("%s command : 0x%x, cmd_base : 0x%x, rrq : %d\n", __func__, cmd, cmd_base, rrq);
    ml_apb_write(cmd_base, cmd);
    return ml_apb_wait_cmd_resp(rrq, cmd_base, cmd);
}
int8_t ml_apb_send_1_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args)
{
    printk("%s, rrq = %d, cmd_base : 0x%x, cmd = 0x%x, args0 = 0x%x\n", __func__, rrq, cmd_base, cmd, args);
    ml_apb_write(cmd_base, cmd);
    ml_apb_write(cmd_base + 0x0001, args);
    return ml_apb_wait_cmd_resp(rrq, cmd_base, cmd);
}
int8_t ml_apb_send_2_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args0, uint16_t args1)
{
    printk("%s, rrq = %d, cmd_base : 0x%x, cmd = 0x%x, args0 = 0x%x, args1 = 0x%x\n", __func__, rrq, cmd_base, cmd, args0, args1);
    ml_apb_write(cmd_base, cmd);
    ml_apb_write(cmd_base + 0x0001, args0);
    ml_apb_write(cmd_base + 0x0002, args1);
    return ml_apb_wait_cmd_resp(rrq, cmd_base, cmd);
}
int8_t ml_apb_send_3_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args0, uint16_t args1, uint16_t args2)
{
    printk("%s, rrq = %d, cmd_base : 0x%x, cmd = 0x%x, args0 = 0x%x, args1 = 0x%x, args2 = 0x%x\n", \
                __func__, rrq, cmd_base, cmd, args0, args1, args2);
    ml_apb_write(cmd_base, cmd);
    ml_apb_write(cmd_base + 0x0001, args0);
    ml_apb_write(cmd_base + 0x0002, args1);
    ml_apb_write(cmd_base + 0x0003, args2);
    return ml_apb_wait_cmd_resp(rrq, cmd_base, cmd);
}
int8_t ml_apb_send_4_params_cmd(uint16_t rrq, uint16_t cmd_base, uint16_t cmd, uint16_t args0, uint16_t args1, uint16_t args2, uint16_t args3)
{
    printk("%s, rrq = %d, cmd_base : 0x%x, cmd = 0x%x, args0 = 0x%x, args1 = 0x%x, args2 = 0x%x, args3=0x%x\n", \
                __func__, rrq, cmd_base, cmd, args0, args1, args2, args3);
    ml_apb_write(cmd_base, cmd);
    ml_apb_write(cmd_base + 0x0001, args0);
    ml_apb_write(cmd_base + 0x0002, args1);
    ml_apb_write(cmd_base + 0x0003, args2);
    ml_apb_write(cmd_base + 0x0004, args3);
    return ml_apb_wait_cmd_resp(rrq, cmd_base, cmd);
}
int8_t rf_set_xo_value(uint32_t xo_value)
{
    uint32_t resp_args = 0;
    uint8_t value = xo_value & 0xff;
    uint32_t cmd_base = ML_CMD_IF_p1_BASE;
    printk("xo_value_conf :0x%x\n",value);
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _CLK_SetXo,value);
    if(resp_args != 1){
        printk(" _CLK_SetXo response num failed!, resp_args : %d\n", resp_args);
        return -1;
    }
    resp_args = ml_apb_read(cmd_base + 0x000B);
    if(resp_args){
        printk(" _CLK_SetXo, failed!, resp_value : %d\n", value);
        return -2;
    }
   return 0;
}
EXPORT_SYMBOL_GPL(rf_set_xo_value);
uint16_t rf_get_property(uint8_t property_id)
{
    uint16_t tmp;
    int8_t resp_number;
    resp_number = ml_apb_send_2_params_cmd(1, ML_CMD_P1_COMMAND_HEADER, _GEN_GetProperty, property_id, 1);
    if(resp_number != 2){
        printk("%d error\n",__LINE__);
        goto ERROR;
    }
    tmp = ml_apb_read(ML_CMD_P1_COMMAND_HEADER + 0x000B);
    if(tmp != 0){
        printk("%d error\n",__LINE__);
        goto ERROR;
    }
    tmp = ml_apb_read(ML_CMD_P1_COMMAND_HEADER + 0x000C);
    return tmp;
ERROR:
    return 0;
}
int16_t rf_set_property(uint8_t property_id, uint16_t value)
{
    uint16_t tmp;
    int8_t resp_number;
    resp_number = ml_apb_send_2_params_cmd(1, ML_CMD_P1_COMMAND_HEADER, _GEN_SetProperty, property_id, value);
    if(resp_number != 1){
        printk("%d error\n",__LINE__);
        goto ERROR;
    }
    tmp = ml_apb_read(ML_CMD_P1_COMMAND_HEADER + 0x000B);
    if(tmp != 0){
        printk("%d error\n",__LINE__);
        goto ERROR;
    }
    printk("property_id %d property_value %d\n",property_id,value);
    return 0;
ERROR:
    return -1;
}
static int rf_app_prepare(uint8_t trx_path_config)
{
    uint16_t value;
    int8_t resp_args;
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _APP_ConfigTrx, trx_path_config);
    if (resp_args != 1) {
        printk("_APP_ConfigTrx response num failed!, resp_args : %d\n", resp_args);
        return -1;
    }
    value = ml_apb_read(cmd_base + 0x000B);
    if (value) {
        printk("_APP_ConfigTrx, failed!, resp_value : %d\n", value);
        return -2;
    }
    rf_set_xo_value((g_rf_pl_ctx->xo_config & 0xff));
#if 0
#endif
    printk("switch to OPERATING mode\n");
    resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_Activate);
    if (resp_args != 1) {
        printk("_APP_Activate response num error %d",resp_args);
        return -3;
    }
    value = ml_apb_read(ML_CMD_IF_p1_BASE + 0x000B);
    if (value != 0x0) {
        printk("_APP_Activate error 0x%x\n",value);
        return -4;
    }
    msleep(5);
    resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_GetStatus);
    if (resp_args != 2) {
        printk("_APP_GetStatus response num error %d\n",resp_args);
        return -5;
    }
    value = ml_apb_read(cmd_base + 0x000B);
    if (value != 0x0) {
        printk("_APP_GetStatus fail %x!\n",value);
        return -6;
    }
    value = ml_apb_read(cmd_base + 0x000C);
    if (value != SS_OPERATING) {
        printk("mode is not in OPERATING! %x\n",value);
    }
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    if (g_rf_pl_ctx->dual_antenna_calibrate) {
        rf_set_property(NEW_TXCMD_API_VER, 1);
        rf_set_property(LB_BASE_GAIN, RF_LB_BASE_GAIN);
        rf_set_property(HB_BASE_GAIN, RF_HB_BASE_GAIN);
    }
    else {
        rf_set_property(NEW_TXCMD_API_VER, 0);
    }
#endif
    rf_set_property(HK_INTERVAL, 50);
    return 0;
}
static int rf_switch_operating_with_standby(uint16_t next_status)
{
    uint16_t ss_status =0;
    uint16_t resp_args = 0;
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    uint16_t value=0;
    resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_GetStatus);
    if (resp_args != 2) {
        printk("_APP_GetStatus response num error %d\n",resp_args);
        return -1;
    }
    value = ml_apb_read(cmd_base + 0x000B);
    if (value != 0x0) {
        printk("_APP_GetStatus fail %x!\n",value);
        return -1;
    }
    ss_status = ml_apb_read(cmd_base + 0x000C);
    if(ss_status == next_status){
        printk("need to change status!! current status is :%d\n",ss_status);
        return 0;
    }
    else if( next_status == SS_OPERATING){
        printk("switch to OPERATING mode\n");
        resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_Activate);
        if (resp_args != 1) {
            printk("_APP_Activate response num error %d",resp_args);
            return -3;
        }
        value = ml_apb_read(ML_CMD_IF_p1_BASE + 0x000B);
        if (value != 0x0) {
            printk("_APP_Activate error 0x%x\n",value);
            return -4;
        }
    }
    else{
        printk("switch to standby mode\n");
        resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_Deactivate);
        if (resp_args != 1) {
            printk("_APP_Deactivate response num error %d",resp_args);
            return -3;
        }
        value = ml_apb_read(ML_CMD_IF_p1_BASE + 0x000B);
        if (value != 0x0) {
            printk("_APP_Activate error 0x%x\n",value);
            return -4;
        }
    }
    return 0;
}
int rf_trx_stop(int trxid)
{
    uint16_t cmd_base = HB_HL_CMD_P1_COMMAND_HEADER;
    uint16_t resp_args=0;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Stop,trxid);
    if (resp_args != 1) {
        printk("trx stop failed, resp_args : %d\n", resp_args);
        return -4;
    }
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Park,trxid);
    if (resp_args != 1) {
        printk("trx park failed, resp_args : %d\n", resp_args);
        return -4;
    }
    return 0;
}
static int rf_trx_start(int trxid,uint16_t freq,uint16_t bw)
{
    uint16_t cmd_base = HB_HL_CMD_P1_COMMAND_HEADER;
    uint16_t resp_args=0;
    cmd_base = LB_HL_CMD_P1_COMMAND_HEADER;
    resp_args = ml_apb_send_3_params_cmd(1, cmd_base, _TRX_Prepare, trxid, freq,bw);
    if (resp_args != 1) {
        printk("trx prepare failed, resp_args : %d\n", resp_args);
        return -7;
    }
    if(trxid){
        cmd_base = LB_LL_CMD_P1_COMMAND_HEADER;
    }
    else{
        cmd_base = HB_LL_CMD_P1_COMMAND_HEADER;
    }
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Receive, trxid);
    if (resp_args != 1) {
        printk("trx receive failed, resp_args : %d\n", resp_args);
        return -8;
    }
    return 0;
}
int8_t rf_switch_fem_ctrl(uint8_t fem_ctl)
{
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    int8_t resp_args;
    uint16_t freq_hb, chbw_hb;
    uint16_t freq_lb, chbw_lb;
    uint16_t ts_status_hb,ts_status_lb;
#ifdef UPDATE_GAIN_TABLE
 if(g_rf_pl_ctx->dual_antenna_calibrate){
  rf_reset_hb_tx_gain_table();
  rf_update_gain_table(LB_TX_GAIN_TABLE_0, &rf_lb_gain_table_def[0],32);
  rf_update_gain_table(LB_TX_DBB_GAIN_TABLE_0, &rf_lb_dbb_gain_table_def[0],16);
 }
#endif
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, 0);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -1;
    }
    ts_status_hb = ml_apb_read(cmd_base + 0x000C);
    freq_hb = ml_apb_read(cmd_base + 0x000D);
    chbw_hb = ml_apb_read(cmd_base + 0x000E);
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, 1);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -2;
    }
    ts_status_lb = ml_apb_read(cmd_base + 0x000C);
    freq_lb = ml_apb_read(cmd_base + 0x000D);
    chbw_lb = ml_apb_read(cmd_base + 0x000E);
    if((ts_status_hb == TS_PARKED) || (ts_status_lb == TS_PARKED)){
        printk("no need to stop!!!! now trx_status_hb %d trx_status_lb: %d\n",ts_status_hb, ts_status_lb);
    }
    rf_trx_stop(0);
    rf_trx_stop(1);
    rf_switch_operating_with_standby(SS_STANDBY);
    rf_trx_set_rfappconfig(0xf0,0xf0,0xfa,0xfa,0xf6,0xf6,0x0,0x0
                    ,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0);
    rf_switch_operating_with_standby(SS_OPERATING);
    rf_trx_start(0,freq_hb,chbw_hb);
    rf_trx_start(1,freq_lb,chbw_lb);
#ifdef UPDATE_GAIN_TABLE
    if(g_rf_pl_ctx->dual_antenna_calibrate){
  rf_update_hb_tx_gain_table();
  rf_update_gain_table(LB_TX_GAIN_TABLE_0, &rf_lb_gain_table[0],32);
  rf_update_gain_table(LB_TX_DBB_GAIN_TABLE_0, &rf_lb_dbb_gain_table[0],16);
 }
#endif
    return 0;
}EXPORT_SYMBOL_GPL(rf_switch_fem_ctrl);
int8_t rf_switch_aet_trx(int ate_num)
{
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    int8_t resp_args;
    uint16_t freq_hb, chbw_hb;
    uint16_t freq_lb, chbw_lb;
    uint16_t value;
    uint16_t ts_status_hb,ts_status_lb;
#ifdef UPDATE_GAIN_TABLE
 if(g_rf_pl_ctx->dual_antenna_calibrate){
  rf_reset_hb_tx_gain_table();
  rf_update_gain_table(LB_TX_GAIN_TABLE_0, &rf_lb_gain_table_def[0],32);
  rf_update_gain_table(LB_TX_DBB_GAIN_TABLE_0, &rf_lb_dbb_gain_table_def[0],16);
 }
#endif
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, 0);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -1;
    }
    ts_status_hb = ml_apb_read(cmd_base + 0x000C);
    freq_hb = ml_apb_read(cmd_base + 0x000D);
    chbw_hb = ml_apb_read(cmd_base + 0x000E);
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, 1);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -2;
    }
    ts_status_lb = ml_apb_read(cmd_base + 0x000C);
    freq_lb = ml_apb_read(cmd_base + 0x000D);
    chbw_lb = ml_apb_read(cmd_base + 0x000E);
    if((ts_status_hb == TS_PARKED) || (ts_status_lb == TS_PARKED)){
        printk("no need to stop!!!! now trx_status_hb %d trx_status_lb: %d\n",ts_status_hb, ts_status_lb);
    }
    rf_trx_stop(0);
    rf_trx_stop(1);
    rf_switch_operating_with_standby(SS_STANDBY);
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _APP_ConfigTrx, ate_num);
    if (resp_args != 1) {
        printk("_APP_ConfigTrx response num failed!, resp_args : %d\n", resp_args);
        return -1;
    }
    value = ml_apb_read(cmd_base + 0x000B);
    if (value) {
        printk("_APP_ConfigTrx, failed!, resp_value : %d\n", value);
        return -2;
    }
    rf_switch_operating_with_standby(SS_OPERATING);
    rf_trx_start(0,freq_hb,chbw_hb);
    rf_trx_start(1,freq_lb,chbw_lb);
#ifdef UPDATE_GAIN_TABLE
    if(g_rf_pl_ctx->dual_antenna_calibrate){
  rf_update_hb_tx_gain_table();
  rf_update_gain_table(LB_TX_GAIN_TABLE_0, &rf_lb_gain_table[0],32);
  rf_update_gain_table(LB_TX_DBB_GAIN_TABLE_0, &rf_lb_dbb_gain_table[0],16);
 }
#endif
    return 0;
}EXPORT_SYMBOL_GPL(rf_switch_aet_trx);
int rf_app_calibrate(bool reactive)
{
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    int8_t resp_args;
    uint16_t freq_hb, chbw_hb;
    uint16_t freq_lb, chbw_lb;
    uint16_t value;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, 0);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -1;
    }
    freq_hb = ml_apb_read(cmd_base + 0x000D);
    chbw_hb = ml_apb_read(cmd_base + 0x000E);
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, 1);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -2;
    }
    freq_lb = ml_apb_read(cmd_base + 0x000D);
    chbw_lb = ml_apb_read(cmd_base + 0x000E);
    if(reactive){
        resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_Deactivate);
        if (resp_args != 1) {
            printk("_APP_DeActivate response num error %d",resp_args);
            return -3;
        }
        value = ml_apb_read(ML_CMD_IF_p1_BASE + 0x000B);
        if (value != 0x0) {
            printk("_APP_DeActivate error 0x%x\n",value);
            return -4;
        }
        resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_Activate);
        if (resp_args != 1) {
            printk("_APP_Activate response num error %d",resp_args);
            return -3;
        }
        value = ml_apb_read(ML_CMD_IF_p1_BASE + 0x000B);
        if (value != 0x0) {
            printk("_APP_Activate error 0x%x\n",value);
            return -4;
        }
        udelay(100);
    }else{
        resp_args = ml_apb_send_0_params_cmd(1, cmd_base, _APP_Calibrate);
        if (resp_args != 1) {
            printk("app calibration failed, resp_args : %d\n", resp_args);
            return -3;
        }
    }
    cmd_base = HB_HL_CMD_P1_COMMAND_HEADER;
    resp_args = ml_apb_send_3_params_cmd(1, cmd_base, _TRX_Prepare, 0, freq_hb, chbw_hb);
    if (resp_args != 1) {
        printk("trx prepare failed, resp_args : %d\n", resp_args);
        return -4;
    }
    cmd_base = HB_LL_CMD_P1_COMMAND_HEADER;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Receive, 0);
    if (resp_args != 1) {
        printk("trx receive failed, resp_args : %d\n", resp_args);
        return -5;
    }
    cmd_base = LB_HL_CMD_P1_COMMAND_HEADER;
    resp_args = ml_apb_send_3_params_cmd(1, cmd_base, _TRX_Prepare, 1, freq_lb, chbw_lb);
    if (resp_args != 1) {
        printk("trx prepare failed, resp_args : %d\n", resp_args);
        return -7;
    }
    cmd_base = LB_LL_CMD_P1_COMMAND_HEADER;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Receive, 1);
    if (resp_args != 1) {
        printk("trx receive failed, resp_args : %d\n", resp_args);
        return -8;
    }
    return 0;
}
int rf_get_temperature(bool from_lmac)
{
    int16_t value = -274;
        int8_t resp_args;
        struct rf_pl_context *pl_ctx = g_rf_pl_ctx;
        mutex_lock(&pl_ctx->bb_mtx);
        resp_args = ml_apb_send_0_params_cmd(1, ML_CMD_P1_COMMAND_HEADER, _APP_GetTempreture);
        WARN_ON(resp_args != 2);
        value = (ml_apb_read(ML_CMD_P1_COMMAND_HEADER + 0x000C) & 0xFFFF);
        mutex_unlock(&pl_ctx->bb_mtx);
    return value;
}
EXPORT_SYMBOL_GPL(rf_get_temperature);
int rf_trx_set_rfappconfig(
               uint8_t conf_in_idel_hb1, uint8_t conf_in_idel_hb2,uint8_t conf_in_rx_hb1, uint8_t conf_in_rx_hb2,
               uint8_t conf_in_tx_hb1, uint8_t conf_in_tx_hb2, uint8_t conf_in_pa_hb1, uint8_t conf_in_pa_hb2,
               uint8_t conf_in_idel_lb1, uint8_t conf_in_idel_lb2,uint8_t conf_in_rx_lb1, uint8_t conf_in_rx_lb2,
               uint8_t conf_in_tx_lb1, uint8_t conf_in_tx_lb2, uint8_t conf_in_pa_lb1, uint8_t conf_in_pa_lb2
               )
{
    uint32_t cmd_base = ML_CMD_IF_p1_BASE;
    int8_t resp_args;
    int value;
    ml_apb_write(cmd_base, _APP_ConfigFemCtrl);
    ml_apb_write(cmd_base + 0x0001, conf_in_idel_hb1 | (conf_in_idel_hb2 << 8));
    ml_apb_write(cmd_base + 0x0002, conf_in_rx_hb1 | (conf_in_rx_hb2 << 8));
    ml_apb_write(cmd_base + 0x0003, conf_in_tx_hb1 | (conf_in_tx_hb2 << 8));
    ml_apb_write(cmd_base + 0x0004, conf_in_pa_hb1 | (conf_in_pa_hb2 << 8));
    ml_apb_write(cmd_base + 0x0005, conf_in_idel_lb1 | (conf_in_idel_lb2 << 8));
    ml_apb_write(cmd_base + 0x0006, conf_in_rx_lb1 | (conf_in_rx_lb2 << 8));
    ml_apb_write(cmd_base + 0x0007, conf_in_tx_lb1 | (conf_in_tx_lb2 << 8));
    ml_apb_write(cmd_base + 0x0008, conf_in_pa_lb1 | (conf_in_pa_lb2 << 8));
#if 1
    printk("command header： %x\n", _APP_ConfigFemCtrl);
    printk("req DW1=0x%x DW2=0x%x DW3=0x%x DW4=0x%x DW5=0x%x DW6=0x%x DW7=0x%x DW8=0x%x DW9=0x%x\n",
                ml_apb_read(cmd_base + 0x0001), ml_apb_read(cmd_base + 0x0002), ml_apb_read(cmd_base + 0x0003), ml_apb_read(cmd_base + 0x0004), ml_apb_read(cmd_base + 0x0005),
                ml_apb_read(cmd_base + 0x0006), ml_apb_read(cmd_base + 0x0007), ml_apb_read(cmd_base + 0x0008), ml_apb_read(cmd_base + 0x0009)
                );
#endif
    resp_args = ml_apb_wait_cmd_resp(1, cmd_base, _APP_ConfigFemCtrl);
    if (resp_args != 1) {
        printk("rf_trx_set_rfappconfig response error %d",resp_args);
        return -3;
    }
    value = ml_apb_read(cmd_base + 0x000B);
    if (value != 0x0) {
        printk("rf_trx_set_rfappconfig error 0x%x\n",value);
        return -4;
    }
    return 0;
}
#ifdef COOLING_TEMP
static void rf_get_boot_temp(struct rf_pl_context *priv)
{
#if 0
    struct file *fp;
    loff_t fp_pos = 0;
    mm_segment_t fs;
    bool first_record = false;
    char temp[2] = {0};
    int record_value = 0;
    int value = rf_get_temperature(0);
    printk("rf_cooling tempture %s %d\n",__func__,__LINE__);
    fp = filp_open(SF_BOOT_TEMP_FILE_NAME, O_RDWR, 0777);
    if (IS_ERR(fp)) {
        printk("rf_temp do not creat,creat it now\n");
        fp = filp_open(SF_BOOT_TEMP_FILE_NAME, O_RDWR | O_CREAT, 0777);
        if (IS_ERR(fp)) {
            printk("can not create the rf_temp\n");
            return;
        }
        first_record = true;
        goto NEXT;
    }
    if (value > ABNORMAL_BOOT_TEMP) {
        fs = get_fs();
        set_fs(KERNEL_DS);
        kernel_read(fp, temp, sizeof(temp), &fp_pos);
        set_fs(fs);
        filp_close(fp, NULL);
        if (sscanf(temp,"%d",(int *)(&record_value)) != 1)
            printk("get record value error\n");
        printk("record value is %d\n",record_value);
        priv->rf_thermal_state.relative_diff = record_value - REFERENCE_TEMP;
        return;
    }
NEXT:
    fs = get_fs();
    set_fs(KERNEL_DS);
    sprintf(temp,"%d",value);
    fp_pos = 0;
    kernel_write(fp, temp, sizeof(temp), &fp_pos);
    set_fs(fs);
    filp_close(fp, NULL);
    if (value > REFERENCE_TEMP)
        priv->rf_thermal_state.relative_diff = value - REFERENCE_TEMP;
    else
#endif
        priv->rf_thermal_state.relative_diff = 0;
}
#endif
#ifdef UPDATE_GAIN_TABLE
#define SF19A28_RF_GAIN_NUM 32
static uint16_t rf_hb_gain_table[SF19A28_RF_GAIN_NUM]={
    0x0005,0x0007,0x0009,0x0025,0x0027,0x0029,0x0035,0x0037,
    0x0039,0x0076,0x0078,0x007a,0x0086,0x0088,0x008a,0x0096,
    0x0098,0x009a,0x00d7,0x00e3,0x00e5,0x00e7,0x0135,0x0137,
    0x0139,0x0145,0x0147,0x0149,0x01a5,0x01a7,0x01a9,0x01b5,
};
static uint16_t rf_hb_gain_table_def[SF19A28_RF_GAIN_NUM]={
    0x008a,0x00c8,0x00ca,0x00d6,0x00d8,0x0127,0x0129,0x0135,
    0x0137,0x0139,0x0145,0x01a5,0x01a7,0x01a9,0x01b5,0x01b7,
    0x01b9,0x01c5,0x01c7,0x01c9,0x01d5,0x01d7,0x01d9,0x01e5,
    0x01e7,0x01e9,0x01f5,0x01f7,0x01f9,0x01fb,0x01fd,0x01ff,
};
uint16_t rf_lb_gain_table[SF19A28_RF_GAIN_NUM]={
#ifdef CONFIG_SF16A18_WIFI_LB_EXT_PA_ENABLE
    0x0006,
    0x0022,
    0x0024,
    0x0026,
    0x0032,
    0x0034,
    0x0036,
    0x0042,
    0x0044,
    0x0046,
    0x0052,
    0x0054,
    0x0056,
    0x0092,
    0x0094,
    0x0096,
    0x00A2,
    0x00A4,
    0x00A6,
    0x00B2,
    0x00B4,
    0x00B6,
    0x00F2,
    0x00F4,
    0x00F6,
    0x0102,
    0x0104,
    0x0106,
    0x0112,
    0x0114,
    0x0116,
    0x0162,
#else
    0x009d,
    0x00A9,
    0x00Ab,
    0x00Ad,
    0x00E9,
    0x00Eb,
    0x00Ed,
    0x00F8,
    0x00Fa,
    0x00Fc,
    0x00Fe,
    0x014a,
    0x014c,
    0x014e,
    0x015a,
    0x015c,
    0x015e,
    0x016a,
    0x016c,
    0x016e,
    0x017a,
    0x017c,
    0x017e,
    0x01Db,
    0x01Dd,
    0x01Ea,
    0x01Ec,
    0x01Ee,
    0x01Fa,
    0x01Fc,
    0x01Fe,
    0x01FF,
#endif
};
uint16_t rf_lb_gain_table_def[SF19A28_RF_GAIN_NUM]={
0X009C,
0x009E,
0x00AA,
0x00AC,
0x00AE,
0x00EA,
0x00EC,
0x00EE,
0x00F9,
0x00FB,
0x00FD,
0x00FF,
0x014B,
0x014D,
0x014F,
0x015B,
0x015D,
0x015F,
0x016B,
0x016D,
0x016F,
0x017B,
0x017D,
0x017F,
0x01DC,
0x01DE,
0x01EB,
0x01ED,
0x01EF,
0x01FB,
0x01FD,
0x01FF,
};
uint16_t rf_lb_dbb_gain_table[SF19A28_RF_DBB_GAIN_NUM]={
0x01C9,
0x01E4,
0x0201,
0x021F,
0x0240,
0x0262,
0x0286,
0x02AC,
0x02D5,
0x0300,
0x032D,
0x035D,
0x0390,
0x03C6,
0x0400,
0x0500,
};
uint16_t rf_lb_dbb_gain_table_def[SF19A28_RF_DBB_GAIN_NUM]={
0x01B0,
0x01C9,
0x01E4,
0x0201,
0x021F,
0x0240,
0x0262,
0x0286,
0x02AC,
0x02D5,
0x0300,
0x032D,
0x035D,
0x0390,
0x03C6,
0x0400,
};
void rf_reset_hb_tx_gain_table(void){
    int i=0;
    uint16_t rf_gain_base_addr=HB_TX_GAIN_TABLE_0;
    for(i=0;i<32;i++){
        ml_apb_write(rf_gain_base_addr,rf_hb_gain_table_def[i]);
        rf_gain_base_addr += 1;
        printk("addr %d rf_gain[i]%d\n",rf_gain_base_addr,rf_hb_gain_table_def[i]);
    }
}
void rf_update_hb_tx_gain_table(void){
    int i=0;
    uint16_t rf_gain_base_addr=HB_TX_GAIN_TABLE_0;
    for(i=0;i<32;i++){
        ml_apb_write(rf_gain_base_addr,rf_hb_gain_table[i]);
        rf_gain_base_addr += 1;
    }
}
void rf_update_gain_table(uint16_t table_base_addr, uint16_t *table_addr,int gain_num){
    int i=0;
    uint16_t rf_gain_base_addr = table_base_addr;
    uint16_t *rf_gain = table_addr;
    for(i=0;i<gain_num;i++){
        ml_apb_write(rf_gain_base_addr,rf_gain[i]);
        rf_gain_base_addr += 1;
    }
}
#endif
int rf_bootup(void)
{
    int8_t resp_number;
    uint16_t value;
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    struct rf_pl_context *priv = g_rf_pl_ctx;
    printk("%s\n", __func__);
    resp_number = ml_apb_send_0_params_cmd(1, cmd_base, _GEN_GetVersion);
    WARN_ON(4 != resp_number);
    value = ml_apb_read(cmd_base + 0x000C);
    printk("rf hw version : 0x%x\n", value);
    priv->rf_hw_version = value;
    value = ml_apb_read(cmd_base + 0x000D);
    priv->rf_sw_version = value;
    printk("rf hw target version %s, sw version %d.%d\n",
            ((value & 0xf000) >> 12 ) == 2 ? "Begonia" : "unknown",
            (value & 0x0f00) >> 8, (value & 0x00ff));
    value = ml_apb_read(cmd_base + 0x000E);
    priv->rf_sw_version1 = value;
    printk("debug version: %d, test number : %d, build number : %d\n",
            (value & 0x8000) >> 15, (value & 0x7f00) >> 8, (value & 0x00ff));
    if (g_rf_pl_ctx->lb_ex_pa_exist || g_rf_pl_ctx->hb_ex_pa_exist) {
        if (rf_trx_set_rfappconfig(
                    priv->ex_pa_config.hb1_idle_cfg, priv->ex_pa_config.hb2_idle_cfg,
                    priv->ex_pa_config.hb1_rx_cfg, priv->ex_pa_config.hb2_rx_cfg,
                    priv->ex_pa_config.hb1_tx_cfg, priv->ex_pa_config.hb2_tx_cfg,
                    priv->ex_pa_config.hb1_pa_cfg, priv->ex_pa_config.hb2_pa_cfg,
                    priv->ex_pa_config.lb1_idle_cfg, priv->ex_pa_config.lb2_idle_cfg,
                    priv->ex_pa_config.lb1_rx_cfg, priv->ex_pa_config.lb2_rx_cfg,
                    priv->ex_pa_config.lb1_tx_cfg, priv->ex_pa_config.lb2_tx_cfg,
                    priv->ex_pa_config.lb1_pa_cfg, priv->ex_pa_config.lb2_pa_cfg)) {
            printk("rf_trx_set_rfappconfig failed!\n");
            return -4;
        }
    }
    if (rf_app_prepare(priv->trx_path_cfg)) {
        printk("rf_app_prepare failed!\n");
        return -1;
    }
    if (((priv->rf_sw_version & 0xf) > 0x9) ){
        rf_set_property(HB_RX_STEP2_DELAY,priv->ex_pa_config.hb_rx_step2_delay);
        rf_set_property(HB_RX_STEP2_VALUE,priv->ex_pa_config.hb_rx_step2_value | (priv->ex_pa_config.hb_rx_step2_value << 8));
        rf_set_property(HB_TX_STEP2_DELAY,priv->ex_pa_config.hb_tx_step2_delay);
    }
#ifdef COOLING_TEMP
    rf_get_boot_temp(priv);
#endif
    return 0;
}
int32_t rf_get_trx_status(bool is_hb, uint16_t *trx_state, uint16_t *freq, uint16_t *chbw)
{
    uint16_t cmd_base = ML_CMD_IF_p1_BASE;
    int8_t resp_args;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, is_hb ? 0 : 1);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -1;
    }
    *trx_state = ml_apb_read(cmd_base + 0x000C);
    *freq = ml_apb_read(cmd_base + 0x000D);
    *chbw = ml_apb_read(cmd_base + 0x000E);
    return 0;
}
int8_t rf_set_test_dc(bool band,uint8_t path,bool status,uint16_t dcre,uint16_t dcim){
    uint16_t dw1= (status << 8) | (path<<6) | band ;
    uint32_t resp_args = 0;
    uint32_t value = 0;
    uint32_t cmd_base = ML_CMD_IF_p1_BASE;
    uint16_t trx_state;
    printk("set dc dw1 :0x%x\n",dw1);
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, band);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -1;
    }
    trx_state = ml_apb_read(cmd_base + 0x000C);
    if(trx_state != 3)
    {
        printk("trx_status is no transmit!!!!please check!!!\n");
        return 1;
    }
    resp_args = ml_apb_send_3_params_cmd(1, cmd_base, _TRX_SetTestDc, dw1, dcre, dcim);
    if(resp_args != 1){
        printk(", response num failed!, resp_args : %d\n", resp_args);
        return -1;
    }
    value = ml_apb_read(cmd_base + 0x000B);
    if(value){
        printk("_TRX_SetTestDc,, failed!, resp_value : %d\n", value);
        return -2;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(rf_set_test_dc);
int32_t rf_trx_status_change(int txpower_idx,uint32_t txrxid,bool txrx)
{
    uint32_t resp_args = 0;
    uint32_t value = 0;
    uint32_t cmd_base = ML_CMD_IF_p1_BASE;
    uint16_t trx_state=0;
    uint16_t power = txpower_idx;
    resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_GetStatus, txrxid);
    if (resp_args != 4) {
        printk("get TRX_GetStatus failed, resp_args : %d\n", resp_args);
        return -1;
    }
    trx_state = ml_apb_read(cmd_base + 0x000C);
    printk("trx_statues is %d\n",trx_state);
    if (!txrxid){
        cmd_base=HB_LL_CMD_IF_p1_BASE;
    }
    else{
        cmd_base=LB_LL_CMD_IF_p1_BASE;
    }
    if(trx_state == 3 || trx_state == 2)
    {
        if (txrx){
            printk("TO RX MODE\n");
            resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Receive,txrxid );
            if(resp_args != 1) {
                printk("TRX_Receive response num error %d\n",resp_args);
                return -1;
            }
            return 1;
        }
        else{
            printk("now trx_status is transmit ,no need change\n");
            return 1;
        }
    }
    else if(trx_state == 4){
        if (!txrx){
            printk("To TX mode\n");
            resp_args = ml_apb_send_1_params_cmd(1, cmd_base, _TRX_Transmit,txrxid|(power << 8 ));
            if(resp_args != 1) {
                printk("TRX_Receive response num error %d\n",resp_args);
                return -1;
            }
            value = ml_apb_read(cmd_base + 0x000B);
            if(value) {
                printk("TRX_Receive fail %d\n",value);
                return -2;
            }
            return 1;
        }
        else{
            printk("now trx_status is receive ,no need change\n");
            return 1;
        }
    }
    return 0;
}
EXPORT_SYMBOL_GPL(rf_trx_status_change);
