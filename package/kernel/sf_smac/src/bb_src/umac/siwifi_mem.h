#ifndef _SIWIFI_MEM_H_
#define _SIWIFI_MEM_H_ 
#include <linux/skbuff.h>
void *siwifi_kmalloc_l(int size, uint32_t flag, char *debug);
void* siwifi_kzalloc_l(int size, uint32_t flag, char *debug);
#ifdef MEMORY_USAGE_DEBUG
void siwifi_kfree(const void *data);
#define siwifi_kzalloc(size,flags) \
({ \
    char debug_string[256]; \
    sprintf(debug_string, "%s %d\n", __func__,__LINE__); \
    siwifi_kzalloc_l(size, flags, debug_string); \
})
#define siwifi_kmalloc(size,flags) \
({ \
    char debug_string[256]; \
    sprintf(debug_string, "%s %d\n", __func__,__LINE__); \
    siwifi_kmalloc_l(size, flags, debug_string); \
})
#else
#define siwifi_kzalloc kzalloc
#define siwifi_kmalloc kmalloc
#define siwifi_kfree kfree
#endif
void siwifi_init_debug_mem(void);
void siwifi_deinit_debug_mem(void);
void siwifi_print_mem_info(void);
#ifdef CONFIG_PRIV_RX_BUFFER_POOL
struct sk_buff *siwifi_dev_alloc_rxskb(int size);
int siwifi_init_private_rxskbs(uint32_t num, uint32_t size);
void siwifi_deinit_private_rxskbs(void);
#else
static inline struct sk_buff *siwifi_dev_alloc_rxskb(int size)
{
    return dev_alloc_skb(size);
};
#endif
void siwifi_set_debug_mem_ctx(void *ctx);
#endif
