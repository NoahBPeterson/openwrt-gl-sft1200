#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/of_net.h>
#include <linux/mtd/mtd.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/thermal.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/reset.h>
#ifdef COOLING_TEMP
#include <sfax8_thermal.h>
#endif
#ifdef CONFIG_SFAX8_FACTORY_READ
#include <sfax8_factory_read.h>
#endif
#ifdef CONFIG_SF16A18_RF_SYSFS_DEBUG
#include "rf_sysfs.h"
#endif
#include "rf_pl_ref.h"
#include "rf_access.h"
#define SF_RF_MODULE_NAME "sf16a28_rf"
#define BOOT_FINISH_WITH_IRQ 
#define SF_RF_FW_NAME "rf_pmem.bin"
#define SF_RF_DEFAULT_REG_NAME "rf_default_reg.bin"
#define SF_RF_XDMA_REG_NAME "rf_xdma_reg.bin"
#define SF_RF_EXPA_CONFIG "sf_rf_expa_config.ini"
#define SF_RF_TRX_PATH_CONFIG "rf_trx_path.ini"
#define IRQ_SOURCE ML_CMD_P1_IRQ_SOURCE
#define HB_IRQ_ENABLE ML_CMD_P1_IRQ_ENABLE
#define LB_IRQ_ENABLE ML_CMD_P1_IRQ_ENABLE
#define UMASK_RF_IRQ 0x0CFF
#define IRQ_HK_TEMP 0
#define IRQ_HK_APP_STATUS 3
#define IRQ_HK_TRX_STATUS_HB SF_RF_IRQ_SOURCE_STOP_REQ_HB
#define IRQ_HK_TRX_STATUS_LB SF_RF_IRQ_SOURCE_STOP_REQ_LB
#define SILENCE_CALIBRATION 
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
int ate_aetset = 0;
EXPORT_SYMBOL_GPL(ate_aetset);
#endif
int thermal_on = 1;
module_param_named(thermal_on,thermal_on,int,S_IRUGO);
MODULE_PARM_DESC(thermal_on,"ENABLE THERMAL (DEFAULT :1)\n");
enum SF_RF_IRQ_SOURCE{
    SF_RF_IRQ_SOURCE_BOOT_ERROR = 0,
    SF_RF_IRQ_SOURCE_CAL_REQ_SYS = 3,
    SF_RF_IRQ_SOURCE_CAL_REQ_CLK = 4,
    SF_RF_IRQ_SOURCE_STOP_REQ_LB = 5,
    SF_RF_IRQ_SOURCE_STOP_REQ_HB = 6,
    SF_RF_IRQ_SOURCE_CMD_ERR_HLC_HB = 8,
    SF_RF_IRQ_SOURCE_CMD_ERR_HLC_LB = 9,
    SF_RF_IRQ_SOURCE_CMD_ERR_LLC_HB = 10,
    SF_RF_IRQ_SOURCE_CMD_ERR_LLC_LB = 11
};
#define SF_RF_CALIBRATION_MASK (0x1 << SF_RF_IRQ_SOURCE_BOOT_ERROR | 0x1 << SF_RF_IRQ_SOURCE_CAL_REQ_SYS | 0x1 << SF_RF_IRQ_SOURCE_CAL_REQ_CLK \
        | 0x1 << SF_RF_IRQ_SOURCE_STOP_REQ_LB | 0x1 << SF_RF_IRQ_SOURCE_STOP_REQ_HB)
