#include "siwifi_frame.h"
#include "siwifi_mem.h"
#include "siwifi_msg_tx.h"
static int siwifi_ie_default(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_SUPP_RATES;
        variable[1] = SIWIFI_IE_SUPP_RATES_LEN;
        if (is_hb){
            variable[2] = 0x8c;
            variable[3] = 0x12;
            variable[4] = 0x98;
            variable[5] = 0x24;
            variable[6] = 0xb0;
            variable[7] = 0x48;
            variable[8] = 0x60;
            variable[9] = 0x6c;
        } else {
            variable[2] = 0x82;
            variable[3] = 0x84;
            variable[4] = 0x8B;
            variable[5] = 0x96;
            variable[6] = 0x0c;
            variable[7] = 0x12;
            variable[8] = 0x18;
            variable[9] = 0x24;
        }
        ie_len = (2 + SIWIFI_IE_SUPP_RATES_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_ie_ext_supp_rates(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_EXT_SUPP_RATES;
        variable[1] = SIWIFI_IE_EXT_SUPP_RATES_LEN;
        variable[2] = 0x30;
        variable[3] = 0x48;
        variable[4] = 0x60;
        variable[5] = 0x6c;
        ie_len = (2 + SIWIFI_IE_EXT_SUPP_RATES_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_supported_channels(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_SUPPORTED_CHANNELS;
        variable[1] = SIWIFI_IE_SUPPORTED_CHANNELS_LEN;
        variable[2] = 0;
        variable[3] = 0;
        ie_len += (2 + SIWIFI_IE_SUPPORTED_CHANNELS_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_country(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
            variable[0] = WLAN_EID_COUNTRY;
            if (is_hb) {
            variable[1] = SIWIFI_IE_COUNTRY_LEN;
            variable[2] = 'C';
            variable[3] = 'N';
            variable[4] = 0x20;
            variable[5] = 36;
            variable[6] = 4;
            variable[7] = 23;
            variable[8] = 100;
            variable[9] = 11;
            variable[10] = 24;
            variable[11] = 149;
            variable[12] = 5;
            variable[13] = 30;
            ie_len = (2 + SIWIFI_IE_COUNTRY_LEN);
        } else {
            variable[1] = SIWIFI_IE_LB_COUNTRY_LEN;
            variable[2] = 'C';
            variable[3] = 'N';
            variable[4] = 0x20;
            variable[5] = 1;
            variable[6] = 13;
            variable[7] = 20;
            ie_len = (2 + SIWIFI_IE_LB_COUNTRY_LEN);
        }
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_qbss(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_QBSS_LOAD;
        variable[1] = SIWIFI_IE_QBSS_LEN;
        variable[2] = 0;
        variable[3] = 0;
        variable[4] = 0x1;
        variable[5] = 0;
        variable[6] = 0;
        ie_len = (2 + SIWIFI_IE_QBSS_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_supp_reg(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_SUPPORTED_REGULATORY_CLASSES;
        if (is_hb) {
            variable[1] = SIWIFI_IE_SUPPORTED_REGULATORY_LEN;
            variable[2] = 0x7c;
            variable[3] = 0x73;
            variable[4] = 0x74;
            variable[5] = 0x75;
            variable[6] = 0x79;
            variable[7] = 0x7a;
            variable[8] = 0x7b;
            variable[9] = 0x7c;
            variable[10] = 0x7d;
            variable[11] = 0x7e;
            variable[12] = 0x7f;
            variable[13] = 0x80;
            variable[14] = 0x82;
            ie_len = (2 + SIWIFI_IE_SUPPORTED_REGULATORY_LEN);
        } else {
            variable[1] = SIWIFI_IE_LB_SUPPORTED_REGULATORY_LEN;
            variable[2] = 0x51;
            variable[3] = 0x51;
            variable[4] = 0x53;
            variable[5] = 0x54;
            ie_len = (2 + SIWIFI_IE_LB_SUPPORTED_REGULATORY_LEN);
        }
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_custom_supp_reg(uint8_t *variable, int is_hb)
{
    int ie_len = SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2 + 2;
    uint8_t custom_ie[SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2 + 2];
    custom_ie[0] = WLAN_EID_SUPPORTED_REGULATORY_CLASSES;
    custom_ie[1] = SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2;
    if (is_hb)
        custom_ie[2] = 0x80;
    else
        custom_ie[2] = 0x51;
    custom_ie[3] = 0;
    return siwifi_fill_ie_supp_reg(variable, custom_ie, ie_len, is_hb);
}
static int siwifi_fill_ie_ht_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_HT_CAPABILITY;
        variable[1] = SIWIFI_IE_HT_CAPABILITY_LEN;
        variable[2] = 0xff;
        if (is_hb)
            variable[3] = 0x9;
        else
            variable[3] = 0x1;
        variable[4] = 0x1f;
        variable[5] = 0xff;
        variable[6] = 0xff;
        variable[7] = 0;
        variable[8] = 0;
        variable[9] = 0;
        variable[10] = 0;
        variable[11] = 0;
        variable[12] = 0;
        variable[13] = 0;
        variable[14] = 0;
        variable[15] = 0;
        variable[16] = 0;
        variable[17] = 0;
        variable[18] = 0;
        variable[19] = 0;
        variable[20] = 0;
        variable[21] = 0;
        variable[22] = 0;
        variable[23] = 0;
        variable[24] = 0;
        variable[25] = 0;
        variable[26] = 0;
        variable[27] = 0;
        ie_len = (2 + SIWIFI_IE_HT_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_ext_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_EXT_CAPABILITY;
        variable[1] = SIWIFI_IE_EXT_CAPABILITY_LEN;
        variable[2] = 4;
        variable[3] = 0;
        variable[4] = 0;
        variable[5] = 2;
        variable[6] = 0;
        variable[7] = 0;
        variable[8] = 1;
        variable[9] = 0x40;
        ie_len = (2 + SIWIFI_IE_EXT_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
extern uint16_t siwifi_freq_to_channel(uint16_t freq);
static int siwifi_fill_ie_dsps(struct siwifi_vif *siwifi_vif, uint8_t *variable)
{
    int ie_len = 0;
    uint8_t channel_num = siwifi_freq_to_channel(siwifi_vif->siwifi_hw->chanctx_table[siwifi_vif->ch_index].chan_def.chan->center_freq);
    variable[0] = WLAN_EID_DS_PARAMS;
    variable[1] = SIWIFI_IE_DSPS_LEN;
    variable[2] = channel_num;
    ie_len = (2 + SIWIFI_IE_DSPS_LEN);
    return ie_len;
}
static int siwifi_fill_ie_ht_operation(struct siwifi_vif *siwifi_vif, uint8_t *variable)
{
    int ie_len = 0;
    uint8_t channel_num = siwifi_freq_to_channel(siwifi_vif->siwifi_hw->chanctx_table[siwifi_vif->ch_index].chan_def.chan->center_freq);
    variable[0] = WLAN_EID_HT_OPERATION;
    variable[1] = SIWIFI_IE_HT_OPERATION_LEN;
    variable[2] = channel_num;
    variable[3] = 0x7;
    variable[4] = 0;
    variable[5] = 0;
    variable[6] = 0;
    variable[7] = 0;
    variable[8] = 0;
    variable[9] = 0;
    variable[10] = 0;
    variable[11] = 0;
    variable[12] = 0;
    variable[13] = 0;
    variable[14] = 0;
    variable[15] = 0;
    variable[16] = 0;
    variable[17] = 0;
    variable[18] = 0;
    variable[19] = 0;
    variable[20] = 0;
    variable[11] = 0;
    variable[22] = 0;
    variable[23] = 0;
    ie_len = (2 + SIWIFI_IE_HT_OPERATION_LEN);
    return ie_len;
}
static int siwifi_fill_ie_vht_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_VHT_CAPABILITY;
        variable[1] = SIWIFI_IE_VHT_CAPABILITY_LEN;
        variable[2] = 0xb1;
        variable[3] = 0x1;
        variable[4] = 0x80;
        variable[5] = 0x3;
        variable[6] = 0xfa;
        variable[7] = 0xff;
        variable[8] = 0xc;
        variable[9] = 0x3;
        variable[10] = 0xfa;
        variable[11] = 0xff;
        variable[12] = 0xc;
        variable[13] = 0x3;
        ie_len = (2 + SIWIFI_IE_VHT_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_get_center_freq(struct siwifi_vif *vif)
{
    int center_freq = vif->siwifi_hw->chanctx_table[vif->ch_index].chan_def.chan->center_freq;
    if (center_freq > 5735 && center_freq < 5835)
        return 155;
    if (center_freq > 5170 && center_freq < 5250)
        return 42;
    if (center_freq > 5250 && center_freq < 5330)
        return 58;
    printk("get freq(%d) center fail\n", center_freq);
    return 0;
}
static int siwifi_fill_ie_vht_operation(struct siwifi_vif *siwifi_vif, uint8_t *variable)
{
    int ie_len = 0;
    uint8_t center_freq = siwifi_get_center_freq(siwifi_vif);
    if (!center_freq)
        return 0;
    variable[0] = WLAN_EID_VHT_OPERATION;
    variable[1] = SIWIFI_IE_VHT_OPERATION_LEN;
    variable[2] = 1;
    variable[3] = center_freq;
    variable[4] = 0;
    variable[5] = 0xfc;
    variable[6] = 0xff;
    ie_len = (2 + SIWIFI_IE_VHT_OPERATION_LEN);
    return ie_len;
}
static int siwifi_fill_ie_vht_tx_power_envelope(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_TX_POWER_ENVELOPE;
        variable[1] = SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN;
        variable[2] = 0x02;
        variable[3] = 0x3c;
        variable[4] = 0x3c;
        variable[5] = 0x3c;
        ie_len = (2 + SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_wmm(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_VENDOR_SPECIFIC;
        variable[1] = SIWIFI_IE_WMM_LEN_2;
        variable[2] = 0;
        variable[3] = 0x50;
        variable[4] = 0xf2;
        variable[5] = 0x02;
        variable[6] = 0x01;
        variable[7] = 0x01;
        variable[8] = 0x01;
        variable[9] = 0;
        variable[10] = 0x03;
        variable[11] = 0xa4;
        variable[12] = 0;
        variable[13] = 0;
        variable[14] = 0x27;
        variable[15] = 0xa4;
        variable[16] = 0;
        variable[17] = 0;
        variable[18] = 0x42;
        variable[19] = 0x43;
        variable[20] = 0x5e;
        variable[11] = 0;
        variable[22] = 0x62;
        variable[23] = 0x32;
        variable[24] = 0x2f;
        variable[25] = 0;
        ie_len = (2 + SIWIFI_IE_WMM_LEN_2);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_custom_wmm(uint8_t *variable)
{
    int ie_len = SIWIFI_IE_WMM_LEN + 2;
    uint8_t custom_ie[SIWIFI_IE_WMM_LEN + 2] = {
        WLAN_EID_VENDOR_SPECIFIC,
        SIWIFI_IE_WMM_LEN,
        0,
        0x50,
        0xf2,
        0x2,
        0,
        0x1,
        0
    };
    return siwifi_fill_ie_wmm(variable, custom_ie, ie_len);
}
static int siwifi_fill_ie_pwr_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_PWR_CAPABILITY;
        variable[1] = SIWIFI_IE_PWR_CAPABILITY_LEN;
        variable[2] = 0;
        variable[3] = 25;
        ie_len = (2 + SIWIFI_IE_PWR_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_erp_information(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        if (!is_hb) {
            variable[0] = WLAN_EID_ERP_INFO;
            variable[1] = SIWIFI_IE_ERP_INFORMATION_LEN;
            variable[2] = 0x02;
            ie_len = (2 + SIWIFI_IE_ERP_INFORMATION_LEN);
        }
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}
static int siwifi_fill_ie_default(uint8_t *variable, enum ieee80211_eid eid, int is_hb)
{
    int ret = 0;
    switch(eid)
    {
        case WLAN_EID_SUPP_RATES:
            ret = siwifi_ie_default(variable, NULL, SIWIFI_IE_SUPP_RATES_LEN, is_hb);
            break;
        case WLAN_EID_EXT_SUPP_RATES:
            ret = siwifi_ie_ext_supp_rates(variable, NULL, SIWIFI_IE_EXT_SUPP_RATES_LEN);
            break;
        case WLAN_EID_SUPPORTED_CHANNELS:
            ret = siwifi_fill_ie_supported_channels(variable, NULL, SIWIFI_IE_SUPPORTED_CHANNELS_LEN);
            break;
        case WLAN_EID_COUNTRY:
            ret = siwifi_fill_ie_country(variable, NULL, SIWIFI_IE_COUNTRY_LEN, is_hb);
            break;
        case WLAN_EID_QBSS_LOAD:
            ret = siwifi_fill_ie_qbss(variable, NULL, SIWIFI_IE_QBSS_LEN);
            break;
        case WLAN_EID_SUPPORTED_REGULATORY_CLASSES:
            ret = siwifi_fill_ie_supp_reg(variable, NULL, SIWIFI_IE_SUPPORTED_REGULATORY_LEN, is_hb);
            break;
        case WLAN_EID_HT_CAPABILITY:
            ret = siwifi_fill_ie_ht_capability(variable, NULL, SIWIFI_IE_HT_CAPABILITY_LEN, is_hb);
            break;
        case WLAN_EID_EXT_CAPABILITY:
            ret = siwifi_fill_ie_ext_capability(variable, NULL, SIWIFI_IE_EXT_CAPABILITY_LEN);
            break;
        case WLAN_EID_VHT_CAPABILITY:
            ret = siwifi_fill_ie_vht_capability(variable, NULL, SIWIFI_IE_VHT_CAPABILITY_LEN);
            break;
        case WLAN_EID_TX_POWER_ENVELOPE:
            ret = siwifi_fill_ie_vht_tx_power_envelope(variable, NULL, SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN);
            break;
        case WLAN_EID_VENDOR_SPECIFIC:
            ret = siwifi_fill_ie_wmm(variable, NULL, SIWIFI_IE_WMM_LEN_2);
            break;
        case WLAN_EID_PWR_CAPABILITY:
            ret = siwifi_fill_ie_pwr_capability(variable, NULL, SIWIFI_IE_PWR_CAPABILITY_LEN);
            break;
        case WLAN_EID_ERP_INFO:
            ret = siwifi_fill_ie_erp_information(variable, NULL, SIWIFI_IE_ERP_INFORMATION_LEN, is_hb);
            break;
        default:
            return 0;
    }
    return ret;
}
static int siwifi_fill_beacon_frame(struct siwifi_vif *siwifi_vif, uint8_t *beacon_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt beacon_mgmt;
    int beacon_length = 0;
    int beacon_ie_len = 0;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    uint8_t *variable = NULL;
    beacon_mgmt.frame_control = 0x80;
    beacon_length += 2;
    beacon_mgmt.duration = 0;
    beacon_length += 2;
    ether_addr_copy(beacon_mgmt.da, node_mac);
    ether_addr_copy(beacon_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(beacon_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    beacon_length += 18;
    beacon_mgmt.seq_ctrl = 0x00;
    beacon_length += 2;
    beacon_mgmt.u.beacon.timestamp = 0;
    beacon_mgmt.u.beacon.beacon_int = 100;
    beacon_mgmt.u.beacon.capab_info = 0x21;
    beacon_length += 12;
    memcpy(beacon_frame, &beacon_mgmt, beacon_length);
    variable = beacon_frame + beacon_length;
    variable[beacon_ie_len] = WLAN_EID_SSID;
    variable[beacon_ie_len+1] = siwifi_vif->ap_settings->ssid_len;
    memcpy((variable + beacon_ie_len + 2), siwifi_vif->ap_settings->ssid, siwifi_vif->ap_settings->ssid_len);
    beacon_ie_len += (2 + siwifi_vif->ap_settings->ssid_len);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_SUPP_RATES, is_hb);
    beacon_ie_len += siwifi_fill_ie_dsps(siwifi_vif, variable + beacon_ie_len);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_COUNTRY, is_hb);
    if (!is_hb)
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_ERP_INFO, is_hb);
    if (!is_hb)
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_EXT_SUPP_RATES, is_hb);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_QBSS_LOAD, is_hb);
    beacon_ie_len += siwifi_fill_ie_custom_supp_reg(variable + beacon_ie_len, is_hb);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_HT_CAPABILITY, is_hb);
    beacon_ie_len += siwifi_fill_ie_ht_operation(siwifi_vif, variable + beacon_ie_len);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_EXT_CAPABILITY, is_hb);
    if (is_hb) {
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_VHT_CAPABILITY, is_hb);
        beacon_ie_len += siwifi_fill_ie_vht_operation(siwifi_vif, variable + beacon_ie_len);
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_TX_POWER_ENVELOPE, is_hb);
    }
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_VENDOR_SPECIFIC, is_hb);
    beacon_length += beacon_ie_len;
    if (beacon_length > length) {
        printk("beacon frame length[%d] is larger than skb length[%d] \n", beacon_length, length);
        return 0;
    }
    return beacon_length;
}
static int siwifi_fill_auth_req_frame(struct siwifi_vif *siwifi_vif, uint8_t *auth_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt auth_mgmt;
    int auth_length = 0;
    auth_mgmt.frame_control = 0xb0;
    auth_length += 2;
    auth_mgmt.duration = 0x3c;
    auth_length += 2;
    ether_addr_copy(auth_mgmt.da, node_mac);
    ether_addr_copy(auth_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(auth_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    auth_length += 18;
    auth_mgmt.seq_ctrl = 0;
    auth_length += 2;
    auth_mgmt.u.auth.auth_alg = 0;
    auth_mgmt.u.auth.auth_transaction = 1;
    auth_mgmt.u.auth.status_code = 0;
    auth_length += 6;
    if (auth_length > length) {
        printk("auth req frame length[%d] is larger than skb length[%d] \n", auth_length, length);
        return 0;
    }
    printk("fill auth length(%d) ie_len(0)\n", auth_length);
    memcpy(auth_frame, &auth_mgmt, auth_length);
    return auth_length;
}
static int siwifi_fill_auth_rsp_frame(struct siwifi_vif *siwifi_vif, uint8_t *auth_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt auth_mgmt;
    int auth_length = 0;
    int auth_ie_len = 0;
    auth_mgmt.frame_control = 0xB0;
    auth_length += 2;
    auth_mgmt.duration = 0;
    auth_length += 2;
    ether_addr_copy(auth_mgmt.da, node_mac);
    ether_addr_copy(auth_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(auth_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    auth_length += 18;
    auth_mgmt.seq_ctrl = 0x00;
    auth_length += 2;
    auth_mgmt.u.auth.auth_alg = 0;
    auth_mgmt.u.auth.auth_transaction = 2;
    auth_mgmt.u.auth.status_code = 0;
    auth_length += 6;
    memcpy(auth_frame, &auth_mgmt, auth_length);
    if (auth_length > length) {
        printk("auth rsp frame length[%d] is larger than skb length[%d] \n", auth_length, length);
        return 0;
    }
    printk("fill auth length(%d) ie_len(%d)\n", auth_length, auth_ie_len);
    return auth_length;
}
static int siwifi_fill_deauth_frame(struct siwifi_vif *siwifi_vif, uint8_t *deauth_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt deauth_mgmt;
    int deauth_length = 0;
    int deauth_ie_len = 0;
    deauth_mgmt.frame_control = 0xC0;
    deauth_length += 2;
    deauth_mgmt.duration = 0;
    deauth_length += 2;
    ether_addr_copy(deauth_mgmt.da, node_mac);
    ether_addr_copy(deauth_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(deauth_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    deauth_length += 18;
    deauth_mgmt.seq_ctrl = 0x00;
    deauth_length += 2;
    deauth_mgmt.u.deauth.reason_code = WLAN_REASON_UNSPECIFIED;
    deauth_length += 2;
    memcpy(deauth_frame, &deauth_mgmt, deauth_length);
    if (deauth_length > length) {
        printk("deauth frame length[%d] is larger than skb length[%d] \n", deauth_length, length);
        return 0;
    }
    printk("fill deauth length(%d) ie_len(%d)\n", deauth_length, deauth_ie_len);
    return deauth_length;
}
static int siwifi_fill_assoc_req_frame(struct siwifi_vif *siwifi_vif, uint8_t *assoc_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt assoc_mgmt;
    int assocreq_length = 0;
    int assocreq_ie_len = 0;
    struct cfg80211_ap_settings *siwifi_ap_settings = NULL;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    uint8_t *variable = NULL;
    assoc_mgmt.frame_control = 0x00;
    assocreq_length += 2;
    assoc_mgmt.duration = 0x3c;
    assocreq_length += 2;
    ether_addr_copy(assoc_mgmt.da, node_mac);
    ether_addr_copy(assoc_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(assoc_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    assocreq_length += 18;
    assoc_mgmt.seq_ctrl = 0x10;
    assocreq_length += 2;
    assoc_mgmt.u.assoc_req.capab_info = 0x0521;
    assoc_mgmt.u.assoc_req.listen_interval = 5;
    assocreq_length += 4;
    memcpy(assoc_frame, &assoc_mgmt, assocreq_length);
    variable = assoc_frame + assocreq_length;
    siwifi_ap_settings = siwifi_vif->ap_settings;
    variable[assocreq_ie_len] = WLAN_EID_SSID;
    variable[assocreq_ie_len+1] = siwifi_ap_settings->ssid_len;
    memcpy((variable + assocreq_ie_len + 2), siwifi_ap_settings->ssid, siwifi_ap_settings->ssid_len);
    assocreq_ie_len += (2 + siwifi_ap_settings->ssid_len);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_SUPP_RATES, is_hb);
    if (!is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_EXT_SUPP_RATES, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_PWR_CAPABILITY, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_SUPPORTED_CHANNELS, is_hb);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_EXT_CAPABILITY, is_hb);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_SUPPORTED_REGULATORY_CLASSES, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_custom_wmm(variable + assocreq_ie_len);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_HT_CAPABILITY, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_VHT_CAPABILITY, is_hb);
    assocreq_length += assocreq_ie_len;
    if (assocreq_length > length) {
        printk("assoc frame length[%d] is larger than skb length[%d] \n", assocreq_length, length);
        return 0;
    }
    return assocreq_length;
}
static int siwifi_fill_assoc_rsp_frame(struct siwifi_vif *siwifi_vif, uint8_t *assoc_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt assocresp_mgmt;
    int assocresp_length = 0;
    int assocresp_ie_len = 0;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    uint8_t *variable = NULL;
    assocresp_mgmt.frame_control = 0x10;
    assocresp_length += 2;
    assocresp_mgmt.duration = 0;
    assocresp_length += 2;
    ether_addr_copy(assocresp_mgmt.da, node_mac);
    ether_addr_copy(assocresp_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(assocresp_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    assocresp_length += 18;
    assocresp_mgmt.seq_ctrl = 0x00;
    assocresp_length += 2;
    assocresp_mgmt.u.assoc_resp.capab_info = 0x21;
    assocresp_mgmt.u.assoc_resp.status_code = 0;
    assocresp_mgmt.u.assoc_resp.aid = 1;
    assocresp_length += 6;
    memcpy(assoc_frame, &assocresp_mgmt, assocresp_length);
    variable = assoc_frame + assocresp_length;
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_SUPP_RATES, is_hb);
    if (!is_hb)
        assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_EXT_SUPP_RATES, is_hb);
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_HT_CAPABILITY, is_hb);
    assocresp_ie_len += siwifi_fill_ie_ht_operation(siwifi_vif, variable + assocresp_ie_len);
    if (is_hb) {
        assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_VHT_CAPABILITY, is_hb);
        assocresp_ie_len += siwifi_fill_ie_vht_operation(siwifi_vif, variable + assocresp_ie_len);
    }
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_EXT_CAPABILITY, is_hb);
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_VENDOR_SPECIFIC, is_hb);
    assocresp_length += assocresp_ie_len;
    if (assocresp_length > length) {
        printk("assocresp frame length[%d] is larger than skb length[%d] \n", assocresp_length, length);
        return 0;
    }
    printk("fill assocresp length(%d) ie_len(%d)\n", assocresp_length, assocresp_ie_len);
    return assocresp_length;
}
static int siwifi_fill_probe_rsp_frame(struct siwifi_vif *siwifi_vif, uint8_t *probe_rsp_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt probe_rsp_mgmt;
    int probe_rsp_length = 0;
    int probe_rsp_ie_length = 0;
    uint8_t *variable = NULL;
    struct cfg80211_ap_settings *siwifi_ap_settings = NULL;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    probe_rsp_mgmt.frame_control = 0x50;
    probe_rsp_length += 2;
    probe_rsp_mgmt.duration = 0;
    probe_rsp_length += 2;
    ether_addr_copy(probe_rsp_mgmt.da, node_mac);
    ether_addr_copy(probe_rsp_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(probe_rsp_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    probe_rsp_length += 18;
    probe_rsp_mgmt.seq_ctrl = 0x00;
    probe_rsp_length += 2;
    probe_rsp_mgmt.u.probe_resp.timestamp = 0;
    probe_rsp_mgmt.u.probe_resp.beacon_int = 100;
    probe_rsp_mgmt.u.probe_resp.capab_info = 0x21;
    probe_rsp_length += 12;
    memcpy(probe_rsp_frame, &probe_rsp_mgmt, probe_rsp_length);
    variable = probe_rsp_frame + probe_rsp_length;
    siwifi_ap_settings = siwifi_vif->ap_settings;
    variable[probe_rsp_ie_length] = WLAN_EID_SSID;
    variable[probe_rsp_ie_length+1] = siwifi_ap_settings->ssid_len;
    memcpy((variable + probe_rsp_ie_length + 2), siwifi_ap_settings->ssid, siwifi_ap_settings->ssid_len);
    probe_rsp_ie_length += (2 + siwifi_ap_settings->ssid_len);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_SUPP_RATES, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_dsps(siwifi_vif, variable + probe_rsp_ie_length);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_COUNTRY, is_hb);
    if (is_hb)
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_QBSS_LOAD, is_hb);
    if (is_hb)
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_ERP_INFO, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_custom_supp_reg(variable + probe_rsp_ie_length, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_HT_CAPABILITY, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_ht_operation(siwifi_vif, variable + probe_rsp_ie_length);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_EXT_CAPABILITY, is_hb);
    if (is_hb) {
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VHT_CAPABILITY, is_hb);
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VHT_OPERATION, is_hb);
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_TX_POWER_ENVELOPE, is_hb);
    }
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VENDOR_SPECIFIC, is_hb);
    probe_rsp_length += probe_rsp_ie_length;
    if (probe_rsp_length > length) {
        printk("proberesp frame length[%d] is larger than skb length[%d] \n", probe_rsp_length, length);
        return 0;
    }
    printk("fill proberesp length(%d) ie_len(%d)\n", probe_rsp_length, probe_rsp_ie_length);
    return probe_rsp_length;
}
struct sk_buff *siwifi_create_beacon(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t beacon_frame[SIWIFI_BEACON_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_BEACON_SKB_LENGTH);
    fill_length = siwifi_fill_beacon_frame(siwifi_vif, beacon_frame, SIWIFI_BEACON_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, beacon_frame, fill_length);
    return ret_skb;
}
struct sk_buff *siwifi_create_auth_req(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t auth_frame[SIWIFI_AUTH_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_AUTH_SKB_LENGTH);
    fill_length = siwifi_fill_auth_req_frame(siwifi_vif, auth_frame, SIWIFI_AUTH_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, auth_frame, fill_length);
    return ret_skb;
}
struct sk_buff *siwifi_create_auth_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t auth_frame[SIWIFI_AUTH_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_AUTH_SKB_LENGTH);
    fill_length = siwifi_fill_auth_rsp_frame(siwifi_vif, auth_frame, SIWIFI_AUTH_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, auth_frame, fill_length);
    return ret_skb;
}
struct sk_buff *siwifi_create_deauth(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t deauth_frame[SIWIFI_DEAUTH_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_DEAUTH_SKB_LENGTH);
    fill_length = siwifi_fill_deauth_frame(siwifi_vif, deauth_frame, SIWIFI_DEAUTH_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, deauth_frame, fill_length);
    printk("siwifi_create_deauth success fill_length(%d) \n", fill_length);
    return ret_skb;
}
struct sk_buff *siwifi_create_assoc_req(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t assoc_frame[SIWIFI_ASSOC_REQ_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_ASSOC_REQ_SKB_LENGTH);
    fill_length = siwifi_fill_assoc_req_frame(siwifi_vif, assoc_frame, SIWIFI_ASSOC_REQ_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, assoc_frame, fill_length);
    printk("siwifi_create_assoc_req success fill_length(%d) \n", fill_length);
    return ret_skb;
}
struct sk_buff *siwifi_create_assoc_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t assoc_frame[SIWIFI_ASSOC_RSP_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_ASSOC_RSP_SKB_LENGTH);
    fill_length = siwifi_fill_assoc_rsp_frame(siwifi_vif, assoc_frame, SIWIFI_ASSOC_RSP_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, assoc_frame, fill_length);
    printk("siwifi_create_assoc_rsp success fill_length(%d) \n", fill_length);
    return ret_skb;
}
struct sk_buff *siwifi_create_probe_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t probe_rsp_frame[SIWIFI_PROBE_RSP_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_PROBE_RSP_SKB_LENGTH);
    fill_length = siwifi_fill_probe_rsp_frame(siwifi_vif, probe_rsp_frame, SIWIFI_PROBE_RSP_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, probe_rsp_frame, fill_length);
    printk("siwifi_create_probe_rsp success fill_length(%d) \n", fill_length);
    return ret_skb;
}
int siwifi_frame_send_mgmt(struct siwifi_vif *vif, struct sk_buff *skb)
{
    struct cfg80211_mgmt_tx_params params;
    u64 cookie;
    int ret = 0;
    params.len = skb->len;
    params.buf = skb->data;
    ret = siwifi_start_mgmt_xmit(vif, NULL, &params, false, &cookie);
    return ret;
}
