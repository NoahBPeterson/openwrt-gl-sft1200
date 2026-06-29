#ifndef _SIWIFI_FW_TRACE_H_
#define _SIWIFI_FW_TRACE_H_ 
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
struct siwifi_fw_trace_ipc_desc {
    volatile uint16_t pattern;
    volatile uint32_t start;
    volatile uint32_t end;
    volatile uint32_t size;
    volatile uint32_t offset;
    volatile uint32_t nb_compo;
    volatile uint32_t offset_compo;
};
struct siwifi_fw_trace_buf {
    volatile uint16_t *lock;
    uint16_t *data;
    uint32_t size;
    volatile uint32_t *start;
    volatile uint32_t *end;
    int reset_idx;
    unsigned int nb_compo;
    uint32_t *compo_table;
};
struct siwifi_fw_trace_local_buf {
    uint16_t *data;
    uint16_t *data_end;
    uint32_t size;
    uint16_t *read;
    uint16_t *write;
    uint16_t nb_entries;
    uint32_t free_space;
    uint16_t *last_read;
    uint16_t last_read_value;
    int reset_idx;
    uint16_t * show_reset;
};
struct siwifi_fw_trace {
    struct siwifi_fw_trace_buf buf;
    struct mutex mutex;
    wait_queue_head_t queue;
    struct delayed_work work;
    int last_read_index;
    bool closing;
};
int siwifi_fw_trace_init(struct siwifi_fw_trace *trace,
                       struct siwifi_fw_trace_ipc_desc *ipc);
void siwifi_fw_trace_deinit(struct siwifi_fw_trace *trace);
int siwifi_fw_trace_buf_init(struct siwifi_fw_trace_buf *shared_buf,
                           struct siwifi_fw_trace_ipc_desc *ipc);
int _siwifi_fw_trace_reset(struct siwifi_fw_trace *trace, bool lock);
void _siwifi_fw_trace_dump(struct siwifi_fw_trace_buf *trace);
int siwifi_fw_trace_alloc_local(struct siwifi_fw_trace_local_buf *local,
                              int size);
void siwifi_fw_trace_free_local(struct siwifi_fw_trace_local_buf *local);
size_t siwifi_fw_trace_read(struct siwifi_fw_trace *trace,
                          struct siwifi_fw_trace_local_buf *local_buf,
                          bool dont_wait, char __user *user_buf, size_t size);
size_t siwifi_fw_trace_level_read(struct siwifi_fw_trace *trace,
                                char __user *user_buf, size_t len, loff_t *ppos);
size_t siwifi_fw_trace_level_write(struct siwifi_fw_trace *trace,
                                 const char __user *user_buf, size_t len);
static inline bool siwifi_fw_trace_empty(struct siwifi_fw_trace_buf *shared_buf)
{
    return (*shared_buf->end >= shared_buf->size);
}
#endif
