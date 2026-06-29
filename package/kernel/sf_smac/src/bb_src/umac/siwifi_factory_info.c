#include "siwifi_defs.h"
#include "siwifi_factory_info.h"
#include "siwifi_lmac_glue.h"
#include "siwifi_cfgfile.h"
#ifdef CONFIG_SFAX8_FACTORY_READ
#include <sfax8_factory_read.h>
#else
#define XO_CONFIG_SIZE 2
#define WIFI_INFO_SIZE 2046
#define WIFI_INFO_SIZE_V4 3380
#endif
#include "siwifi_mem.h"
static int sf_wifi_init_normal_txpower_list(
                struct siwifi_factory_info *factory_info,
                int lb,
                char *wifi_info_buf)
{
    if (lb) {
        factory_info->list_len = LB_LIST_LEN;
        factory_info->normal_txpower_list = (char *)siwifi_kzalloc(
                        factory_info->list_len, GFP_KERNEL);
        if (!factory_info->normal_txpower_list)
            return -1;
        memcpy(factory_info->normal_txpower_list,
                        wifi_info_buf + XO_CONFIG_SIZE,
                        factory_info->list_len);
    } else {
        factory_info->list_len = HB_LIST_LEN;
        factory_info->normal_txpower_list = (char *)siwifi_kzalloc(
                        factory_info->list_len, GFP_KERNEL);
        if (!factory_info->normal_txpower_list)
            return -1;
        memcpy(factory_info->normal_txpower_list,
                        wifi_info_buf + XO_CONFIG_SIZE + LB_LIST_LEN,
                        factory_info->list_len);
    }
    return 0;
}
static int sf_wifi_init_sleepmode_txpower_list(struct siwifi_factory_info *factory_info, int lb, struct siwifi_hw *siwifi_hw)
{
 factory_info->sleepmode_txpower_list = (char *)siwifi_kzalloc(factory_info->list_len, GFP_KERNEL);
 if (!factory_info->sleepmode_txpower_list)
  return -1;
 memcpy(factory_info->sleepmode_txpower_list,factory_info->normal_txpower_list,factory_info->list_len);
 siwifi_update_txpower_offset_configfile(factory_info->sleepmode_txpower_list, 0, lb, siwifi_hw, 0);
 return 0;
}
static int sf_wifi_init_low_txpower_list(struct siwifi_factory_info *factory_info, int lb, struct siwifi_hw *siwifi_hw)
{
 factory_info->low_txpower_list = (char *)siwifi_kzalloc(factory_info->list_len, GFP_KERNEL);
 if (!factory_info->low_txpower_list)
  return -1;
 memcpy(factory_info->low_txpower_list,factory_info->normal_txpower_list,factory_info->list_len);
 siwifi_update_txpower_offset_configfile(factory_info->low_txpower_list, 1, lb, siwifi_hw, 0);
    return 0;
}
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
static int sf_wifi_init_normal_txpower_list_second_antenna(
                struct siwifi_factory_info *factory_info,
                int lb,
                char *wifi_info_buf)
{
    if (lb) {
        factory_info->normal_txpower_list_second_antenna = (char *)siwifi_kzalloc(
                        factory_info->list_len, GFP_KERNEL);
        if (!factory_info->normal_txpower_list_second_antenna)
            return -1;
        memcpy(factory_info->normal_txpower_list_second_antenna,
                        wifi_info_buf + XO_CONFIG_SIZE + LB_LIST_LEN + HB_LIST_LEN,
                        factory_info->list_len);
    } else {
        factory_info->normal_txpower_list_second_antenna = (char *)siwifi_kzalloc(
                        factory_info->list_len, GFP_KERNEL);
        if (!factory_info->normal_txpower_list_second_antenna)
            return -1;
        memcpy(factory_info->normal_txpower_list_second_antenna,
                        wifi_info_buf + XO_CONFIG_SIZE + LB_LIST_LEN + LB_LIST_LEN + HB_LIST_LEN,
                        factory_info->list_len);
    }
    return 0;
}
static int sf_wifi_init_sleepmode_txpower_list_second_antenna(struct siwifi_factory_info *factory_info, int lb, struct siwifi_hw *siwifi_hw)
{
 factory_info->sleepmode_txpower_list_second_antenna = (char *)siwifi_kzalloc(factory_info->list_len, GFP_KERNEL);
 if (!factory_info->sleepmode_txpower_list)
  return -1;
 memcpy(factory_info->sleepmode_txpower_list_second_antenna,factory_info->normal_txpower_list_second_antenna,factory_info->list_len);
 siwifi_update_txpower_offset_configfile(factory_info->sleepmode_txpower_list_second_antenna, 0, lb, siwifi_hw, 1);
 return 0;
}
static int sf_wifi_init_low_txpower_list_second_antenna(struct siwifi_factory_info *factory_info, int lb, struct siwifi_hw *siwifi_hw)
{
 factory_info->low_txpower_list_second_antenna = (char *)siwifi_kzalloc(factory_info->list_len, GFP_KERNEL);
 if (!factory_info->low_txpower_list_second_antenna)
  return -1;
 memcpy(factory_info->low_txpower_list_second_antenna,factory_info->normal_txpower_list_second_antenna,factory_info->list_len);
 siwifi_update_txpower_offset_configfile(factory_info->low_txpower_list_second_antenna, 1, lb, siwifi_hw, 1);
 return 0;
}
#endif
static int sf_wifi_init_high_txpower_list(
                struct siwifi_factory_info *factory_info,
                int lb,
                char *wifi_info_buf)
{
    int i = 0;
 int wrong_start_value = 0;
    if (lb) {
        char *lb_chan_gain = wifi_info_buf + XO_CONFIG_SIZE +
                             LB_LIST_LEN + HB_LIST_LEN;
        if (lb_chan_gain[0] != 24) {
            printk("the magic key of lb v2 more info is not "
                   "matched. %d vs 24\n",
                            lb_chan_gain[0]);
            wrong_start_value = 1;
        }
        factory_info->high_txpower_list = (char *)siwifi_kzalloc(
                        factory_info->list_len, GFP_KERNEL);
        if (!factory_info->high_txpower_list)
            goto err;
  if (wrong_start_value) {
    memset(factory_info->high_txpower_list, 0, factory_info->list_len);
  } else {
   for (i = 0; i <= LB_CHANNEL_COUNT; i++) {
    memcpy(factory_info->high_txpower_list + i * LB_ONE_CHAN_GAIN_NUM,
      lb_chan_gain + LB_V2_MAGIC_INFO_SIZE, LB_ONE_CHAN_GAIN_NUM);
   }
  }
    } else {
        char *hb_chan_gain = wifi_info_buf + XO_CONFIG_SIZE +
                             LB_LIST_LEN + HB_LIST_LEN + LB_V2_MORE_INFO_SIZE;
        if (hb_chan_gain[0] != 5) {
            printk("the magic key of hb v2 more info is not "
                   "matched. %d vs 5\n",
                            hb_chan_gain[0]);
             wrong_start_value = 1;
        }
        factory_info->high_txpower_list = (char *)siwifi_kzalloc(
                        factory_info->list_len, GFP_KERNEL);
        if (!factory_info->high_txpower_list)
            goto err;
  if (wrong_start_value) {
    memset(factory_info->high_txpower_list, 0, factory_info->list_len);
  }else{
   memcpy(factory_info->high_txpower_list,
      factory_info->normal_txpower_list,
      factory_info->list_len);
   for (i = 0; i <= 7; i++) {
    memcpy(factory_info->high_txpower_list + i * HB_ONE_CHAN_GAIN_NUM,
      hb_chan_gain + HB_V2_MAGIC_INFO_SIZE, HB_ONE_CHAN_GAIN_NUM);
   }
  }
    }
    return 0;
err:
    if (factory_info->high_txpower_list)
        siwifi_kfree(factory_info->high_txpower_list);
    return -1;
}
int sf_wifi_check_calibration_table_available(char *buf){
 if (buf[0] == 'X' && buf[1] == 'O')
        return 1;
    else if (buf[0] == 'V' && buf[1] == '2')
        return 2;
    else if (buf[0] == 'V' && buf[1] == '3')
        return 3;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    else if (buf[0] == 'V' && buf[1] == '4')
  return 4;
#endif
    else {
        return 0;
    }
}
int sf_wifi_init_wifi_factory_info(struct siwifi_hw *siwifi_hw)
{
    int ret = 0;
#ifdef CONFIG_SFAX8_FACTORY_READ
    char buf[WIFI_VERSION_SIZE];
#endif
    struct mpw0_plat_data *priv =
                    (struct mpw0_plat_data *)&siwifi_hw->plat->priv;
    int lb = priv->band & LB_MODULE;
    struct siwifi_factory_info *factory_info = &siwifi_hw->factory_info;
    char *wifi_info_buf = NULL;
    uint16_t xo_value = 0;
 unsigned char *txpower_table_value;
 u32 size_calitable = 4096;
 u32 wifi_info_size = WIFI_INFO_SIZE;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
 int dual_antenna_calibrate = 0;
#endif
 struct file *fp_read = NULL;
 char fisrt_txpower_table_file[] = "/usr/bin/txpower_calibrate_table.bin";
 char second_txpower_table_file[] = "/lib/firmware/default_txpower_calibrate_table.bin";
 struct phy_aetnensis_cfg_tag *phy_tag;
 char second_txpower_expa_table_file[] = "/lib/firmware/default_txpower_calibrate_expa_table.bin";
 phy_tag=(struct phy_aetnensis_cfg_tag *)&siwifi_hw->phy_config;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
 if ((phy_tag->flag & DUAL_ANTENNA_CALIBRATE)){
  dual_antenna_calibrate = 1;
  size_calitable = 5430;
  wifi_info_size = WIFI_INFO_SIZE_V4;
 }
#endif
 txpower_table_value = (unsigned char *)siwifi_kzalloc(size_calitable, GFP_ATOMIC);
 if(!txpower_table_value){
  printk("malloc txpower_table_value fail\n");
  goto err;
 }
 wifi_info_buf = (char *)siwifi_kzalloc(wifi_info_size, GFP_KERNEL);
 if (!wifi_info_buf) {
  printk("can not malloc wifi info buf\n");
  goto err;
 }
 fp_read = filp_open(fisrt_txpower_table_file, O_RDONLY , 0);
 if(!IS_ERR(fp_read)){
  memset(txpower_table_value, 0, size_calitable);
  kernel_read(fp_read, txpower_table_value, size_calitable, &fp_read->f_pos);
  filp_close(fp_read, NULL);
  factory_info->version = sf_wifi_check_calibration_table_available((char *)&txpower_table_value[2048]);
  if(factory_info->version == 0){
   printk("factory_info->version in txpower_calibrate_table.bin is wrong!\n");
  }
  else{
   printk("txpower calibration table use txpower_calibrate_table.bin\n");
   memcpy(wifi_info_buf, &txpower_table_value[2050], wifi_info_size);
   goto read_xo_value;
  }
 }
 else{
 }
#ifdef CONFIG_SFAX8_FACTORY_READ
    ret = sf_get_value_from_factory(
                    READ_WIFI_VERSION, buf, WIFI_VERSION_SIZE);
    if (ret) {
        factory_info->version = 0;
    } else {
        factory_info->version = sf_wifi_check_calibration_table_available(buf);
    }
 if (sf_get_value_from_factory(
                        READ_WIFI_INFO, wifi_info_buf, wifi_info_size)) {
        printk("can not get READ_LB_MORE_INFO from factory\n");
        factory_info->version = 0;
    }else{
  printk("txpower calibration table use factory info\n");
  goto read_xo_value;
 }
#else
    factory_info->version = 0;
#endif
 if(factory_info->version == 0){
  if ((phy_tag->flag & RF_EXTERNAL_PA_ENABLE)) {
   fp_read = filp_open(second_txpower_expa_table_file, O_RDONLY , 0);
  }
  else{
   fp_read = filp_open(second_txpower_table_file, O_RDONLY , 0);
  }
  if(!IS_ERR(fp_read)){
   memset(txpower_table_value, 0, size_calitable);
   kernel_read(fp_read, txpower_table_value, size_calitable, &fp_read->f_pos);
   filp_close(fp_read, NULL);
   factory_info->version = sf_wifi_check_calibration_table_available((char *)&txpower_table_value[2048]);
   if(factory_info->version == 0){
    printk("factory_info->version in default_txpower_calibrate_table.bin is wrong!\n");
    goto err;
   }
   else{
    memcpy(wifi_info_buf, &txpower_table_value[2050], wifi_info_size);
    printk("txpower calibration table use default_txpower_calibrate_table.bin\n");
   }
  }
  else{
   goto err;
  }
 }
read_xo_value:
#ifdef CONFIG_SFAX8_FACTORY_READ
 if (sf_get_value_from_factory(
                        READ_RF_XO_CONFIG, &xo_value, XO_CONFIG_SIZE)) {
#endif
  fp_read = filp_open(second_txpower_table_file, O_RDONLY , 0);
  if(!IS_ERR(fp_read)){
   memset(txpower_table_value, 0, size_calitable);
   kernel_read(fp_read, txpower_table_value, size_calitable, &fp_read->f_pos);
   filp_close(fp_read, NULL);
   factory_info->version = sf_wifi_check_calibration_table_available((char *)&txpower_table_value[2048]);
   if(factory_info->version == 0){
    goto err;
   }
   else{
    printk("get XO config from deautlt_txpower_calibrate.bin\n");
    memcpy(&xo_value, &txpower_table_value[2050], XO_CONFIG_SIZE);
   }
  }
  else{
   goto err;
  }
#ifdef CONFIG_SFAX8_FACTORY_READ
    }
#endif
 factory_info->xo_value = xo_value;
    switch (factory_info->version) {
    case 1:
    case 2:
    case 3:
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
 case 4:
#endif
        ret = sf_wifi_init_normal_txpower_list(
                        factory_info, lb, wifi_info_buf);
        if (ret)
            goto err;
  if(sf_wifi_init_sleepmode_txpower_list(factory_info, lb, siwifi_hw))
   goto err;
  if(sf_wifi_init_low_txpower_list(factory_info, lb, siwifi_hw))
   goto err;
        if ((factory_info->version == 2) || (factory_info->version == 3)) {
            ret = sf_wifi_init_high_txpower_list(
                            factory_info, lb, wifi_info_buf);
            if (ret)
                goto err;
        }
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
  if ((dual_antenna_calibrate == 1) && (factory_info->version == 4)){
   if(sf_wifi_init_normal_txpower_list_second_antenna(factory_info, lb, wifi_info_buf))
    goto err;
   if(sf_wifi_init_sleepmode_txpower_list_second_antenna(factory_info, lb, siwifi_hw))
    goto err;
   if(sf_wifi_init_low_txpower_list_second_antenna(factory_info, lb, siwifi_hw))
    goto err;
  }
#endif
  break;
    case 0:
    default:
        break;
    }
    siwifi_kfree(wifi_info_buf);
 siwifi_kfree(txpower_table_value);
    return 0;
err:
    sf_wifi_deinit_wifi_factory_info(siwifi_hw);
    if (wifi_info_buf){
        siwifi_kfree(wifi_info_buf);
 }
 if (txpower_table_value){
  siwifi_kfree(txpower_table_value);
 }
    return -1;
}
void sf_wifi_deinit_wifi_factory_info(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_factory_info *factory_info = &siwifi_hw->factory_info;
    if (factory_info->sleepmode_txpower_list)
        siwifi_kfree(factory_info->sleepmode_txpower_list);
    if (factory_info->low_txpower_list)
        siwifi_kfree(factory_info->low_txpower_list);
    if (factory_info->normal_txpower_list)
        siwifi_kfree(factory_info->normal_txpower_list);
    if (factory_info->high_txpower_list)
        siwifi_kfree(factory_info->high_txpower_list);
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
    if (factory_info->sleepmode_txpower_list_second_antenna)
        siwifi_kfree(factory_info->sleepmode_txpower_list_second_antenna);
    if (factory_info->low_txpower_list_second_antenna)
        siwifi_kfree(factory_info->low_txpower_list_second_antenna);
    if (factory_info->normal_txpower_list_second_antenna)
        siwifi_kfree(factory_info->normal_txpower_list_second_antenna);
#endif
    memset(&siwifi_hw->factory_info, 0, sizeof(struct siwifi_factory_info));
}
