#ifndef _IQ_TE_CORE_H_
#define _IQ_TE_CORE_H_ 
#include <linux/mutex.h>
#include <linux/spinlock.h>
struct iq_test_engine_platform_context{
    struct device *dev;
    struct mutex iq_te_mtx;
    spinlock_t iq_te_lock;
    int8_t band_type;
    int8_t rp_mode;
    int8_t over_write;
    int8_t sel_bb;
    uint32_t iq_offset;
    uint32_t iq_length;
    uint32_t iq_trigger;
    uint32_t op_time;
    int32_t use_dma;
};
extern struct iq_test_engine_platform_context *g_iq_te_pl_ctx;
int8_t iq_engine_rp_init(void *);
void iq_engine_rp_deinit(void *);
void iq_engine_record_start(void *);
void iq_engine_record_stop(void *);
void iq_engine_player_start(void *);
void iq_engine_player_stop(void *);
int iq_engine_player_once(void *);
int iq_engine_player_once(void *);
int iq_engine_record_once(void *);
void sys_iram_reset(void);
#endif
