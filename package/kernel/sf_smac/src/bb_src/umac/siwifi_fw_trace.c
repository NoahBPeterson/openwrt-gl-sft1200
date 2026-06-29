#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include "siwifi_fw_trace.h"
#define SIWIFI_FW_TRACE_HEADER_LEN 4
#define SIWIFI_FW_TRACE_HEADER_FMT "ts=%12u ID=%8d"
#define SIWIFI_FW_TRACE_HEADER_ASCII_LEN (3 + 12 + 4 + 8)
#define SIWIFI_FW_TRACE_PARAM_FMT ", %5d"
#define SIWIFI_FW_TRACE_PARAM_ASCII_LEN (7)
#define SIWIFI_FW_TRACE_NB_PARAM(a) ((*a >> 8) & 0xff)
#define SIWIFI_FW_TRACE_ID(a) (uint32_t)(((a[0] & 0xff) << 16) + a[1])
#define SIWIFI_FW_TRACE_ENTRY_SIZE(a) (SIWIFI_FW_TRACE_NB_PARAM(a) + \
                                     SIWIFI_FW_TRACE_HEADER_LEN)
#define SIWIFI_FW_TRACE_READY 0x1234
#define SIWIFI_FW_TRACE_LOCKED 0xdead
#define SIWIFI_FW_TRACE_LOCKED_HOST 0x0230
#define SIWIFI_FW_TRACE_LAST_ENTRY 0xffff
#define SIWIFI_FW_TRACE_RESET "*** RESET ***\n"
#define SIWIFI_FW_TRACE_RESET_SIZE sizeof(SIWIFI_FW_TRACE_RESET) - 1
static int trace_last_reset=0;
#define SIWIFI_FW_TRACE_CHECK_INT_MS 1000
static void siwifi_fw_trace_work(struct work_struct *ws)
{
    struct delayed_work *dw = container_of(ws, struct delayed_work, work);
    struct siwifi_fw_trace *trace = container_of(dw, struct siwifi_fw_trace, work);
    if (trace->closing ||
        (!siwifi_fw_trace_empty(&trace->buf) &&
         trace->last_read_index != *trace->buf.end)) {
        trace->last_read_index = *trace->buf.end;
        wake_up_interruptible(&trace->queue);
        return;
    }
    if (waitqueue_active(&trace->queue) && !delayed_work_pending(dw)) {
        schedule_delayed_work(dw, msecs_to_jiffies(SIWIFI_FW_TRACE_CHECK_INT_MS));
    }
}
static void siwifi_fw_trace_buf_lock(struct siwifi_fw_trace_buf *shared_buf)
{
  wait:
    while(*shared_buf->lock == SIWIFI_FW_TRACE_LOCKED) {}
    *shared_buf->lock &= SIWIFI_FW_TRACE_LOCKED_HOST;
    if (*shared_buf->lock == SIWIFI_FW_TRACE_LOCKED)
        goto wait;
}
static void siwifi_fw_trace_buf_unlock(struct siwifi_fw_trace_buf *shared_buf)
{
    *shared_buf->lock = SIWIFI_FW_TRACE_READY;
}
int siwifi_fw_trace_buf_init(struct siwifi_fw_trace_buf *shared_buf,
                           struct siwifi_fw_trace_ipc_desc *ipc)
{
    uint16_t lock_status = ipc->pattern;
    if ((lock_status != SIWIFI_FW_TRACE_READY &&
         lock_status != SIWIFI_FW_TRACE_LOCKED)) {
        shared_buf->data = NULL;
        return -ENOENT;
    }
    shared_buf->data = (uint16_t *)((uint8_t *)(&ipc->offset) + ipc->offset);
    shared_buf->lock = &ipc->pattern;
    shared_buf->size = ipc->size;
    shared_buf->start = &ipc->start;
    shared_buf->end = &ipc->end;
    shared_buf->reset_idx = ++trace_last_reset;
    if ((ipc->nb_compo >> 16) == SIWIFI_FW_TRACE_READY) {
        shared_buf->nb_compo = ipc->nb_compo & 0xffff;
        shared_buf->compo_table = (uint32_t *)((uint8_t *)(&ipc->offset_compo)
                                               + ipc->offset_compo);
    } else {
        shared_buf->nb_compo = 0;
        shared_buf->compo_table = NULL;
    }
    return 0;
}
int siwifi_fw_trace_init(struct siwifi_fw_trace *trace,
                       struct siwifi_fw_trace_ipc_desc *ipc)
{
    if (siwifi_fw_trace_buf_init(&trace->buf, ipc))
        return -ENOENT;
    INIT_DELAYED_WORK(&trace->work, siwifi_fw_trace_work);
    init_waitqueue_head(&trace->queue);
    mutex_init(&trace->mutex);
    trace->closing = false;
    return 0;
}
void siwifi_fw_trace_deinit(struct siwifi_fw_trace *trace)
{
    if (trace->buf.data == NULL)
        return;
    trace->closing = true;
    cancel_delayed_work_sync(&trace->work);
    trace->buf.data = NULL;
}
static void siwifi_fw_trace_reset_local(struct siwifi_fw_trace_local_buf *local_buf)
{
    local_buf->read = local_buf->data;
    local_buf->write = local_buf->data;
    local_buf->nb_entries = 0;
    local_buf->free_space = local_buf->size;
    local_buf->last_read = NULL;
    local_buf->reset_idx = 0;
    local_buf->show_reset = NULL;
}
int siwifi_fw_trace_alloc_local(struct siwifi_fw_trace_local_buf *local_buf,
                              int size)
{
    local_buf->data = siwifi_kmalloc(size * sizeof(uint16_t), GFP_KERNEL);
    if (!local_buf->data) {
        return -ENOMEM;
    }
    local_buf->data_end = local_buf->data + size;
    local_buf->size = size;
    siwifi_fw_trace_reset_local(local_buf);
    return 0;
}
void siwifi_fw_trace_free_local(struct siwifi_fw_trace_local_buf *local_buf)
{
    if (local_buf->data)
        siwifi_kfree(local_buf->data);
    local_buf->data = NULL;
}
static inline int siwifi_fw_trace_strlen(uint16_t *entry)
{
    return (SIWIFI_FW_TRACE_HEADER_ASCII_LEN +
            (SIWIFI_FW_TRACE_NB_PARAM(entry) * SIWIFI_FW_TRACE_PARAM_ASCII_LEN) +
            1);
}
static uint16_t *siwifi_fw_trace_to_str(uint16_t *trace, char *buf, size_t *size)
{
    uint32_t ts, id;
    int nb_param;
    int res, buf_idx = 0, left = *size;
    id = SIWIFI_FW_TRACE_ID(trace);
    nb_param = SIWIFI_FW_TRACE_NB_PARAM(trace);
    trace +=2;
    ts = *trace++;
    ts <<= 16;
    ts += *trace++;
    res = scnprintf(&buf[buf_idx], left, SIWIFI_FW_TRACE_HEADER_FMT, ts, id);
    buf_idx += res;
    left -= res;
    while (nb_param > 0) {
        res = scnprintf(&buf[buf_idx], left, SIWIFI_FW_TRACE_PARAM_FMT, *trace++);
        buf_idx += res;
        left -= res;
        nb_param--;
    }
    res = scnprintf(&buf[buf_idx], left, "\n");
    left -= res;
    *size = (*size - left);
    return trace;
}
static void siwifi_fw_trace_copy_entry(struct siwifi_fw_trace_local_buf *local_buf,
                                     uint16_t *trace_entry, int size)
{
    uint16_t *write = local_buf->write;
    uint16_t *read = trace_entry;
    int i;
    for (i = 0; i < size; i++) {
        *write++ = *read++;
    }
    if (write >= local_buf->data_end)
        local_buf->write = local_buf->data;
    else
        local_buf->write = write;
    local_buf->free_space -= size;
    local_buf->last_read = trace_entry;
    local_buf->last_read_value = *trace_entry;
    local_buf->nb_entries++;
}
static int siwifi_fw_trace_copy(struct siwifi_fw_trace *trace,
                              struct siwifi_fw_trace_local_buf *local_buf)
{
    struct siwifi_fw_trace_buf *trace_buf = &trace->buf;
    uint16_t *ptr, *ptr_end, *ptr_limit;
    int entry_size, ret = 0;
    if (mutex_lock_interruptible(&trace->mutex))
        return 0;
    if (local_buf->reset_idx != trace_buf->reset_idx) {
        local_buf->show_reset = local_buf->write;
        local_buf->reset_idx = trace_buf->reset_idx;
        local_buf->last_read = NULL;
    }
    siwifi_fw_trace_buf_lock(trace_buf);
    ptr_end = trace_buf->data + *trace_buf->end;
    if (siwifi_fw_trace_empty(trace_buf) || (ptr_end == local_buf->last_read))
        goto end;
    ptr_limit = trace_buf->data + trace_buf->size;
    if (local_buf->last_read &&
        (local_buf->last_read_value == *local_buf->last_read)) {
        ptr = local_buf->last_read;
        ptr += SIWIFI_FW_TRACE_ENTRY_SIZE(ptr);
    } else {
        ptr = trace_buf->data + *trace_buf->start;
    }
    while (1) {
        if ((ptr == ptr_limit) || (*ptr == SIWIFI_FW_TRACE_LAST_ENTRY))
             ptr = trace_buf->data;
        entry_size = SIWIFI_FW_TRACE_ENTRY_SIZE(ptr);
        if (entry_size > local_buf->size) {
            pr_err("FW_TRACE local buffer too small, trace skipped");
            goto next_entry;
        }
        if (local_buf->free_space >= entry_size) {
            int contiguous = local_buf->data_end - local_buf->write;
            if ((local_buf->write < local_buf->read) || contiguous >= entry_size) {
                siwifi_fw_trace_copy_entry(local_buf, ptr, entry_size);
                ret++;
            } else if ((local_buf->free_space - contiguous) >= entry_size) {
                *local_buf->write = SIWIFI_FW_TRACE_LAST_ENTRY;
                if (local_buf->show_reset == local_buf->write)
                    local_buf->show_reset = local_buf->data;
                local_buf->write = local_buf->data;
                local_buf->free_space -= contiguous;
                siwifi_fw_trace_copy_entry(local_buf, ptr, entry_size);
                ret++;
            } else {
                goto end;
            }
        } else {
            goto end;
        }
        if (ptr == ptr_end) {
            break;
        } else if (ptr > ptr_limit) {
            pr_err("Corrupted trace buffer\n");
            break;
        }
      next_entry:
        ptr += entry_size;
    }
  end:
    siwifi_fw_trace_buf_unlock(trace_buf);
    mutex_unlock(&trace->mutex);
    return ret;
}
static size_t siwifi_fw_trace_read_local(struct siwifi_fw_trace_local_buf *local_buf,
                                       char __user *user_buf, size_t size)
{
    uint16_t *ptr;
    char str[1824];
    size_t str_size;
    int entry_size;
    size_t res = 0 , remain = size, not_cpy = 0;
    if (!local_buf->nb_entries)
        return res;
    ptr = local_buf->read;
    while(local_buf->nb_entries && !not_cpy) {
        if (local_buf->show_reset == ptr) {
            if (remain < SIWIFI_FW_TRACE_RESET_SIZE)
                break;
            local_buf->show_reset = NULL;
            not_cpy = copy_to_user(user_buf + res, SIWIFI_FW_TRACE_RESET,
                                   SIWIFI_FW_TRACE_RESET_SIZE);
            res += (SIWIFI_FW_TRACE_RESET_SIZE - not_cpy);
            remain -= (SIWIFI_FW_TRACE_RESET_SIZE - not_cpy);
        }
        if (remain < siwifi_fw_trace_strlen(ptr))
            break;
        entry_size = SIWIFI_FW_TRACE_ENTRY_SIZE(ptr);
        str_size = sizeof(str);
        ptr = siwifi_fw_trace_to_str(ptr, str, &str_size);
        not_cpy = copy_to_user(user_buf + res, str, str_size);
        str_size -= not_cpy;
        res += str_size;
        remain -= str_size;
        local_buf->nb_entries--;
        local_buf->free_space += entry_size;
        if (ptr >= local_buf->data_end) {
            ptr = local_buf->data;
        } else if (*ptr == SIWIFI_FW_TRACE_LAST_ENTRY) {
            local_buf->free_space += local_buf->data_end - ptr;
            ptr = local_buf->data;
        }
        local_buf->read = ptr;
    }
    if ( !local_buf->nb_entries) {
        local_buf->write = local_buf->read = local_buf->data;
        local_buf->free_space = local_buf->size;
    }
    return res;
}
size_t siwifi_fw_trace_read(struct siwifi_fw_trace *trace,
                          struct siwifi_fw_trace_local_buf *local_buf,
                          bool dont_wait, char __user *user_buf, size_t size)
{
    size_t res = 0;
    siwifi_fw_trace_copy(trace, local_buf);
    while(!local_buf->nb_entries) {
        int last_index;
        if (dont_wait)
            return -EAGAIN;
        if (!delayed_work_pending(&trace->work)) {
            trace->last_read_index = *trace->buf.end;
            schedule_delayed_work(&trace->work,
                                  msecs_to_jiffies(SIWIFI_FW_TRACE_CHECK_INT_MS));
        }
        last_index = *trace->buf.end;
        if (wait_event_interruptible(trace->queue,
                                     (trace->closing ||
                                      (last_index != *trace->buf.end)))) {
            return -ERESTARTSYS;
        }
        if (trace->closing)
            return 0;
        siwifi_fw_trace_copy(trace, local_buf);
    }
    while (1) {
        size_t read;
        read = siwifi_fw_trace_read_local(local_buf, user_buf + res, size - res);
        res += read;
        siwifi_fw_trace_copy(trace, local_buf);
        if (!read)
            break;
    }
    return res;
}
void _siwifi_fw_trace_dump(struct siwifi_fw_trace_buf *trace_buf)
{
    uint16_t *ptr, *ptr_end, *ptr_limit, *next_ptr;
    char buf[1824];
    size_t size;
    if (!trace_buf->data || siwifi_fw_trace_empty(trace_buf))
        return;
    siwifi_fw_trace_buf_lock(trace_buf);
    ptr = trace_buf->data + *trace_buf->start;
    ptr_end = trace_buf->data + *trace_buf->end;
    ptr_limit = trace_buf->data + trace_buf->size;
    while (1) {
        size = sizeof(buf);
        next_ptr = siwifi_fw_trace_to_str(ptr, buf, &size);
        pr_info("%s", buf);
        if (ptr == ptr_end) {
            break;
        } else if ((next_ptr == ptr_limit) ||
                   (*next_ptr == SIWIFI_FW_TRACE_LAST_ENTRY)) {
            ptr = trace_buf->data;
        } else if (next_ptr > ptr_limit) {
            pr_err("Corrupted trace buffer\n");
            break;
        } else {
            ptr = next_ptr;
        }
    }
    siwifi_fw_trace_buf_unlock(trace_buf);
}
int _siwifi_fw_trace_reset(struct siwifi_fw_trace *trace, bool lock)
{
    struct siwifi_fw_trace_buf *trace_buf = &trace->buf;
    if (lock && mutex_lock_interruptible(&trace->mutex))
        return -ERESTARTSYS;
    if (trace->buf.data) {
        siwifi_fw_trace_buf_lock(trace_buf);
        *trace_buf->start = 0;
        *trace_buf->end = trace_buf->size + 1;
        trace_buf->reset_idx = ++trace_last_reset;
        siwifi_fw_trace_buf_unlock(trace_buf);
    }
    if (lock)
        mutex_unlock(&trace->mutex);
    return 0;
}
static uint32_t siwifi_fw_trace_get_trace_level(struct siwifi_fw_trace_buf *trace_buf,
                                              unsigned int compo_id)
{
    if (compo_id >= trace_buf->nb_compo)
        return 0;
    return trace_buf->compo_table[compo_id];
}
static void siwifi_fw_trace_set_trace_level(struct siwifi_fw_trace_buf *trace_buf,
                                          unsigned int compo_id, uint32_t level)
{
    if (compo_id > trace_buf->nb_compo)
        return;
    if (compo_id == trace_buf->nb_compo) {
        int i;
        for (i = 0; i < trace_buf->nb_compo; i++) {
            trace_buf->compo_table[i] = level;
        }
    } else {
        trace_buf->compo_table[compo_id] = level;
    }
}
size_t siwifi_fw_trace_level_read(struct siwifi_fw_trace *trace,
                                char __user *user_buf, size_t len, loff_t *ppos)
{
    struct siwifi_fw_trace_buf *trace_buf = &trace->buf;
    size_t res = 0;
    int i, size;
    char *buf;
    size = trace_buf->nb_compo * 16;
    buf = siwifi_kmalloc(size, GFP_KERNEL);
    if (buf == NULL)
        return 0;
    if (mutex_lock_interruptible(&trace->mutex)) {
        siwifi_kfree(buf);
        return -ERESTARTSYS;
    }
    for (i = 0 ; i < trace_buf->nb_compo ; i ++) {
        res += scnprintf(&buf[res], size - res, "%3d:0x%08x\n", i,
                         siwifi_fw_trace_get_trace_level(trace_buf, i));
    }
    mutex_unlock(&trace->mutex);
    res = simple_read_from_buffer(user_buf, len, ppos, buf, res);
    siwifi_kfree(buf);
    return res;
}
size_t siwifi_fw_trace_level_write(struct siwifi_fw_trace *trace,
                                 const char __user *user_buf, size_t len)
{
    struct siwifi_fw_trace_buf *trace_buf = &trace->buf;
    char *buf, *token, *next;
    buf = siwifi_kmalloc(len + 1, GFP_KERNEL);
    if (buf == NULL)
        return -ENOMEM;
    if (copy_from_user(buf, user_buf, len)) {
        siwifi_kfree(buf);
        return -EFAULT;
    }
    buf[len] = '\0';
    if (mutex_lock_interruptible(&trace->mutex)) {
        siwifi_kfree(buf);
        return -ERESTARTSYS;
    }
    next = buf;
    token = strsep(&next, " \t\n");
    while (token) {
        unsigned int compo, level;
        if ((sscanf(token, "%d:0x%x", &compo, &level) == 2)||
            (sscanf(token, "%d:%d", &compo, &level) == 2)) {
            siwifi_fw_trace_set_trace_level(trace_buf, compo, level);
        }
        token = strsep(&next, " \t");
    }
    mutex_unlock(&trace->mutex);
    siwifi_kfree(buf);
    return len;
}