void rfchip_hot(uint32_t timeout)
{
    int i;
    uint16_t default_value[32];
    uint16_t addr[32] = {
        0x0001,
        0x1001,
        0x2001,
        0x2080,
        0x2332,
        0x2338,
        0x2352,
        0x2358,
        0x2382,
        0x2388,
        0x23A2,
        0x23A8,
        0x233B,
        0x235B,
        0x238B,
        0x23AB,
        0x233C,
        0x235C,
        0x238C,
        0x23AC,
        0x233D,
        0x235D,
        0x238D,
        0x23AD,
        0x0030,
        0x1030,
        0x01CF,
        0x11CF,
        0x01BF,
        0x11BF,
        0x0003,
        0x1003
    };
    uint16_t heating_value[32] = {
         0x0F00,
         0x0F00,
         0x0002,
         0x0000,
         0x0400,
         0x0303,
         0x0400,
         0x0303,
         0x0400,
         0x0303,
         0x0400,
         0x0303,
         0x286D,
         0x286D,
         0x1066,
         0x1066,
         0x1F77,
         0x1F77,
         0x1F43,
         0x1F43,
         0x6000,
         0x6000,
         0x6000,
         0x6000,
         0x0007,
         0x0007,
         0x3F3F,
         0x3F3F,
         0x01FF,
         0x01FF,
         0x1F1F,
         0x1F1F
    };
    for (i = 0; i < 32; i++) {
        if ((i >= 4) && (i <= 23)) {
            default_value[i] = ml_apb_rsbus_read(addr[i]);
        } else {
            default_value[i] = ml_apb_read(addr[i]);
        }
    }
    for (i = 0; i < 32; i++) {
        ml_apb_write(addr[i], heating_value[i]);
    }
    udelay(timeout);
    for (i = 0; i < 32; i++) {
        ml_apb_write(addr[i], default_value[i]);
    }
}
struct rf_pl_context *g_rf_pl_ctx;
int call_umhelper(char *cmd, char *args)
{
    char *envp[] = { "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
    char **argv;
    int argc, ret;
    char *cmd_buf;
    cmd_buf = (char *)kmalloc((strlen(cmd) + strlen(args) + 8), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if(!cmd_buf)
        return 0;
    sprintf(cmd_buf, "%s %s\n", cmd, args);
    printk("%s, cmd : %s, args : %s\n", __func__, cmd, args);
    argv = argv_split(in_interrupt() ? GFP_ATOMIC : GFP_KERNEL, cmd_buf, &argc);
    if (!argc){
        kfree(cmd_buf);
        return -EINVAL;
    }
    if ((ret = call_usermodehelper(argv[0], argv, envp,
                                   UMH_WAIT_PROC | UMH_KILLABLE)))
        printk(KERN_CRIT "Failed to call %s (%s returned %d)\n",
               argv[0], cmd, ret);
    argv_free(argv);
    kfree(cmd_buf);
    return ret;
}
struct siwifi_reprobe {
    struct device *dev;
    struct work_struct work;
};
void siwifi_reprobe_work(struct work_struct *wk)
{
    struct siwifi_reprobe *reprobe;
    printk("siwifi_reprobe_work>>\n");
    reprobe = container_of(wk, struct siwifi_reprobe, work);
    kfree(reprobe);
    call_umhelper("/bin/reload_wifi.sh", "0");
    printk("siwifi_reprobe_work<<\n");
}
#ifndef SILENCE_CALIBRATION
static int sf_wifi_rf_trx_calibrate(struct rf_pl_context *priv)
{
    printk("%s\n", __func__);
    if(test_bit(IRQ_HK_TRX_STATUS_HB, &priv->irq_source))
        call_umhelper("/bin/sfwifi" , "reload_band hb");
    else if(test_bit(IRQ_HK_TRX_STATUS_LB, &priv->irq_source))
        call_umhelper("/bin/sfwifi" , "reload_band lb");
    return 0;
}
static int sf_wifi_rf_app_calibrate(struct rf_pl_context *priv,bool reactive)
{
    printk("%s\n", __func__);
    if(call_umhelper("/bin/sfwifi", "remove_band all")){
        printk("try to remove allband failed!\n");
        return -1;
    }
    if(rf_app_calibrate(reactive)){
        printk("rf_app_calibrate failed!\n");
        return -2;
    }
    if(call_umhelper("/bin/sfwifi", "reload_band all")){
        printk("try to install all band failed\n");
        return -3;
    }
    return 0;
}
#else
static int sf_wifi_rf_trx_calibrate(struct rf_pl_context *priv)
{
    int i;
    int ret;
    uint16_t cali_conf = 0xFFFF;
    printk("%s\n", __func__);
    mutex_lock(&priv->bb_mtx);
    for(i = 0; i < SUPPORT_MAX_CLIENT_NUM; i++){
        if(priv->clients[i].band_type) {
            if((test_bit(IRQ_HK_TRX_STATUS_LB, &priv->irq_source) &&
                        priv->clients[i].band_type == CLIENT_LB) ||
                        (test_bit(IRQ_HK_TRX_STATUS_HB, &priv->irq_source) &&
                        priv->clients[i].band_type == CLIENT_HB))
            {
                ret = priv->clients[i].cb(priv->clients[i].cb_data, RF_EVENT_TRX_RECALI, 0, &cali_conf);
                if(ret){
                    printk("RF_EVENT_TRX_RECALI failed, ret : %d\n", ret);
                    WARN_ON(1);
                }
            }
        }
    }
    mutex_unlock(&priv->bb_mtx);
    return 0;
}
static int sf_wifi_rf_app_calibrate(struct rf_pl_context *priv,bool reactive)
{
    int i;
    int ret;
    unsigned long now = jiffies;
    static struct {
        unsigned long last;
        unsigned int cnt;
        unsigned int max_cnt;
    } resche_recs = { .last = 0, .cnt = 0, .max_cnt = 5 };
    printk("%s\n", __func__);
    mutex_lock(&priv->bb_mtx);
    for(i = 0; i < SUPPORT_MAX_CLIENT_NUM; i++) {
        if(priv->clients[i].band_type) {
            ret = priv->clients[i].cb(priv->clients[i].cb_data, RF_EVENT_STOP_BUSINESS, 0, NULL);
            printk("stop business return!\n");
            if(ret == RF_RET_AGAIN)
            {
                if(resche_recs.cnt) {
                    if (jiffies_to_msecs(now - resche_recs.last) > 3000) {
                        resche_recs.cnt = 0;
                    } else if (resche_recs.cnt > resche_recs.max_cnt) {
                        printk(KERN_CRIT "%s: Too many failures .. aborting\n", __func__);
                        goto CLEAN_UP;
                    }
                }
                resche_recs.cnt++;
                resche_recs.last = now;
                mutex_unlock(&priv->bb_mtx);
                return ret;
            } else if(ret != 0){
                printk("RF_EVENT_STOP_BUSINESS failed, ret : %d\n", ret);
                goto CLEAN_UP;
            }
        }
    }
    if (rf_app_calibrate(reactive)) {
        printk("rf_app_calibrate failed, this is a fatal error!\n");
    }
CLEAN_UP:
    for (i = 0; i < SUPPORT_MAX_CLIENT_NUM; i++) {
        if (priv->clients[i].band_type) {
            ret = priv->clients[i].cb(priv->clients[i].cb_data, RF_EVENT_START_BUSINESS, 0, NULL);
            WARN_ON(ret);
        }
    }
    mutex_unlock(&priv->bb_mtx);
    return 0;
}
#endif
static int sf_wifi_int_handler(struct rf_pl_context *priv)
{
    int ret = 0;
    if (test_bit(SF_RF_IRQ_SOURCE_BOOT_ERROR, &priv->irq_source)) {
        struct siwifi_reprobe *reprobe;
        printk("rf boot error\n");
        reprobe = kzalloc(sizeof(*reprobe), GFP_ATOMIC);
        if (!reprobe) {
            return 0;
        }
        reprobe->dev = priv->dev;
        INIT_WORK(&reprobe->work, siwifi_reprobe_work);
        schedule_work(&reprobe->work);
        ret = 0;
    }else if(test_bit(SF_RF_IRQ_SOURCE_CAL_REQ_CLK, &priv->irq_source)){
        ret = sf_wifi_rf_app_calibrate(priv, true);
    }else if(test_bit(SF_RF_IRQ_SOURCE_CAL_REQ_SYS, &priv->irq_source)){
        ret = sf_wifi_rf_app_calibrate(priv, false);
    }else if(test_bit(SF_RF_IRQ_SOURCE_STOP_REQ_LB, &priv->irq_source) || test_bit(SF_RF_IRQ_SOURCE_STOP_REQ_HB, &priv->irq_source)){
        ret = sf_wifi_rf_trx_calibrate(priv);
    }
    return ret;
}
void sf_wifi_rf_calibration_work(struct work_struct *work)
{
    int ret;
#ifdef PROFILING_CALIBRATION_TIME
    unsigned long start;
    uint32_t dur;
#endif
    struct rf_pl_context *priv =
        container_of(work, struct rf_pl_context, cali_work.work);
    if(!priv->irq_source){
        ml_apb_write(HB_IRQ_ENABLE, UMASK_RF_IRQ);
        ml_apb_write(LB_IRQ_ENABLE, UMASK_RF_IRQ);
        return;
    }
    priv->calibrating = true;
    printk("sf_wifi_rf_calibration_work, irq_source : %lx\n", priv->irq_source);
#ifdef PROFILING_CALIBRATION_TIME
    start = jiffies;
#endif
    ret = sf_wifi_int_handler(priv);
    if(ret == RF_RET_AGAIN)
        goto AGAIN;
#ifdef PROFILING_CALIBRATION_TIME
    dur = jiffies_to_msecs(jiffies - start);
    printk("the calibration take %d ms\n", dur);
    if(dur >= 1000)
        printk("the calibration take too long time, %d ms\n", dur);
#endif
    priv->cali_work_num++;
    priv->irq_source = 0;
    ml_apb_write(HB_IRQ_ENABLE, UMASK_RF_IRQ);
    ml_apb_write(LB_IRQ_ENABLE, UMASK_RF_IRQ);
    priv->calibrating = false;
    printk("hk calibration>>\n");
    return;
AGAIN:
    printk("bb ask us to do calibration again!\n");
    schedule_delayed_work(&priv->cali_work, msecs_to_jiffies(500));
    return;
}
#ifdef DUMP_IRQ_STATUS
static void sf_dump_llc_cmd_buffer(void)
{
    uint16_t addr_hb = 0x5e83;
    uint16_t addr_lb = 0x5ea3;
    int i = 0;
    printk("HB LOG buffer============= first entry=%d\n", ml_apb_read(0x5ec3));
    while(addr_hb < 0x5eA2){
        printk("entry=%d addr[%04x] command=%04x rtc=%04x\n", i, addr_hb, ml_apb_read(addr_hb), ml_apb_read(addr_hb + 1));
        addr_hb += 2;
        i++;
    }
    printk("LB LOG buffer============= first entry=%d\n", ml_apb_read(0x5ec4));
    i = 0;
    while(addr_lb < 0x5ec2){
        printk("entry=%d addr[%04x] command=%04x rtc=%04x\n", i, addr_lb, ml_apb_read(addr_lb), ml_apb_read(addr_lb + 1));
        addr_lb += 2;
        i++;
    }
}
#endif
static irqreturn_t sf_wifi_rf_irq_handle(int irq, void *params)
{
    struct platform_device *pdev;
    struct rf_pl_context *priv;
    uint16_t tmp;
    pdev = (struct platform_device *)params;
    priv = (struct rf_pl_context *)platform_get_drvdata(pdev);
    spin_lock(&priv->cali_lock);
    tmp = ml_apb_read(IRQ_SOURCE);
#ifdef DUMP_IRQ_STATUS
    sf_dump_llc_cmd_buffer();
    printk("ML_CMD_P1_IRQ_SOURCE(%x) = %x \n", ML_CMD_P1_IRQ_SOURCE, tmp);
    printk("LB_LL_CMD_P0_IRQ_SOURCE(%x) = %x \n", LB_LL_CMD_P0_IRQ_SOURCE, ml_apb_read(LB_LL_CMD_P0_IRQ_SOURCE));
    printk("LB_LL_CMD_P1_IRQ_SOURCE(%x) = %x \n", LB_LL_CMD_P1_IRQ_SOURCE, ml_apb_read(LB_LL_CMD_P1_IRQ_SOURCE));
    printk("LB_LL_CMD_P0_RESPONSE_HEADER(%x) = %x \n", LB_LL_CMD_P0_RESPONSE_HEADER, ml_apb_read(LB_LL_CMD_P0_RESPONSE_HEADER));
    printk("LB_LL_CMD_P0_RESPONSE_DATA_1(%x) = %x \n", LB_LL_CMD_P0_RESPONSE_DATA_1, ml_apb_read(LB_LL_CMD_P0_RESPONSE_DATA_1));
    printk("LB_LL_CMD_P1_RESPONSE_HEADER(%x) = %x \n", LB_LL_CMD_P1_RESPONSE_HEADER, ml_apb_read(LB_LL_CMD_P1_RESPONSE_HEADER));
    printk("LB_LL_CMD_P1_RESPONSE_DATA_1(%x) = %x \n", LB_LL_CMD_P1_RESPONSE_DATA_1, ml_apb_read(LB_LL_CMD_P1_RESPONSE_DATA_1));
#endif
    while(ml_apb_read(IRQ_SOURCE)){
        printk("rf irq do not clear\n");
    }
    tmp &= 0xFFF;
    printk("sf_wifi_rf_irq_handle, irq : 0x%x\n", tmp);
    if(tmp & 0xF)
        priv->hk_irq_source_num[(tmp & 0xF)]++;
    if(tmp & 0xF0)
        priv->hk_irq_source_num[(tmp & 0xF0) >> 4]++;
    if(tmp & 0xF00)
        priv->hk_irq_source_num[(tmp & 0xF00) >> 8]++;
    if(tmp & SF_RF_CALIBRATION_MASK){
        if(priv->enable_recali){
            ml_apb_write(HB_IRQ_ENABLE, 0);
            ml_apb_write(LB_IRQ_ENABLE, 0);
            priv->irq_source = ((tmp & 0xFF) >> 4);
            schedule_delayed_work(&priv->cali_work, 0);
        }
    }
    spin_unlock(&priv->cali_lock);
    priv->irq_num++;
    return IRQ_HANDLED;
}
static int sf_wifi_rf_irqs_register(struct platform_device *pdev, struct rf_pl_context *priv)
{
    int ret;
    ret = request_irq(priv->irq, sf_wifi_rf_irq_handle,
            IRQF_TRIGGER_LOW, dev_name(&pdev->dev), pdev);
    if(ret){
        printk("can not request irq %d\n", priv->irq);
        return -1;
    }
    ml_apb_write(HB_IRQ_ENABLE, UMASK_RF_IRQ);
    ml_apb_write(LB_IRQ_ENABLE, UMASK_RF_IRQ);
    return 0;
}
static void sf_wifi_rf_irqs_unregister(struct platform_device *pdev, struct rf_pl_context *priv)
{
    printk("%s\n", __func__);
    free_irq(priv->irq, pdev);
}
static const char *find_tag_from_file(const u8 *file_data, unsigned int file_size,
                const char *tag_name, unsigned int tag_len)
{
    unsigned int curr, line_start = 0, line_size;
    while (line_start < file_size) {
        for (curr = line_start; curr < file_size; curr++)
            if (file_data[curr] == '\n')
                break;
        line_size = curr - line_start;
        if ((line_size == (strlen(tag_name) + tag_len)) &&
                        (!strncmp(&file_data[line_start], tag_name, strlen(tag_name))))
            return (&file_data[line_start + strlen(tag_name)]);
        line_start = curr + 1;
    }
    return NULL;
}
enum SF_EXPA_GPIO_MAP_TYPE {
    EXPA_GPIO_MAP_TYPE_1 = 1,
    EXPA_GPIO_MAP_TYPE_2 = 2,
    EXPA_GPIO_MAP_TYPE_MAX
};
static uint8_t sf_parser_value_from_cfg_file(const struct firmware *config_fw, const char *str)
{
    const u8 *tag_ptr;
    uint32_t value;
    tag_ptr = find_tag_from_file(config_fw->data, config_fw->size, str, strlen("0x00"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &value) != 1)
            value = 0xF0;
    } else
        value = 0xF0;
    printk("get %s=%02x\n", str, (uint8_t)(value & 0xFF));
    return (uint8_t)(value & 0xFF);
}
static int sf_rf_parse_expa_configfile(struct platform_device *pdev, struct rf_pl_context *priv,
                const char *filename)
{
    const struct firmware *config_fw;
    int ret;
    if ((ret = request_firmware(&config_fw, filename, &(pdev->dev)))) {
        printk(KERN_CRIT "%s: Failed to get %s (%d)\n", __func__, filename, ret);
        return ret;
    }
    priv->ex_pa_config.lb1_idle_cfg = sf_parser_value_from_cfg_file(config_fw, "LB1_EXTERNAL_PA_CONF_IN_IDLE=");
    priv->ex_pa_config.lb1_rx_cfg = sf_parser_value_from_cfg_file(config_fw, "LB1_EXTERNAL_PA_CONF_IN_RX=");
    priv->ex_pa_config.lb1_tx_cfg = sf_parser_value_from_cfg_file(config_fw, "LB1_EXTERNAL_PA_CONF_IN_TX=");
    priv->ex_pa_config.lb1_pa_cfg = sf_parser_value_from_cfg_file(config_fw, "LB1_EXTERNAL_PA_CONF_IN_PA=");
    priv->ex_pa_config.lb2_idle_cfg = sf_parser_value_from_cfg_file(config_fw, "LB2_EXTERNAL_PA_CONF_IN_IDLE=");
    priv->ex_pa_config.lb2_rx_cfg = sf_parser_value_from_cfg_file(config_fw, "LB2_EXTERNAL_PA_CONF_IN_RX=");
    priv->ex_pa_config.lb2_tx_cfg = sf_parser_value_from_cfg_file(config_fw, "LB2_EXTERNAL_PA_CONF_IN_TX=");
    priv->ex_pa_config.lb2_pa_cfg = sf_parser_value_from_cfg_file(config_fw, "LB2_EXTERNAL_PA_CONF_IN_PA=");
    priv->ex_pa_config.hb1_idle_cfg = sf_parser_value_from_cfg_file(config_fw, "HB1_EXTERNAL_PA_CONF_IN_IDLE=");
    priv->ex_pa_config.hb1_rx_cfg = sf_parser_value_from_cfg_file(config_fw, "HB1_EXTERNAL_PA_CONF_IN_RX=");
    priv->ex_pa_config.hb1_tx_cfg = sf_parser_value_from_cfg_file(config_fw, "HB1_EXTERNAL_PA_CONF_IN_TX=");
    priv->ex_pa_config.hb1_pa_cfg = sf_parser_value_from_cfg_file(config_fw, "HB1_EXTERNAL_PA_CONF_IN_PA=");
    priv->ex_pa_config.hb2_idle_cfg = sf_parser_value_from_cfg_file(config_fw, "HB2_EXTERNAL_PA_CONF_IN_IDLE=");
    priv->ex_pa_config.hb2_rx_cfg = sf_parser_value_from_cfg_file(config_fw, "HB2_EXTERNAL_PA_CONF_IN_RX=");
    priv->ex_pa_config.hb2_tx_cfg = sf_parser_value_from_cfg_file(config_fw, "HB2_EXTERNAL_PA_CONF_IN_TX=");
    priv->ex_pa_config.hb2_pa_cfg = sf_parser_value_from_cfg_file(config_fw, "HB2_EXTERNAL_PA_CONF_IN_PA=");
    priv->ex_pa_config.hb_rx_step2_value = sf_parser_value_from_cfg_file(config_fw,"HB_RX_STEP2_VALUE=");
    priv->ex_pa_config.hb_rx_step2_delay = sf_parser_value_from_cfg_file(config_fw, "HB_RX_STEP2_DELAY=");
    priv->ex_pa_config.hb_tx_step2_delay = sf_parser_value_from_cfg_file(config_fw, "HB_TX_STEP2_DELAY=");
    release_firmware(config_fw);
    return 0;
}
static int sf_wifi_rf_get_trx_path_cfg(struct platform_device *pdev, struct rf_pl_context *priv)
{
    const u8 *tag_ptr;
    const struct firmware *config_fw;
    struct file *fp=NULL;
    loff_t pos;
    char *trx_path_file = "/tmp/rf_trx_path.ini";
    u8 buf[20] = "0";
    int ret;
    uint32_t value;
    priv->trx_path_cfg = 0xFF;
    fp=filp_open(trx_path_file, O_RDWR, 0);
    if(!IS_ERR(fp)){
        pos = fp->f_pos;
        kernel_read(fp, buf, sizeof(buf),&pos);
        filp_close(fp,0);
        tag_ptr = find_tag_from_file(buf, sizeof(buf), "TRX_PATH_CFG=", strlen("0x00"));
        if (tag_ptr != NULL) {
            if (sscanf(tag_ptr, "%08x", &value) != 1)
                value = 0xF0;
        } else
            value = 0xFF;
        printk("get  %s=%02x\n","TRX_PATH_CFG=", (uint8_t)(value & 0xFF));
        priv->trx_path_cfg= (uint8_t)(value & 0xFF);
        return 0;
    }
    if ((ret = request_firmware(&config_fw, SF_RF_TRX_PATH_CONFIG, &(pdev->dev)))) {
        printk(KERN_CRIT "%s: Failed to get %s (%d)\n", __func__, SF_RF_TRX_PATH_CONFIG, ret);
        return ret;
    }
    priv->trx_path_cfg = sf_parser_value_from_cfg_file(config_fw, "TRX_PATH_CFG=");
    release_firmware(config_fw);
    return 0;
}
static int sf_wifi_rf_ex_pa_check(struct platform_device *pdev, struct rf_pl_context *priv)
{
    int gpio_num, ret, level;
    int force_expa = 0;
    bool gpio_requested = false;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    unsigned int dual_antenna_calibrate = 0;
#endif
#ifdef CONFIG_SFAX8_FACTORY_READ
    char buf[2];
#endif
    priv->lb_ex_pa_exist = 0;
    priv->hb_ex_pa_exist = 0;
    gpio_num = 0;
#ifdef CONFIG_SFAX8_FACTORY_READ
    if (!sf_get_value_from_factory(READ_WIFI_VERSION, buf, WIFI_VERSION_SIZE)){
        if(((buf[0] == 'V')&& (buf[1] == '3' || buf[1] == '2')) || (buf[0] == 'X' && buf[1] == 'O'))
            priv->cali_table_version = 1;
  else
   priv->cali_table_version = 2;
    }
    else{
        priv->cali_table_version = 2;
    }
#else
    priv->cali_table_version = 2;
#endif
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    priv->dual_antenna_calibrate = 0;
    if (of_property_read_u32(pdev->dev.of_node, "dual-antenna-calibrate", &dual_antenna_calibrate)){
        printk("can not get dual-antenna-calibrate from dts\n");
    } else if((dual_antenna_calibrate > 0) && (priv->cali_table_version != 1)){
        printk("dual antenna calibration is on\n");
        priv->dual_antenna_calibrate = 1;
    } else {
        printk("dual antenna calibration is off\n");
    }
#ifdef CONFIG_SFAX8_FACTORY_READ
    printk("priv->cali_table_version = %d---%c%c dual_antenna_calibrate = %d  \n", priv->cali_table_version, buf[0],
           buf[1], priv->dual_antenna_calibrate);
#endif
#endif
    if (of_property_read_u32(pdev->dev.of_node, "force_expa", &force_expa)) {
        force_expa = 0;
    }
    if (force_expa != 0) {
        level = 0;
        goto INIT_EXPA;
    }
    gpio_num = of_get_named_gpio(pdev->dev.of_node, "gpio-expa", 0);
    if (gpio_num < 0) {
        printk("gpio %d,of_get_named_gpio failed! Do not support external PA\n", gpio_num);
        return 0;
    }
    ret = gpio_request(gpio_num, NULL);
    if (ret == -16) {
        printk("gpio %d is busy, which is used by other device\n", gpio_num);
        return 0;
    } else if (ret < 0) {
        printk("error ret = %d: gpio_request gpio %u is failed!\n", ret, gpio_num);
        return ret;
    }
    gpio_requested = true;
    ret = gpio_direction_input((unsigned)gpio_num);
    if (ret) {
        printk("error %d: gpio_direction_input failed!\n", ret);
        gpio_free(gpio_num);
        return ret;
    }
    level = gpio_get_value((unsigned)gpio_num);
INIT_EXPA:
    if (!level) {
#ifdef CONFIG_SF16A18_WIFI_LB_EXT_PA_ENABLE
        priv->lb_ex_pa_exist = 1;
#else
        priv->lb_ex_pa_exist = 0;
#endif
#ifdef CONFIG_SF16A18_WIFI_HB_EXT_PA_ENABLE
        priv->hb_ex_pa_exist = 1;
#else
        priv->hb_ex_pa_exist = 0;
#endif
        ret = sf_rf_parse_expa_configfile(pdev, priv, SF_RF_EXPA_CONFIG);
        if (ret) {
            printk("error %d!!!,can not get config from sf_rf_expa_config.ini, so use default value\n", ret);
            priv->ex_pa_config.lb1_idle_cfg = 0x00;
            priv->ex_pa_config.lb1_rx_cfg = 0x06;
            priv->ex_pa_config.lb1_tx_cfg = 0x04;
            priv->ex_pa_config.lb1_pa_cfg = 0x01;
            priv->ex_pa_config.lb2_idle_cfg = 0x00;
            priv->ex_pa_config.lb2_rx_cfg = 0x06;
            priv->ex_pa_config.lb2_tx_cfg = 0x04;
            priv->ex_pa_config.lb2_pa_cfg = 0x01;
            priv->ex_pa_config.hb1_idle_cfg = 0x00;
            priv->ex_pa_config.hb1_rx_cfg = 0x06;
            priv->ex_pa_config.hb1_tx_cfg = 0x04;
            priv->ex_pa_config.hb1_pa_cfg = 0x01;
            priv->ex_pa_config.hb2_idle_cfg = 0x00;
            priv->ex_pa_config.hb2_rx_cfg = 0x06;
            priv->ex_pa_config.hb2_tx_cfg = 0x04;
            priv->ex_pa_config.hb2_pa_cfg = 0x01;
        }
    }
    if (gpio_requested)
        gpio_free(gpio_num);
    printk("gpio %d level is %d, hb_ex_pa_exist %d, lb_ex_pa_exist %d\n", gpio_num, level, priv->hb_ex_pa_exist, priv->lb_ex_pa_exist);
    return 0;
}
static int sf_wifi_rf_fw_load(struct platform_device *pdev, struct rf_pl_context *priv)
{
    int err = 0;
    const struct firmware *fw;
    int i = 0;
    uint16_t addr = 0;
    uint16_t value = 0;
    uint16_t *src;
    if (priv->fw_loaded) {
        printk("rf fw has been loaded!\n");
        return 0;
    }
    err = request_firmware(&fw, SF_RF_FW_NAME, &(pdev->dev));
    if (err) {
        printk("# Failed to get %s, with error: %d!\n", SF_RF_FW_NAME, err);
        return err;
    }
    WARN_ON(fw->size == 0);
    WARN_ON((fw->size % 2) != 0);
    printk("Now copy %s firmware with size %d, @ = 0x4002\n", SF_RF_FW_NAME, fw->size);
    value = ml_apb_read(MISC_DEBUG);
    ml_apb_write(MISC_DEBUG, (0xFFF7 & value));
    ml_apb_write(PROG_MEM_BASE + 1, 0x0003);
    ml_apb_write(PROG_MEM_BASE, 0);
    src = (uint16_t *)fw->data;
    for (i = 0; i < fw->size; i += 2)
        ml_apb_write(PROG_MEM_BASE + 2, *src++);
    release_firmware(fw);
    err = request_firmware(&fw, SF_RF_DEFAULT_REG_NAME, &(pdev->dev));
    if (err) {
        printk("# Failed to get %s, with error: %d!\n", SF_RF_DEFAULT_REG_NAME, err);
        return err;
    }
    WARN_ON(fw->size == 0);
    WARN_ON((fw->size % 4) != 0);
    printk("Now copy %s default reg with size %d\n", SF_RF_DEFAULT_REG_NAME, fw->size);
    src = (uint16_t *)fw->data;
    for (i = 0; i < fw->size; i += 4) {
        value = *src++;
        addr = *src++;
        ml_apb_write(addr, value);
    }
    release_firmware(fw);
#ifdef UPDATE_GAIN_TABLE
#endif
    priv->fw_loaded = 1;
    msleep(10);
    return 0;
}
int sf_wifi_rf_get_feature(int8_t band_type)
{
    int feature = 0;
    if(!g_rf_pl_ctx){
        printk("ERROR: g_rf_pl_ctx not inited yet\n");
        return 0;
    }
    if (g_rf_pl_ctx->hb_ex_pa_exist)
        feature |= RF_FEATURE_SUPPORT_EXTERNAL_PA_HB;
    if (g_rf_pl_ctx->lb_ex_pa_exist)
        feature |= RF_FEATURE_SUPPORT_EXTERNAL_PA_LB;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    if (g_rf_pl_ctx->dual_antenna_calibrate)
        feature |= RF_FEATURE_DUAL_ANTENNA_CALIBRATE;
#endif
#ifdef ENHANCE_LOWRATE_POWER
    if (!(g_rf_pl_ctx->hb_ex_pa_exist || g_rf_pl_ctx->lb_ex_pa_exist))
        feature |= RF_FEATURE_SUPPORT_ENHANCE_LOWRATE_POWER;
#endif
    return feature;
}
EXPORT_SYMBOL_GPL(sf_wifi_rf_get_feature);
static uint16_t sf_wifi_rf_cali_config_get_mtd(struct platform_device *pdev)
{
    uint16_t value = 0;
    struct device_node *mtd_np = NULL;
    struct mtd_info *mtd;
    size_t retlen;
    int size;
    const char *part;
    const __be32 *list;
    unsigned char ret[5] = {0};
    phandle phandle;
    list = of_get_property(pdev->dev.of_node, "mtd-rf-cali-config", &size);
    if (!list || (size != (2 * sizeof(*list)))) {
        printk("error! size %d, %d\n", size, sizeof(*list));
        return 0;
    }
    phandle = be32_to_cpup(list++);
    if (phandle)
        mtd_np = of_find_node_by_phandle(phandle);
    if (!mtd_np)
        return 0;
    part = of_get_property(mtd_np, "label", NULL);
    if (!part)
        part = mtd_np->name;
    mtd = get_mtd_device_nm(part);
    if (IS_ERR(mtd))
        return 0;
    if (mtd_read(mtd, be32_to_cpup(list), 4, &retlen, (u_char *)ret)) {
        printk("get config from mtd error\n");
        put_mtd_device(mtd);
        return 0;
    }
    if ((ret[0] == 'X' && ret[1] == 'O') || ((ret[0] == 'V') && (ret[1] == '2' || ret[1] =='3' || ret[1]=='4'))) {
        value = (ret[2] << 8) | ret[3];
    } else
        printk("Do not find XO cali value in flash,mark is %c%c\n", ret[0], ret[1]);
    put_mtd_device(mtd);
    return value;
}
static int sf_wifi_rf_cali_config_get(struct platform_device *pdev,struct rf_pl_context *priv)
{
    uint16_t value = 0;
#ifdef CONFIG_SFAX8_FACTORY_READ
    if (sf_get_value_from_factory(READ_RF_XO_CONFIG, &value, XO_CONFIG_SIZE)) {
        printk("get XO config from sf_factory_read failed\n");
#endif
        value = sf_wifi_rf_cali_config_get_mtd(pdev);
#ifdef CONFIG_SFAX8_FACTORY_READ
    }
#endif
    printk("XO config value : %x\n", value);
    priv->xo_config = value;
    return 0;
}
int sf_wifi_rf_bb_register(int8_t band_type, int (*callback)(void *data, uint32_t event, uint32_t flags, void *parameter), void *data)
{
    struct rf_pl_context *priv;
    int8_t i;
    int ret = 0;
    priv = g_rf_pl_ctx;
    if(priv == NULL){
        printk("rf driver must register before bb!\n");
        return -100;
    }
    mutex_lock(&priv->bb_mtx);
    if(band_type == CLIENT_LB){
        printk("lb registering.......\n");
        if(priv->lb_working){
            printk("lb has been registered!\n");
            goto DONE;
        }
    }else if(band_type == CLIENT_HB){
        printk("hb registering.......\n");
        if(priv->hb_working){
            printk("hb has been registered!\n");
            goto DONE;
        }
    }else{
        printk("error band type, %d\n", band_type);
        ret = -1;
        goto DONE;
    }
    if(!priv->fw_loaded){
        printk("rf firmware has not been loaded, please load the firmware before enable bb!\n");
        ret = -2;
        goto DONE;
    }
    for (i = 0; i < SUPPORT_MAX_CLIENT_NUM; i++) {
        if (priv->clients[i].band_type)
            continue;
        printk("find a empty client seat : %d\n", i);
        priv->clients[i].band_type = band_type;
        priv->clients[i].cb = callback;
        priv->clients[i].cb_data = data;
        if(band_type == CLIENT_LB)
            priv->lb_working = 1;
        else if(band_type == CLIENT_HB)
            priv->hb_working = 1;
        break;
    }
    if (i == SUPPORT_MAX_CLIENT_NUM) {
        printk("invalid client registered!\n");
        ret = -5;
        goto DONE;
    }
DONE:
    mutex_unlock(&priv->bb_mtx);
    return ret;
}
EXPORT_SYMBOL_GPL(sf_wifi_rf_bb_register);
int sf_wifi_rf_bb_unregister(int8_t band_type)
{
    struct rf_pl_context *priv;
    int i ;
    priv = g_rf_pl_ctx;
    if(priv == NULL){
        printk("rf driver must register before bb!\n");
        return -100;
    }
    mutex_lock(&priv->bb_mtx);
    for(i = 0 ;i < SUPPORT_MAX_CLIENT_NUM; i++){
        if(band_type == priv->clients[i].band_type){
            memset(&(priv->clients[i]), 0 , sizeof(struct rf_client_info));
            if(band_type == CLIENT_LB){
                priv->lb_working = 0;
            }else if(band_type == CLIENT_HB){
                priv->hb_working = 0;
            }
            break;
        }
    }
    if(!priv->lb_working && !priv->hb_working){
    }
    mutex_unlock(&priv->bb_mtx);
    WARN_ON(i == SUPPORT_MAX_CLIENT_NUM);
    return 0;
}
EXPORT_SYMBOL_GPL(sf_wifi_rf_bb_unregister);
int sf_wifi_rf_os_resources_free(struct platform_device *pdev, struct rf_pl_context *priv)
{
    if (priv->lp_clk_timer_enable) {
        del_timer_sync(&priv->lp_clk_timer);
    }
    cancel_delayed_work_sync(&priv->cali_work);
    mutex_destroy(&priv->bb_mtx);
    platform_set_drvdata(pdev, NULL);
    g_rf_pl_ctx = NULL;
    return 0;
}
int sf_wifi_rf_os_resources_get(struct platform_device *pdev, struct rf_pl_context **p_priv)
{
    struct rf_pl_context *priv;
    int ret;
    priv = devm_kzalloc(&pdev->dev, sizeof(struct rf_pl_context), GFP_KERNEL);
    if(!priv){
        printk("can not allocate memory!\n");
        return -ENOMEM;
    }
    priv->base = devm_platform_ioremap_resource(pdev, 0);
    if(IS_ERR(priv->base)) {
 return dev_err_probe(&pdev->dev, PTR_ERR(priv->base),
  "failed to remap memory region.\n");
    }
    ret = devm_clk_bulk_get_all(&pdev->dev, &priv->clks);
    if (ret < 3) {
        return dev_err_probe(&pdev->dev, ret, "failed to get all clocks\n");
        return ret;
    }
    priv->num_clks = ret;
    priv->rstc = devm_reset_control_get(&pdev->dev, NULL);
    if (IS_ERR(priv->rstc)) {
        return PTR_ERR(priv->rstc);
    }
    priv->irq = platform_get_irq(pdev, 0);
    if(priv->irq == -ENXIO){
        printk("can not get the irq from system!\n");
        return -ENXIO;
    }
    rf_set_access_base(priv->base);
    mutex_init(&priv->bb_mtx);
    spin_lock_init(&priv->cali_lock);
    INIT_DELAYED_WORK(&priv->cali_work, sf_wifi_rf_calibration_work);
    priv->enable_recali = 1;
#ifdef COOLING_TEMP
    priv->cooling_temp_set.thermal_on = thermal_on;
 priv->cooling_temp_set.flag = true;
#endif
    printk("%s:\n", __func__);
    printk(" priv->base : %px\n", priv->base);
    printk(" priv->irq : %d\n", priv->irq);
    *p_priv = priv;
    priv->dev = &pdev->dev;
    g_rf_pl_ctx = priv;
    platform_set_drvdata(pdev, priv);
    return 0;
}
#ifdef CONFIG_SF16A18_RF_SYSFS_DEBUG
void sf_wifi_rf_force_calibrate(uint32_t type)
{
    struct rf_pl_context *priv;
    unsigned long flags;
    priv = g_rf_pl_ctx;
    if (!priv->booted) {
        printk("have not boot up, do not support force calibrate!\n");
        return ;
    }
    spin_lock_irqsave(&priv->cali_lock, flags);
    if (type == 100) {
        priv->enable_recali = 1;
        goto DONE;
    } else if(type == 200) {
        priv->enable_recali = 0;
        goto DONE;
    }
    if(priv->calibrating){
        printk("rf calibrating!\n");
        goto DONE;
    }
    if (!priv->irq_source) {
        printk("%s, type : %d\n", __func__, type);
        ml_apb_read(IRQ_SOURCE);
        ml_apb_write(HB_IRQ_ENABLE, 0);
        ml_apb_write(LB_IRQ_ENABLE, 0);
        priv->irq_source = (unsigned long)type;
        schedule_delayed_work(&priv->cali_work, 0);
    }
DONE:
    spin_unlock_irqrestore(&priv->cali_lock, flags);
}
#endif
#ifdef COOLING_TEMP
#define MAX_STATE 31
#define HB_EXTERNAL_PA_MAX_GAIN 16
#define LB_EXTERNAL_PA_MAX_GAIN 20
#define HB_EXTERNAL_PA_STATE1_GAIN 4
#define LB_EXTERNAL_PA_STATE1_GAIN 6
#define HB_EXTERNAL_PA_STATE2_GAIN 2
#define LB_EXTERNAL_PA_STATE2_GAIN 4
#define HB_EXTERNAL_PA_STATE3_GAIN 0
#define LB_EXTERNAL_PA_STATE3_GAIN 1
#define MAX_GAIN 31
#define STATE1_GAIN 21
#define STATE2_GAIN 15
#define HB_STATE3_GAIN 11
#define LB_STATE3_GAIN 1
int thermal_state = -1;
enum trend {
    TREND_DROPPING = -1,
    TREND_STABLE = 0,
    TREND_RAISING = 1,
};
static int sf_wifi_rf_get_trig_temperature(void)
{
    struct device_node *np, *gnp, *child, *gchild;
    int temp,ret,child_num = 0;
    struct rf_cooling_temp_set *temp_set;
    np = of_find_node_by_name(NULL, "thermal-zones");
    if (!np) {
        printk("unable to find thermal zones\n");
        return 0;
    }
    gnp = of_get_next_child(np, NULL);
    child = of_get_child_by_name(gnp, "trips");
    if (!child) {
        printk("unable to find trips\n");
        return 0;
    }
    if (g_rf_pl_ctx) {
        temp_set= &(g_rf_pl_ctx->cooling_temp_set);
        for_each_child_of_node(child, gchild) {
            child_num++ ;
            if (temp_set->cool_state == 0)
                break;
            else if (child_num == temp_set->cool_state)
                break;
        }
    }
    else {
        gchild = of_get_child_by_name(child, "rf-alert0");
    }
    if (!gchild) {
        printk("unable to find rf-alert0\n");
        return 0;
    }
    ret = of_property_read_u32(gchild, "temperature", &temp);
    if (ret < 0) {
        printk("missing temperature property\n");
        return 0;
    }
    return (temp / 1000);
}
static int rf_thermal_sensors_get_temperature(void *data, int *temp)
{
    struct sfax8_rf_thermal_state *rs = data;
    if (!temp)
        return 0;
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
    if (ate_aetset)
        return 0;
#endif
    rs->temp_last = rs->temp;
    rs->temp = rf_get_temperature(1);
    if (rs->temp <= TEMP_ABNORMAL) {
        *temp = 0;
        return 0;
    }
    *temp = (rs->temp - rs->relative_diff) * 1000;
    return 0;
}
static int rf_thermal_sensors_get_trend(void *data,enum thermal_trend *trend)
{
    struct sfax8_rf_thermal_state *rs = data;
    struct rf_pl_context *pl_ctx = g_rf_pl_ctx;
    long ttrend;
    if (!trend)
        return 0;
    *trend = THERMAL_TREND_STABLE;
    if ((!rs) || (rs->temp <= TEMP_ABNORMAL) || (rs->temp_last <= TEMP_ABNORMAL) || (!pl_ctx->cooling_temp_set.flag))
        return 0;
    pl_ctx->cooling_temp_set.trig_temp = sf_wifi_rf_get_trig_temperature();
    ttrend = rs->temp - rs->temp_last;
    if ((ttrend < 0) && ((rs->temp - rs->relative_diff) >= pl_ctx->cooling_temp_set.trig_temp)){
            *trend = THERMAL_TREND_STABLE;
        return 0;
    }
    else if (rs->state && ((rs->temp - rs->relative_diff) < pl_ctx->cooling_temp_set.trig_temp)){
        *trend = THERMAL_TREND_DROPPING;
        return 0;
    }
    if (ttrend > 0)
        *trend = THERMAL_TREND_RAISING;
    else if(ttrend < 0)
        *trend = THERMAL_TREND_DROPPING;
    else
        *trend = THERMAL_TREND_STABLE;
    return 0;
}
static int sfax8_rf_get_max_state(void *data, unsigned long *state)
{
    *state = MAX_STATE;
    return 0;
}
static int sfax8_rf_get_cur_state(void *data,unsigned long *state)
{
    struct sfax8_rf_thermal_state *rs = data;
    *state = rs->state;
    return 0;
}
#define DEFAULT_THERMAL_STATE 12
#define DEFAULT_THERMAL_STATE_MAX 19
static int sfax8_get_thermal_change_power_from_cooling_state(unsigned long state,int band){
    int change_power=0;
    int change_power_trend;
    struct rf_pl_context *pl_ctx = g_rf_pl_ctx;
    struct rf_cooling_temp_set *temp_set = &(pl_ctx->cooling_temp_set);
    int states = (int)state;
    int default_thermal_state = DEFAULT_THERMAL_STATE;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    int hb_base_gain = RF_HB_BASE_GAIN;
    int hb_base_gain_diff = RF_HB_BASE_GAIN_DIFF;
#endif
    if(thermal_state > 0 )
        default_thermal_state = thermal_state;
    if(band == 0){
        if (states > DEFAULT_THERMAL_STATE_MAX){
            states = DEFAULT_THERMAL_STATE_MAX;
        }
        if (state >= default_thermal_state){
            change_power_trend = 1;
            change_power=states - default_thermal_state;
        }
        else{
            change_power_trend = -1;
            change_power= default_thermal_state - states;
        }
#ifdef ENABLE_DIGGAIN_TABLE
        if ( change_power %6 <= 2){
            change_power = (int)change_power/6;
        }
        else{
            change_power = ((int)change_power/6 + 1) | 0x80;
        }
        temp_set->change_power_trend[band] = change_power_trend;
#else
            change_power = (int)change_power/6;
        temp_set->change_power_trend[band] = change_power_trend;
#endif
        return change_power;
    }
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    if ( pl_ctx->dual_antenna_calibrate){
        if((states < 7) && !(temp_set->hb_low_power_state) ){
            if(hb_base_gain >= hb_base_gain_diff){
                rf_set_property(44,(hb_base_gain - hb_base_gain_diff));
                printk("set base gain %d state = %d\n",rf_get_property(44),states);
                temp_set->hb_low_power_state = 1;
            }
            else{
                printk("error state or gain setting !!!!\n");
            }
        }
        else if ((states >= 7) && temp_set->hb_low_power_state == 1){
            rf_set_property(44,(rf_get_property(44) + hb_base_gain_diff));
            printk("set base gain %d state = %d\n",rf_get_property(44),states);
            temp_set->hb_low_power_state = 0;
        }
        if (temp_set->hb_low_power_state == 1){
            default_thermal_state -= 6;
        }
        if(default_thermal_state < 0 )
            default_thermal_state = 0;
    }
#endif
    if (states > DEFAULT_THERMAL_STATE_MAX){
        states = DEFAULT_THERMAL_STATE_MAX;
    }
    if (state >= default_thermal_state){
        change_power_trend = 1;
        change_power=states - default_thermal_state;
    }
    else{
        change_power_trend = -1;
        change_power= default_thermal_state - states;
    }
#ifndef ENABLE_DIGGAIN_TABLE
    if (change_power%2 == 0 ){
        change_power = change_power/2;
    }
    else {
        change_power = ((change_power-1)/2) ;
    }
    temp_set->change_power_trend[band] = change_power_trend;
#else
    if (change_power%2 == 0 ){
        change_power = change_power/2;
    }
    else {
        change_power = ((change_power+1)/2) | 0x80;
    }
    temp_set->change_power_trend[band] = change_power_trend;
#endif
    return change_power;
}
static int sfax8_rf_set_cur_state(void *data,unsigned long state)
{
    int8_t ret = 0;
    int8_t i = 0;
    struct sfax8_rf_thermal_state *rs = data;
    struct rf_pl_context *pl_ctx = g_rf_pl_ctx;
    struct rf_cooling_temp_set *temp_set = &(pl_ctx->cooling_temp_set);
    int change_power = 0;
    int change_power_lb = 0;
 if (state > 24)
  return 0;
    change_power = sfax8_get_thermal_change_power_from_cooling_state(state,1);
    change_power_lb = sfax8_get_thermal_change_power_from_cooling_state(state,0);
    rs->state = state;
    pl_ctx->rf_thermal_state.count++;
    if (pl_ctx->calibrating || (!temp_set->flag)){
        return 0;
    }
    temp_set->cool_state = state;
    mutex_lock(&pl_ctx->bb_mtx);
    for (i = 0; i < SUPPORT_MAX_CLIENT_NUM; i++) {
        if (pl_ctx->clients[i].band_type) {
#ifdef CONFIG_SIWIFI_TEMPERATURE_CONTROL
            temp_set->cur_temp = rs->temp;
            ret = pl_ctx->clients[i].cb(pl_ctx->clients[i].cb_data, RF_EVENT_TEMP_CTL, 0, temp_set);
#endif
            if (i == CLIENT_LB - 1)
                temp_set->change_power[i] = change_power_lb;
            else if (i == CLIENT_HB - 1)
                temp_set->change_power[i] = change_power;
            else
                temp_set->change_power[i] = 0;
            ret = pl_ctx->clients[i].cb(pl_ctx->clients[i].cb_data, RF_EVENT_COOLING_TEMP, 0, temp_set);
        }
        if (ret) {
            mutex_unlock(&pl_ctx->bb_mtx);
            return -1;
        }
    }
    mutex_unlock(&pl_ctx->bb_mtx);
    return 0;
}
static struct sfax8_zone_device_ops sfax8_rf_zone_ops = {
    .get_temp = rf_thermal_sensors_get_temperature,
    .get_trend = rf_thermal_sensors_get_trend,
};
static struct sfax8_cooling_device_ops sfax8_rf_thermal_ops = {
    .get_max_state = sfax8_rf_get_max_state,
    .get_cur_state = sfax8_rf_get_cur_state,
    .set_cur_state = sfax8_rf_set_cur_state,
};
int sf_wifi_set_thermal_state(void)
{
#ifdef CONFIG_SFAX8_FACTORY_READ
    char cool_temp[COOLING_TEMP_SIZE] = "00";
    int tmp_thermal_state;
    if (sf_get_value_from_factory(READ_COOLING_TEMP, &cool_temp, COOLING_TEMP_SIZE)){
        printk("get cooling temp from sf_factory_read failed\n");
        return -1;
    }else
        printk("read cooling temp(0x%x%x) from factory",
                cool_temp[0], cool_temp[1]);
    if (cool_temp[0] > 100){
        printk("factory temp is abnormal!");
            return 0;
    }
    else{
  if(cool_temp[1]==1)
  {
    printk("factory store temp %d below zero ",cool_temp[0]);
    cool_temp[0] = ~cool_temp[0] - 255;
    printk("cool_temp[0] is %d",cool_temp[0]);
  }
       tmp_thermal_state = (cool_temp[0]+20)/5 +1;
    }
    thermal_state = tmp_thermal_state;
    printk("set thermal_state : %d",thermal_state);
    return 0;
#else
    printk("Can not get cooling temp due to CONFIG_SFAX8_FACTORY_READ is not set\n");
    return -1;
#endif
}
#endif
int sf_wifi_rf_probe(struct platform_device *pdev)
{
    struct rf_pl_context *priv;
    int ret;
    int i;
    if((ret = sf_wifi_rf_os_resources_get(pdev, &priv))){
        printk("sf_wifi_rf_os_resources_get failed, %d!\n", ret);
        return ret;
    }
    ret = clk_bulk_prepare_enable(priv->num_clks, priv->clks);
    if (ret < 0) {
        dev_err(&pdev->dev, "failed to enable all clocks: %d\n", ret);
        goto error_release_resources;
    }
    dev_warn(&pdev->dev, "%s: %d\n", __FUNCTION__, __LINE__);
    reset_control_deassert(priv->rstc);
    if((ret = sf_wifi_rf_irqs_register(pdev, priv)))
    {
        printk("sf_wifi_rf_register_irqs failed, ret = %d!\n", ret);
        goto error_clk;
    }
    if ((ret = sf_wifi_rf_ex_pa_check(pdev, priv))) {
        printk("sf_wifi_rf_ex_pa_check failed, ret = %d!\n", ret);
        goto error_irqs_unregister;
    }
    if ((ret = sf_wifi_rf_fw_load(pdev, priv))) {
        printk("sf_wifi_rf_load_fw failed, ret = %d!\n", ret);
        goto error_irqs_unregister;
    }
    if ((ret = sf_wifi_rf_cali_config_get(pdev, priv))) {
        printk("sf_wifi_rf_cali_config_get failed, ret = %d!\n", ret);
    }
#define rf_BOOTUP_RETRY_MAX 20
    if ((ret = sf_wifi_rf_get_trx_path_cfg(pdev, priv))) {
        printk("sf_wifi_rf_get_trx_path_cfg failed, ret = %d!\n", ret);
    }
    i = 0;
    while (i < rf_BOOTUP_RETRY_MAX && rf_bootup()) {
        int temp;
        printk("rf_bootup failed! retry=%d\n", i);
        temp = rf_get_temperature(0);
        printk("temperature is %d\n", temp);
        sf_wifi_rf_irqs_unregister(pdev, priv);
        reset_control_assert(priv->rstc);
        priv->fw_loaded = 0;
        msleep(1);
        reset_control_deassert(priv->rstc);
        if ((ret = sf_wifi_rf_fw_load(pdev, priv))) {
            i++;
            continue;
        }
        if ((ret = sf_wifi_rf_irqs_register(pdev, priv))) {
            printk("sf_wifi_rf_register_irqs failed, ret = %d!\n", ret);
            goto error_release_resources;
        }
        i++;
        if (temp < 0)
            rfchip_hot(1000 * 1000 * 10);
    }
    if (i == rf_BOOTUP_RETRY_MAX) {
        ret = -4;
        goto error_irqs_unregister;
    }
    priv->booted = 1;
#ifdef COOLING_TEMP
    if (priv->cooling_temp_set.thermal_on) {
#ifdef CONFIG_SF16A18_WIFI_ATE_TOOLS
        ate_aetset = 0;
#endif
        printk("rf mod param thermal_on %d \n",thermal_on);
        if(sf_wifi_set_thermal_state()){
            printk("sf_wifi_set_thermal_state failed!\n");
        }
        priv->cooling_temp_set.dual_cali_enable = priv->dual_antenna_calibrate;
        printk("dual thermal %d\n",priv->cooling_temp_set.dual_cali_enable);
        if ((ret = sfax8_thermal_sensor_register(
                        &pdev->dev, &sfax8_rf_zone_ops, &(priv->rf_thermal_state)))) {
            printk("sfax8_thermal_sensor_register failed, ret = %d!\n", ret);
            goto error_irqs_unregister;
        }
        if ((ret = sfax8_cooling_device_register(&pdev->dev, &(priv->rf_thermal_state), &sfax8_rf_thermal_ops))) {
            printk("sfax8_cooling_device_register failed, ret = %d!\n", ret);
            goto error_irqs_unregister;
        }
    }
#endif
#ifdef CONFIG_SF16A18_RF_SYSFS_DEBUG
    if((ret = sf_wifi_rf_sysfs_register(pdev, "aetnensis"))){
        printk("sf_wifi_rf_sysfs_register failed, ret = %d!\n", ret);
        goto error_irqs_unregister;
    }
#endif
#ifdef UPDATE_GAIN_TABLE
 if(g_rf_pl_ctx->dual_antenna_calibrate){
  rf_update_hb_tx_gain_table();
  rf_update_gain_table(LB_TX_GAIN_TABLE_0, &rf_lb_gain_table[0],32);
  rf_update_gain_table(LB_TX_DBB_GAIN_TABLE_0, &rf_lb_dbb_gain_table[0],16);
 }
#endif
     return 0;
error_irqs_unregister:
    sf_wifi_rf_irqs_unregister(pdev, priv);
error_clk:
    clk_bulk_disable_unprepare(priv->num_clks, priv->clks);
error_release_resources:
    sf_wifi_rf_os_resources_free(pdev, priv);
    return ret;
}
void sf_wifi_rf_remove(struct platform_device *pdev)
{
    struct rf_pl_context *priv;
    printk("%s\n", __func__);
    priv = (struct rf_pl_context *)platform_get_drvdata(pdev);
#ifdef COOLING_TEMP
    sfax8_thermal_sensor_unregister(&pdev->dev);
    sfax8_cooling_device_unregister(&pdev->dev);
#endif
#ifdef CONFIG_SF16A18_RF_SYSFS_DEBUG
    sf_wifi_rf_sysfs_unregister(pdev);
#endif
    sf_wifi_rf_irqs_unregister(pdev, priv);
    reset_control_assert(priv->rstc);
    clk_bulk_disable_unprepare(priv->num_clks, priv->clks);
    sf_wifi_rf_os_resources_free(pdev, priv);
    return;
}
static const struct of_device_id sf_wifi_rf_of_match[] = {
    {
        .compatible = "siflower,sf19a2890-rf",
    },
    {},
};
static struct platform_driver sf_wifi_rf_driver = {
    .probe = sf_wifi_rf_probe,
    .remove = sf_wifi_rf_remove,
    .driver = {
        .name = SF_RF_MODULE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = sf_wifi_rf_of_match,
    },
};
static void __exit exit_sf_wifi_rf(void)
{
    platform_driver_unregister(&sf_wifi_rf_driver);
}
static int __init init_sf_wifi_rf(void)
{
    return platform_driver_register(&sf_wifi_rf_driver);
}
module_init(init_sf_wifi_rf);
module_exit(exit_sf_wifi_rf);
MODULE_DEVICE_TABLE(of, sf_wifi_rf_of_match);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Wireless rf drivers for sf16a28");
MODULE_VERSION(RF_VERSION);
