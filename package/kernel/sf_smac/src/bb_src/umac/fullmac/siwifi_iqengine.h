#ifndef _SIWIFI_IQENGINE_H_
#define _SIWIFI_IQENGINE_H_ 
#include "siwifi_defs.h"
#define SIWIFI_IQ_OUTPUT_FILE "/tmp/iq_recorder.txt"
#define SIWIFI_IQ_INPUT_FILE "/tmp/iq_player.txt"
#define SIWIFI_IQ_BASE_ADDR 0x20000000
#define SIWIFI_IQ_BUFFER_MAX 524288
enum {
    IQE_MODE_NONE,
    IQE_BB_PLAYER,
    IQE_RF_PLAYER,
    IQE_BB_RECORDER,
    IQE_RF_RECORDER,
};
int iqe_init(struct siwifi_hw *priv, uint32_t iqlength, int mode);
void iqe_enable(struct siwifi_hw *priv, int enable);
#endif
